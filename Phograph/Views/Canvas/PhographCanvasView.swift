import MetalKit
import SwiftUI

#if os(macOS)
import AppKit
typealias PlatformView = NSView
#else
import UIKit
typealias PlatformView = UIView
#endif

/// MTKView subclass that routes touch/mouse/keyboard events to the C++ engine
/// and displays the rendered pixel buffer via MetalRenderer.
/// Follows the iospharo PharoCanvasView pattern.
class PhographCanvasView: MTKView {

    var renderer: MetalRenderer?
    private let bridge: PhographBridge

    init(bridge: PhographBridge) {
        self.bridge = bridge
        super.init(frame: .zero, device: MTLCreateSystemDefaultDevice())

        self.preferredFramesPerSecond = 60
        self.isPaused = false
        self.enableSetNeedsDisplay = false

        if let renderer = MetalRenderer(mtkView: self) {
            self.renderer = renderer
            self.delegate = renderer
            renderer.pixelBufferProvider = { [weak self] in
                guard let self = self else { return nil }
                var w: Int32 = 0
                var h: Int32 = 0
                guard let ptr = self.bridge.pixelBufferWidth(&w, height: &h),
                      w > 0, h > 0 else { return nil }
                return (UnsafeRawPointer(ptr), Int(w), Int(h))
            }
        }
    }

    required init(coder: NSCoder) {
        fatalError("init(coder:) not supported")
    }

    #if os(macOS)
    // MARK: - macOS Mouse Events

    override var acceptsFirstResponder: Bool { true }

    override func mouseDown(with event: NSEvent) {
        let pt = convertToLocal(event)
        bridge.sendPointerDown(Float(pt.x), y: Float(pt.y), button: 0)
    }

    override func mouseUp(with event: NSEvent) {
        let pt = convertToLocal(event)
        bridge.sendPointerUp(Float(pt.x), y: Float(pt.y), button: 0)
    }

    override func mouseDragged(with event: NSEvent) {
        let pt = convertToLocal(event)
        bridge.sendPointerDrag(Float(pt.x), y: Float(pt.y),
                               dx: Float(event.deltaX), dy: Float(event.deltaY))
    }

    override func mouseMoved(with event: NSEvent) {
        let pt = convertToLocal(event)
        bridge.sendPointerMove(Float(pt.x), y: Float(pt.y))
    }

    override func scrollWheel(with event: NSEvent) {
        let pt = convertToLocal(event)
        bridge.sendScroll(Float(pt.x), y: Float(pt.y),
                          dx: Float(event.scrollingDeltaX),
                          dy: Float(event.scrollingDeltaY))
    }

    override func keyDown(with event: NSEvent) {
        let mods = modifierFlags(event.modifierFlags)
        bridge.sendKeyDown(event.characters ?? "", keyCode: UInt32(event.keyCode), modifiers: mods)
    }

    override func keyUp(with event: NSEvent) {
        let mods = modifierFlags(event.modifierFlags)
        bridge.sendKeyUp(event.characters ?? "", keyCode: UInt32(event.keyCode), modifiers: mods)
    }

    private func convertToLocal(_ event: NSEvent) -> NSPoint {
        let pt = convert(event.locationInWindow, from: nil)
        // Flip Y: NSView origin is bottom-left, we want top-left
        return NSPoint(x: pt.x, y: bounds.height - pt.y)
    }

    private func modifierFlags(_ flags: NSEvent.ModifierFlags) -> UInt32 {
        var mods: UInt32 = 0
        if flags.contains(.shift)   { mods |= 1 }
        if flags.contains(.control) { mods |= 2 }
        if flags.contains(.option)  { mods |= 4 }
        if flags.contains(.command) { mods |= 8 }
        return mods
    }
    #else
    // MARK: - iOS Touch Events

    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let touch = touches.first else { return }
        let pt = touch.location(in: self)
        bridge.sendPointerDown(Float(pt.x), y: Float(pt.y), button: 0)
    }

    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let touch = touches.first else { return }
        let pt = touch.location(in: self)
        let prev = touch.previousLocation(in: self)
        bridge.sendPointerDrag(Float(pt.x), y: Float(pt.y),
                               dx: Float(pt.x - prev.x), dy: Float(pt.y - prev.y))
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let touch = touches.first else { return }
        let pt = touch.location(in: self)
        bridge.sendPointerUp(Float(pt.x), y: Float(pt.y), button: 0)
    }
    #endif
}

// MARK: - SwiftUI Wrapper

#if os(macOS)
struct PhographCanvasRepresentable: NSViewRepresentable {
    let bridge: PhographBridge

    func makeNSView(context: Context) -> PhographCanvasView {
        PhographCanvasView(bridge: bridge)
    }

    func updateNSView(_ nsView: PhographCanvasView, context: Context) {}
}
#else
struct PhographCanvasRepresentable: UIViewRepresentable {
    let bridge: PhographBridge

    func makeUIView(context: Context) -> PhographCanvasView {
        PhographCanvasView(bridge: bridge)
    }

    func updateUIView(_ uiView: PhographCanvasView, context: Context) {}
}
#endif
