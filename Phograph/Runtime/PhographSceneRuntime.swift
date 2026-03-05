// PhographSceneRuntime.swift
// Scene graph types + CoreGraphics rendering for standalone compiled apps.
// Works on both macOS and iOS (CoreGraphics is universal).

import Foundation
import CoreGraphics
import CoreText

// MARK: - Color

struct PhoColor {
    var r: Double, g: Double, b: Double, a: Double

    static let clear = PhoColor(r: 0, g: 0, b: 0, a: 0)
    static let black = PhoColor(r: 0, g: 0, b: 0, a: 1)
    static let white = PhoColor(r: 1, g: 1, b: 1, a: 1)

    var cgColor: CGColor {
        CGColor(red: r, green: g, blue: b, alpha: a)
    }

    static func fromPhoValue(_ v: PhoValue) -> PhoColor {
        // Accepts integer RGBA (0xRRGGBBAA) or list [r,g,b] / [r,g,b,a]
        switch v {
        case .integer(let i):
            let u = UInt32(bitPattern: Int32(truncatingIfNeeded: i))
            return PhoColor(
                r: Double((u >> 24) & 0xFF) / 255.0,
                g: Double((u >> 16) & 0xFF) / 255.0,
                b: Double((u >> 8) & 0xFF) / 255.0,
                a: Double(u & 0xFF) / 255.0
            )
        case .list(let arr):
            let r = arr.count > 0 ? arr[0].asReal : 0
            let g = arr.count > 1 ? arr[1].asReal : 0
            let b = arr.count > 2 ? arr[2].asReal : 0
            let a = arr.count > 3 ? arr[3].asReal : 1
            return PhoColor(r: r, g: g, b: b, a: a)
        default:
            return .black
        }
    }

    func toPhoValue() -> PhoValue {
        .list([.real(r), .real(g), .real(b), .real(a)])
    }
}

// MARK: - Shape

enum PhoShapeType {
    case rect
    case oval
    case text(String)
    case group
}

class PhoShape {
    var type: PhoShapeType
    var x: Double = 0, y: Double = 0, w: Double = 0, h: Double = 0
    var fill: PhoColor = .clear
    var stroke: PhoColor = .clear
    var strokeWidth: Double = 1
    var cornerRadius: Double = 0
    var opacity: Double = 1
    var visible: Bool = true
    var tag: String = ""
    var children: [PhoShape] = []

    init(_ type: PhoShapeType, x: Double = 0, y: Double = 0, w: Double = 0, h: Double = 0) {
        self.type = type
        self.x = x; self.y = y; self.w = w; self.h = h
    }

    func render(to ctx: CGContext) {
        guard visible && opacity > 0 else { return }

        ctx.saveGState()
        ctx.setAlpha(opacity)

        let rect = CGRect(x: x, y: y, width: w, height: h)

        switch type {
        case .rect:
            let path: CGPath
            if cornerRadius > 0 {
                path = CGPath(roundedRect: rect, cornerWidth: cornerRadius, cornerHeight: cornerRadius, transform: nil)
            } else {
                path = CGPath(rect: rect, transform: nil)
            }
            if fill.a > 0 {
                ctx.setFillColor(fill.cgColor)
                ctx.addPath(path)
                ctx.fillPath()
            }
            if stroke.a > 0 {
                ctx.setStrokeColor(stroke.cgColor)
                ctx.setLineWidth(strokeWidth)
                ctx.addPath(path)
                ctx.strokePath()
            }

        case .oval:
            let path = CGPath(ellipseIn: rect, transform: nil)
            if fill.a > 0 {
                ctx.setFillColor(fill.cgColor)
                ctx.addPath(path)
                ctx.fillPath()
            }
            if stroke.a > 0 {
                ctx.setStrokeColor(stroke.cgColor)
                ctx.setLineWidth(strokeWidth)
                ctx.addPath(path)
                ctx.strokePath()
            }

        case .text(let str):
            // Simple text rendering via CoreGraphics
            ctx.setFillColor(fill.a > 0 ? fill.cgColor : PhoColor.black.cgColor)
            let fontSize = h > 0 ? h : 14.0
            let font = CTFontCreateWithName("Helvetica" as CFString, fontSize, nil)
            let attrs: [NSAttributedString.Key: Any] = [
                .font: font,
                .foregroundColor: fill.a > 0 ? fill.cgColor : PhoColor.black.cgColor
            ]
            let attrStr = NSAttributedString(string: str, attributes: attrs)
            let line = CTLineCreateWithAttributedString(attrStr)
            ctx.textPosition = CGPoint(x: x, y: y)
            CTLineDraw(line, ctx)

        case .group:
            break
        }

        // Render children
        for child in children {
            child.render(to: ctx)
        }

        ctx.restoreGState()
    }

