// AppTemplates.swift
// Template strings for generating standalone app projects from Phograph programs.

import Foundation

enum AppTemplates {

    // MARK: - App.swift

    static func appSwift(appName: String) -> String {
        """
        import SwiftUI

        @main
        struct \(appName)App: App {
            var body: some Scene {
                WindowGroup {
                    ContentView()
                }
            }
        }
        """
    }

    // MARK: - ContentView.swift

    static func contentView(entryFunction: String) -> String {
        """
        import SwiftUI

        struct ContentView: View {
            @State private var consoleText: String = ""
            @State private var hasCanvas: Bool = false

            var body: some View {
                Group {
                    if hasCanvas {
                        CanvasView()
                    } else {
                        ScrollView {
                            Text(consoleText)
                                .font(.system(.body, design: .monospaced))
                                .frame(maxWidth: .infinity, alignment: .leading)
                                .padding()
                                .textSelection(.enabled)
                        }
                    }
                }
                .onAppear {
                    runProgram()
                }
            }

            private func runProgram() {
                phoConsoleOutput = ""
                let _ = \(entryFunction)()
                consoleText = phoConsoleOutput
                hasCanvas = phoActiveCanvas != nil
            }
        }
        """
    }

    // MARK: - CanvasView.swift (macOS)

    static func canvasViewMacOS(entryFunction: String) -> String {
        """
        import SwiftUI
        import MetalKit

        struct CanvasView: NSViewRepresentable {
            func makeNSView(context: Context) -> MTKView {
                let view = MTKView()
                view.device = MTLCreateSystemDefaultDevice()
                view.isPaused = false
                view.enableSetNeedsDisplay = false
                view.preferredFramesPerSecond = 60

                let renderer = MetalRenderer(mtkView: view)
                context.coordinator.renderer = renderer

                // Run the program
                phoConsoleOutput = ""
                let _ = \(entryFunction)()

                renderer?.pixelBufferProvider = {
                    guard let canvas = phoActiveCanvas else { return nil }
                    return (UnsafeRawPointer(canvas.pixels), canvas.width, canvas.height)
                }

                return view
            }

            func updateNSView(_ nsView: MTKView, context: Context) {}

            func makeCoordinator() -> Coordinator { Coordinator() }

            class Coordinator {
                var renderer: MetalRenderer?
            }
        }
        """
    }

    // MARK: - CanvasView.swift (iOS)

    static func canvasViewIOS(entryFunction: String) -> String {
        """
        import SwiftUI
        import MetalKit

        struct CanvasView: UIViewRepresentable {
            func makeUIView(context: Context) -> MTKView {
                let view = MTKView()
                view.device = MTLCreateSystemDefaultDevice()
                view.isPaused = false
                view.enableSetNeedsDisplay = false
                view.preferredFramesPerSecond = 60

                let renderer = MetalRenderer(mtkView: view)
                context.coordinator.renderer = renderer

                // Run the program
                phoConsoleOutput = ""
                let _ = \(entryFunction)()

                renderer?.pixelBufferProvider = {
                    guard let canvas = phoActiveCanvas else { return nil }
                    return (UnsafeRawPointer(canvas.pixels), canvas.width, canvas.height)
                }

                return view
            }

            func updateUIView(_ uiView: MTKView, context: Context) {}

            func makeCoordinator() -> Coordinator { Coordinator() }

            class Coordinator {
                var renderer: MetalRenderer?
            }
        }
        """
    }

    // MARK: - MetalRenderer.swift

    static let metalRenderer = """
    import MetalKit
    import Foundation

    class MetalRenderer: NSObject, MTKViewDelegate {
        private let device: MTLDevice
        private let commandQueue: MTLCommandQueue
        private var pipelineState: MTLRenderPipelineState!
        private var texture: MTLTexture?
        private var textureWidth: Int = 0
        private var textureHeight: Int = 0

        var pixelBufferProvider: (() -> (UnsafeRawPointer, Int, Int)?)?

        init?(mtkView: MTKView) {
            guard let device = mtkView.device ?? MTLCreateSystemDefaultDevice() else { return nil }
            self.device = device
            mtkView.device = device
            guard let queue = device.makeCommandQueue() else { return nil }
            self.commandQueue = queue
            super.init()
            mtkView.colorPixelFormat = .bgra8Unorm
            mtkView.delegate = self
            buildPipeline(mtkView: mtkView)
        }

        private func buildPipeline(mtkView: MTKView) {
            guard let library = device.makeDefaultLibrary(),
                  let vertexFunc = library.makeFunction(name: "fullscreenQuadVertex"),
                  let fragmentFunc = library.makeFunction(name: "fullscreenQuadFragment") else { return }
            let desc = MTLRenderPipelineDescriptor()
            desc.vertexFunction = vertexFunc
            desc.fragmentFunction = fragmentFunc
            desc.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat
            desc.colorAttachments[0].isBlendingEnabled = true
            desc.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
            desc.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
            desc.colorAttachments[0].sourceAlphaBlendFactor = .one
            desc.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha
            pipelineState = try? device.makeRenderPipelineState(descriptor: desc)
        }

        private func ensureTexture(width: Int, height: Int) {
            guard width > 0, height > 0 else { return }
            if textureWidth == width && textureHeight == height && texture != nil { return }
            let desc = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .bgra8Unorm, width: width, height: height, mipmapped: false)
            desc.usage = [.shaderRead]
            texture = device.makeTexture(descriptor: desc)
            textureWidth = width; textureHeight = height
        }

        func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {}

        func draw(in view: MTKView) {
            guard let pipelineState = pipelineState,
                  let drawable = view.currentDrawable,
                  let passDescriptor = view.currentRenderPassDescriptor,
                  let commandBuffer = commandQueue.makeCommandBuffer() else { return }
            if let provider = pixelBufferProvider, let (ptr, w, h) = provider() {
                ensureTexture(width: w, height: h)
                if let tex = texture {
                    tex.replace(region: MTLRegionMake2D(0, 0, w, h), mipmapLevel: 0, withBytes: ptr, bytesPerRow: w * 4)
                }
            }
            passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1)
            passDescriptor.colorAttachments[0].loadAction = .clear
            passDescriptor.colorAttachments[0].storeAction = .store
            guard let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: passDescriptor) else { return }
            encoder.setRenderPipelineState(pipelineState)
            if let tex = texture { encoder.setFragmentTexture(tex, index: 0) }
            encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 6)
            encoder.endEncoding()
            commandBuffer.present(drawable)
            commandBuffer.commit()
        }
    }
    """

