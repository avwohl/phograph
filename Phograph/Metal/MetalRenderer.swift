import MetalKit
import Foundation

/// Renders a BGRA8 pixel buffer from the C++ engine onto a fullscreen quad.
/// Follows the iospharo pattern: CPU pixel buffer -> MTLTexture -> fragment shader.
class MetalRenderer: NSObject, MTKViewDelegate {

    private let device: MTLDevice
    private let commandQueue: MTLCommandQueue
    private var pipelineState: MTLRenderPipelineState!
    private var texture: MTLTexture?
    private var textureWidth: Int = 0
    private var textureHeight: Int = 0

    /// Callback to get the current pixel buffer from the engine.
    /// Returns (pointer, width, height) or nil if no buffer.
    var pixelBufferProvider: (() -> (UnsafeRawPointer, Int, Int)?)?

    init?(mtkView: MTKView) {
        guard let device = mtkView.device ?? MTLCreateSystemDefaultDevice() else {
            return nil
        }
        self.device = device
        mtkView.device = device

        guard let queue = device.makeCommandQueue() else {
            return nil
        }
        self.commandQueue = queue

        super.init()

        mtkView.colorPixelFormat = .bgra8Unorm
        mtkView.delegate = self

        buildPipeline(mtkView: mtkView)
    }

    private func buildPipeline(mtkView: MTKView) {
        guard let library = device.makeDefaultLibrary() else {
            print("MetalRenderer: Failed to create default library")
            return
        }
        let vertexFunc = library.makeFunction(name: "fullscreenQuadVertex")
        let fragmentFunc = library.makeFunction(name: "fullscreenQuadFragment")

        let desc = MTLRenderPipelineDescriptor()
        desc.vertexFunction = vertexFunc
        desc.fragmentFunction = fragmentFunc
        desc.colorAttachments[0].pixelFormat = mtkView.colorPixelFormat

        // Enable alpha blending
        desc.colorAttachments[0].isBlendingEnabled = true
        desc.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        desc.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha
        desc.colorAttachments[0].sourceAlphaBlendFactor = .one
        desc.colorAttachments[0].destinationAlphaBlendFactor = .oneMinusSourceAlpha

        do {
            pipelineState = try device.makeRenderPipelineState(descriptor: desc)
        } catch {
            print("MetalRenderer: Failed to create pipeline state: \(error)")
        }
    }

    private func ensureTexture(width: Int, height: Int) {
        guard width > 0 && height > 0 else { return }
        if textureWidth == width && textureHeight == height && texture != nil {
            return
        }
        let desc = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .bgra8Unorm,
            width: width,
            height: height,
            mipmapped: false
        )
        desc.usage = [.shaderRead]
        texture = device.makeTexture(descriptor: desc)
        textureWidth = width
        textureHeight = height
    }

    // MARK: - MTKViewDelegate

    func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        // The drawable size changed; nothing special needed.
    }

    func draw(in view: MTKView) {
        guard let pipelineState = pipelineState,
              let drawable = view.currentDrawable,
              let passDescriptor = view.currentRenderPassDescriptor,
              let commandBuffer = commandQueue.makeCommandBuffer() else {
            return
        }

        // Upload pixel buffer to texture
        if let provider = pixelBufferProvider,
           let (ptr, w, h) = provider() {
            ensureTexture(width: w, height: h)
            if let tex = texture {
                let bytesPerRow = w * 4
                tex.replace(
                    region: MTLRegionMake2D(0, 0, w, h),
                    mipmapLevel: 0,
                    withBytes: ptr,
                    bytesPerRow: bytesPerRow
                )
            }
        }

        passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1)
        passDescriptor.colorAttachments[0].loadAction = .clear
        passDescriptor.colorAttachments[0].storeAction = .store

        guard let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: passDescriptor) else {
            return
        }

        encoder.setRenderPipelineState(pipelineState)

        if let tex = texture {
            encoder.setFragmentTexture(tex, index: 0)
        }

        // Draw fullscreen quad (6 vertices, generated in vertex shader)
        encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 6)
        encoder.endEncoding()

        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}