    func hitTest(px: Double, py: Double) -> PhoShape? {
        guard visible else { return nil }
        // Check children in reverse (front to back)
        for child in children.reversed() {
            if let hit = child.hitTest(px: px, py: py) {
                return hit
            }
        }
        if px >= x && px < x + w && py >= y && py < y + h {
            return self
        }
        return nil
    }

    func findByTag(_ searchTag: String) -> PhoShape? {
        if tag == searchTag { return self }
        for child in children {
            if let found = child.findByTag(searchTag) { return found }
        }
        return nil
    }
}

// MARK: - Canvas

class PhoCanvas {
    let width: Int
    let height: Int
    var pixels: [UInt8]  // BGRA8
    var root: PhoShape?

    init(width: Int, height: Int) {
        self.width = width
        self.height = height
        self.pixels = [UInt8](repeating: 0, count: width * height * 4)
    }

    func clear() {
        pixels = [UInt8](repeating: 0, count: width * height * 4)
    }

    func render() {
        guard let root = root else { return }

        let colorSpace = CGColorSpaceCreateDeviceRGB()
        guard let ctx = CGContext(
            data: &pixels,
            width: width,
            height: height,
            bitsPerComponent: 8,
            bytesPerRow: width * 4,
            space: colorSpace,
            bitmapInfo: CGImageAlphaInfo.premultipliedFirst.rawValue | CGBitmapInfo.byteOrder32Little.rawValue
        ) else { return }

        // Clear to white
        ctx.setFillColor(CGColor(red: 1, green: 1, blue: 1, alpha: 1))
        ctx.fill(CGRect(x: 0, y: 0, width: width, height: height))

        // Flip coordinate system (origin at top-left)
        ctx.translateBy(x: 0, y: CGFloat(height))
        ctx.scaleBy(x: 1, y: -1)

        root.render(to: ctx)
    }

    func pixelAt(x: Int, y: Int) -> PhoValue {
        guard x >= 0 && x < width && y >= 0 && y < height else { return .null }
        let i = (y * width + x) * 4
        let b = pixels[i], g = pixels[i+1], r = pixels[i+2], a = pixels[i+3]
        return .list([.integer(Int64(r)), .integer(Int64(g)), .integer(Int64(b)), .integer(Int64(a))])
    }

    func setPixel(x: Int, y: Int, color: PhoColor) {
        guard x >= 0 && x < width && y >= 0 && y < height else { return }
        let i = (y * width + x) * 4
        pixels[i]   = UInt8(Swift.min(255, Swift.max(0, color.b * 255)))
        pixels[i+1] = UInt8(Swift.min(255, Swift.max(0, color.g * 255)))
        pixels[i+2] = UInt8(Swift.min(255, Swift.max(0, color.r * 255)))
        pixels[i+3] = UInt8(Swift.min(255, Swift.max(0, color.a * 255)))
    }

    func fillRect(x: Int, y: Int, w: Int, h: Int, color: PhoColor) {
        for py in y..<min(y+h, height) {
            for px in x..<min(x+w, width) {
                if px >= 0 && py >= 0 { setPixel(x: px, y: py, color: color) }
            }
        }
    }

    func strokeRect(x: Int, y: Int, w: Int, h: Int, color: PhoColor) {
        for px in x..<min(x+w, width) {
            if px >= 0 { setPixel(x: px, y: y, color: color); setPixel(x: px, y: y+h-1, color: color) }
        }
        for py in y..<min(y+h, height) {
            if py >= 0 { setPixel(x: x, y: py, color: color); setPixel(x: x+w-1, y: py, color: color) }
        }
    }