    // MARK: - Shaders.metal

    static let shadersMetal = """
    #include <metal_stdlib>
    using namespace metal;

    struct VertexOut {
        float4 position [[position]];
        float2 texCoord;
    };

    vertex VertexOut fullscreenQuadVertex(uint vid [[vertex_id]]) {
        float2 positions[6] = {
            float2(-1, -1), float2(-1,  1), float2( 1, -1),
            float2( 1, -1), float2(-1,  1), float2( 1,  1)
        };
        float2 texCoords[6] = {
            float2(0, 1), float2(0, 0), float2(1, 1),
            float2(1, 1), float2(0, 0), float2(1, 0)
        };
        VertexOut out;
        out.position = float4(positions[vid], 0, 1);
        out.texCoord = texCoords[vid];
        return out;
    }

    fragment float4 fullscreenQuadFragment(VertexOut in [[stage_in]],
                                            texture2d<float> tex [[texture(0)]]) {
        constexpr sampler nearestSampler(mag_filter::nearest, min_filter::nearest);
        return tex.sample(nearestSampler, in.texCoord);
    }
    """

    // MARK: - Info.plist

    static func infoPlist(bundleID: String, appName: String, version: String = "1.0") -> String {
        """
        <?xml version="1.0" encoding="UTF-8"?>
        <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
        <plist version="1.0">
        <dict>
            <key>CFBundleIdentifier</key>
            <string>\(bundleID)</string>
            <key>CFBundleName</key>
            <string>\(appName)</string>
            <key>CFBundleDisplayName</key>
            <string>\(appName)</string>
            <key>CFBundleVersion</key>
            <string>1</string>
            <key>CFBundleShortVersionString</key>
            <string>\(version)</string>
            <key>CFBundlePackageType</key>
            <string>APPL</string>
            <key>CFBundleExecutable</key>
            <string>\(appName)</string>
            <key>LSMinimumSystemVersion</key>
            <string>13.0</string>
            <key>ITSAppUsesNonExemptEncryption</key>
            <false/>
        </dict>
        </plist>
        """
    }

    // MARK: - Entitlements

    static func entitlements(appName: String) -> String {
        """
        <?xml version="1.0" encoding="UTF-8"?>
        <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
        <plist version="1.0">
        <dict>
            <key>com.apple.security.app-sandbox</key>
            <true/>
        </dict>
        </plist>
        """
    }

    // MARK: - project.yml (XcodeGen)

    static func projectYml(appName: String, bundleID: String, teamID: String, platform: String, deploymentTarget: String) -> String {
        """
        name: \(appName)
        options:
          bundleIdPrefix: \(bundleID.components(separatedBy: ".").dropLast().joined(separator: "."))
          createIntermediateGroups: true

        settings:
          base:
            SWIFT_VERSION: "5.9"

        targets:
          \(appName):
            type: application
            platform: \(platform)
            deploymentTarget: "\(deploymentTarget)"
            sources:
              - path: Sources
            settings:
              base:
                PRODUCT_BUNDLE_IDENTIFIER: \(bundleID)
                DEVELOPMENT_TEAM: \(teamID)
                CODE_SIGN_STYLE: Automatic
                INFOPLIST_FILE: Info.plist
                CODE_SIGN_ENTITLEMENTS: \(appName).entitlements
                GENERATE_INFOPLIST_FILE: NO
                PRODUCT_NAME: \(appName)
        """
    }

    // MARK: - Assets.xcassets

    static let assetsContentsJSON = """
    {
      "info" : {
        "version" : 1,
        "author" : "xcode"
      }
    }
    """

    static let appIconContentsJSON = """
    {
      "images" : [
        {
          "idiom" : "universal",
          "platform" : "ios",
          "size" : "1024x1024"
        },
        {
          "idiom" : "mac",
          "scale" : "1x",
          "size" : "16x16"
        },
        {
          "idiom" : "mac",
          "scale" : "2x",
          "size" : "16x16"
        },
        {
          "idiom" : "mac",
          "scale" : "1x",
          "size" : "32x32"
        },
        {
          "idiom" : "mac",
          "scale" : "2x",
          "size" : "32x32"
        },
        {
          "idiom" : "mac",
          "scale" : "1x",
          "size" : "128x128"
        },
        {
          "idiom" : "mac",
          "scale" : "2x",
          "size" : "128x128"
        },
        {
          "idiom" : "mac",
          "scale" : "1x",
          "size" : "256x256"
        },
        {
          "idiom" : "mac",
          "scale" : "2x",
          "size" : "256x256"
        },
        {
          "idiom" : "mac",
          "scale" : "1x",
          "size" : "512x512"
        },
        {
          "idiom" : "mac",
          "scale" : "2x",
          "size" : "512x512"
        }
      ],
      "info" : {
        "version" : 1,
        "author" : "xcode"
      }
    }
    """
}