    func fillOval(x: Int, y: Int, w: Int, h: Int, color: PhoColor) {
        let cx = Double(x) + Double(w) / 2.0
        let cy = Double(y) + Double(h) / 2.0
        let rx = Double(w) / 2.0, ry = Double(h) / 2.0
        guard rx > 0 && ry > 0 else { return }
        for py in y..<min(y+h, height) {
            for px in x..<min(x+w, width) {
                let dx = (Double(px) + 0.5 - cx) / rx
                let dy = (Double(py) + 0.5 - cy) / ry
                if dx*dx + dy*dy <= 1.0 && px >= 0 && py >= 0 {
                    setPixel(x: px, y: py, color: color)
                }
            }
        }
    }

    func fillRoundedRect(x: Int, y: Int, w: Int, h: Int, radius: Double, color: PhoColor) {
        let colorSpace = CGColorSpaceCreateDeviceRGB()
        guard let ctx = CGContext(
            data: &pixels,
            width: width,
            height: height,
            bitsPerComponent: 8,
            bytesPerRow: width * 4,
            space: colorSpace,
            bitmapInfo: CGImageAlphaInfo.premultipliedFirst.rawValue | CGBitmapInfo.byteOrder32Little.rawValue
        ) else { return }
        let rect = CGRect(x: x, y: y, width: w, height: h)
        let path = CGPath(roundedRect: rect, cornerWidth: radius, cornerHeight: radius, transform: nil)
        ctx.setFillColor(color.cgColor)
        ctx.addPath(path)
        ctx.fillPath()
    }
}

// Global canvas registry for standalone apps
var phoCanvasRegistry: [Int: PhoCanvas] = [:]
var phoShapeRegistry: [Int: PhoShape] = [:]
private var phoNextCanvasId = 1
private var phoNextShapeId = 1

// The "active" canvas for rendering output (used by CanvasView in exported apps)
var phoActiveCanvas: PhoCanvas?

// MARK: - Canvas Primitives

func phoCreateCanvas(_ w: PhoValue, _ h: PhoValue) -> PhoValue {
    let canvas = PhoCanvas(width: Int(w.asInteger), height: Int(h.asInteger))
    let id = phoNextCanvasId; phoNextCanvasId += 1
    phoCanvasRegistry[id] = canvas
    phoActiveCanvas = canvas
    return .integer(Int64(id))
}

func phoCanvasWidth(_ canvasId: PhoValue) -> PhoValue {
    guard let c = phoCanvasRegistry[Int(canvasId.asInteger)] else { return .error("invalid canvas") }
    return .integer(Int64(c.width))
}

func phoCanvasHeight(_ canvasId: PhoValue) -> PhoValue {
    guard let c = phoCanvasRegistry[Int(canvasId.asInteger)] else { return .error("invalid canvas") }
    return .integer(Int64(c.height))
}

func phoCanvasClear(_ canvasId: PhoValue) -> PhoValue {
    guard let c = phoCanvasRegistry[Int(canvasId.asInteger)] else { return .error("invalid canvas") }
    c.clear()
    return .null
}

func phoCanvasRender(_ canvasId: PhoValue) -> PhoValue {
    guard let c = phoCanvasRegistry[Int(canvasId.asInteger)] else { return .error("invalid canvas") }
    c.render()
    return .null
}

func phoCanvasPixelAt(_ canvasId: PhoValue, _ x: PhoValue, _ y: PhoValue) -> PhoValue {
    guard let c = phoCanvasRegistry[Int(canvasId.asInteger)] else { return .error("invalid canvas") }
    return c.pixelAt(x: Int(x.asInteger), y: Int(y.asInteger))
}

func phoCanvasSetRoot(_ canvasId: PhoValue, _ shapeId: PhoValue) -> PhoValue {
    guard let c = phoCanvasRegistry[Int(canvasId.asInteger)] else { return .error("invalid canvas") }
    guard let s = phoShapeRegistry[Int(shapeId.asInteger)] else { return .error("invalid shape") }
    c.root = s
    return .null
}

// MARK: - Shape Primitives

func phoShapeRect(_ x: PhoValue, _ y: PhoValue, _ w: PhoValue, _ h: PhoValue) -> PhoValue {
    let s = PhoShape(.rect, x: x.asReal, y: y.asReal, w: w.asReal, h: h.asReal)
    let id = phoNextShapeId; phoNextShapeId += 1
    phoShapeRegistry[id] = s
    return .integer(Int64(id))
}

func phoShapeOval(_ x: PhoValue, _ y: PhoValue, _ w: PhoValue, _ h: PhoValue) -> PhoValue {
    let s = PhoShape(.oval, x: x.asReal, y: y.asReal, w: w.asReal, h: h.asReal)
    let id = phoNextShapeId; phoNextShapeId += 1
    phoShapeRegistry[id] = s
    return .integer(Int64(id))
}

func phoShapeText(_ text: PhoValue, _ x: PhoValue, _ y: PhoValue) -> PhoValue {
    let s = PhoShape(.text(text.asString), x: x.asReal, y: y.asReal)
    let id = phoNextShapeId; phoNextShapeId += 1
    phoShapeRegistry[id] = s
    return .integer(Int64(id))
}

func phoShapeGroup() -> PhoValue {
    let s = PhoShape(.group)
    let id = phoNextShapeId; phoNextShapeId += 1
    phoShapeRegistry[id] = s
    return .integer(Int64(id))
}

func phoShapeSetFill(_ shapeId: PhoValue, _ color: PhoValue) -> PhoValue {
    guard let s = phoShapeRegistry[Int(shapeId.asInteger)] else { return .error("invalid shape") }
    s.fill = PhoColor.fromPhoValue(color)
    return shapeId
}

func phoShapeSetStroke(_ shapeId: PhoValue, _ color: PhoValue) -> PhoValue {
    guard let s = phoShapeRegistry[Int(shapeId.asInteger)] else { return .error("invalid shape") }
    s.stroke = PhoColor.fromPhoValue(color)
    return shapeId
}

func phoShapeSetCornerRadius(_ shapeId: PhoValue, _ radius: PhoValue) -> PhoValue {
    guard let s = phoShapeRegistry[Int(shapeId.asInteger)] else { return .error("invalid shape") }
    s.cornerRadius = radius.asReal
    return shapeId
}

func phoShapeSetOpacity(_ shapeId: PhoValue, _ opacity: PhoValue) -> PhoValue {
    guard let s = phoShapeRegistry[Int(shapeId.asInteger)] else { return .error("invalid shape") }
    s.opacity = opacity.asReal
    return shapeId
}

func phoShapeSetBounds(_ shapeId: PhoValue, _ x: PhoValue, _ y: PhoValue, _ w: PhoValue, _ h: PhoValue) -> PhoValue {
    guard let s = phoShapeRegistry[Int(shapeId.asInteger)] else { return .error("invalid shape") }
    s.x = x.asReal; s.y = y.asReal; s.w = w.asReal; s.h = h.asReal
    return shapeId
}

func phoShapeSetVisible(_ shapeId: PhoValue, _ vis: PhoValue) -> PhoValue {
    guard let s = phoShapeRegistry[Int(shapeId.asInteger)] else { return .error("invalid shape") }
    s.visible = vis.asBool
    return shapeId
}

func phoShapeSetTag(_ shapeId: PhoValue, _ tag: PhoValue) -> PhoValue {
    guard let s = phoShapeRegistry[Int(shapeId.asInteger)] else { return .error("invalid shape") }
    s.tag = tag.asString
    return shapeId
}

func phoShapeAddChild(_ parentId: PhoValue, _ childId: PhoValue) -> PhoValue {
    guard let p = phoShapeRegistry[Int(parentId.asInteger)] else { return .error("invalid parent shape") }
    guard let c = phoShapeRegistry[Int(childId.asInteger)] else { return .error("invalid child shape") }
    p.children.append(c)
    return parentId
}

func phoShapeGetFill(_ shapeId: PhoValue) -> PhoValue {
    guard let s = phoShapeRegistry[Int(shapeId.asInteger)] else { return .error("invalid shape") }
    return s.fill.toPhoValue()
}

func phoShapeGetBounds(_ shapeId: PhoValue) -> PhoValue {
    guard let s = phoShapeRegistry[Int(shapeId.asInteger)] else { return .error("invalid shape") }
    return .list([.real(s.x), .real(s.y), .real(s.w), .real(s.h)])
}

func phoShapeHitTest(_ shapeId: PhoValue, _ x: PhoValue, _ y: PhoValue) -> PhoValue {
    guard let s = phoShapeRegistry[Int(shapeId.asInteger)] else { return .error("invalid shape") }
    if let hit = s.hitTest(px: x.asReal, py: y.asReal) {
        // Find the id of the hit shape
        for (id, shape) in phoShapeRegistry {
            if shape === hit { return .integer(Int64(id)) }
        }
    }
    return .null
}

func phoShapeFindByTag(_ shapeId: PhoValue, _ tag: PhoValue) -> PhoValue {
    guard let s = phoShapeRegistry[Int(shapeId.asInteger)] else { return .error("invalid shape") }
    if let found = s.findByTag(tag.asString) {
        for (id, shape) in phoShapeRegistry {
            if shape === found { return .integer(Int64(id)) }
        }
    }
    return .null
}

// MARK: - Direct Draw Primitives

func phoDrawFillRect(_ canvasId: PhoValue, _ x: PhoValue, _ y: PhoValue, _ w: PhoValue, _ h: PhoValue) -> PhoValue {
    guard let c = phoCanvasRegistry[Int(canvasId.asInteger)] else { return .error("invalid canvas") }
    c.fillRect(x: Int(x.asInteger), y: Int(y.asInteger), w: Int(w.asInteger), h: Int(h.asInteger), color: .white)
    return .null
}

func phoDrawStrokeRect(_ canvasId: PhoValue, _ x: PhoValue, _ y: PhoValue, _ w: PhoValue, _ h: PhoValue) -> PhoValue {
    guard let c = phoCanvasRegistry[Int(canvasId.asInteger)] else { return .error("invalid canvas") }
    c.strokeRect(x: Int(x.asInteger), y: Int(y.asInteger), w: Int(w.asInteger), h: Int(h.asInteger), color: .white)
    return .null
}

func phoDrawFillOval(_ canvasId: PhoValue, _ x: PhoValue, _ y: PhoValue, _ w: PhoValue, _ h: PhoValue) -> PhoValue {
    guard let c = phoCanvasRegistry[Int(canvasId.asInteger)] else { return .error("invalid canvas") }
    c.fillOval(x: Int(x.asInteger), y: Int(y.asInteger), w: Int(w.asInteger), h: Int(h.asInteger), color: .white)
    return .null
}

func phoDrawFillRoundedRect(_ canvasId: PhoValue, _ x: PhoValue, _ y: PhoValue, _ w: PhoValue, _ h: PhoValue, _ r: PhoValue) -> PhoValue {
    guard let c = phoCanvasRegistry[Int(canvasId.asInteger)] else { return .error("invalid canvas") }
    c.fillRoundedRect(x: Int(x.asInteger), y: Int(y.asInteger), w: Int(w.asInteger), h: Int(h.asInteger), radius: r.asReal, color: .white)
    return .null
}

func phoDrawSetPixel(_ canvasId: PhoValue, _ x: PhoValue, _ y: PhoValue, _ color: PhoValue) -> PhoValue {
    guard let c = phoCanvasRegistry[Int(canvasId.asInteger)] else { return .error("invalid canvas") }
    c.setPixel(x: Int(x.asInteger), y: Int(y.asInteger), color: PhoColor.fromPhoValue(color))
    return .null
}

func phoDrawClear(_ canvasId: PhoValue, _ color: PhoValue) -> PhoValue {
    guard let c = phoCanvasRegistry[Int(canvasId.asInteger)] else { return .error("invalid canvas") }
    let col = PhoColor.fromPhoValue(color)
    for y in 0..<c.height {
        for x in 0..<c.width {
            c.setPixel(x: x, y: y, color: col)
        }
    }
    return .null
}
