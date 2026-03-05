import Foundation
#if os(macOS)
import CoreGraphics
import AppKit
#endif

/// Exports a GraphModel as SVG or PDF vector graphics.
enum GraphExporter {

    // MARK: - Colors

    private struct RGBColor {
        let r: Double, g: Double, b: Double
        var svgFill: String { "rgb(\(Int(r*255)),\(Int(g*255)),\(Int(b*255)))" }
        var cgColor: CGColor { CGColor(red: r, green: g, blue: b, alpha: 1) }
    }

    private static func headerColor(for nodeType: String) -> RGBColor {
        switch nodeType {
        case "input_bar":   return RGBColor(r: 0.2,  g: 0.55, b: 0.3)
        case "output_bar":  return RGBColor(r: 0.7,  g: 0.2,  b: 0.2)
        case "constant":    return RGBColor(r: 0.5,  g: 0.45, b: 0.2)
        case "primitive":   return RGBColor(r: 0.25, g: 0.35, b: 0.6)
        case "method_call": return RGBColor(r: 0.4,  g: 0.25, b: 0.55)
        default:            return RGBColor(r: 0.4,  g: 0.4,  b: 0.4)
        }
    }

    private static func bodyColor(for nodeType: String) -> RGBColor {
        switch nodeType {
        case "input_bar":   return RGBColor(r: 0.85, g: 0.95, b: 0.87)
        case "output_bar":  return RGBColor(r: 0.97, g: 0.88, b: 0.88)
        case "constant":    return RGBColor(r: 0.98, g: 0.96, b: 0.88)
        case "primitive":   return RGBColor(r: 0.92, g: 0.94, b: 0.98)
        case "method_call": return RGBColor(r: 0.95, g: 0.92, b: 0.98)
        default:            return RGBColor(r: 0.95, g: 0.95, b: 0.95)
        }
    }

    // MARK: - Layout helpers

    private static func pinX(index: Int, count: Int, nodeWidth: CGFloat) -> CGFloat {
        CGFloat(index + 1) * nodeWidth / CGFloat(count + 1)
    }

    private static func graphBounds(_ graph: GraphModel, margin: CGFloat = 30) -> CGRect {
        guard !graph.nodes.isEmpty else { return CGRect(x: 0, y: 0, width: 200, height: 200) }
        var minX = CGFloat.infinity, minY = CGFloat.infinity
        var maxX = -CGFloat.infinity, maxY = -CGFloat.infinity
        for node in graph.nodes {
            minX = min(minX, node.x)
            minY = min(minY, node.y)
            maxX = max(maxX, node.x + node.width)
            maxY = max(maxY, node.y + node.height)
        }
        return CGRect(
            x: minX - margin, y: minY - margin,
            width: maxX - minX + margin * 2,
            height: maxY - minY + margin * 2
        )
    }

    // MARK: - SVG Export

    static func exportSVG(graph: GraphModel) -> String {
        let bounds = graphBounds(graph)
        let pinR: CGFloat = 6
        let headerH: CGFloat = 28

        var svg = """
        <svg xmlns="http://www.w3.org/2000/svg"
             viewBox="\(f(bounds.minX)) \(f(bounds.minY)) \(f(bounds.width)) \(f(bounds.height))"
             width="\(f(bounds.width))" height="\(f(bounds.height))"
             font-family="-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, sans-serif">
        <defs>
          <filter id="shadow" x="-4%" y="-4%" width="108%" height="112%">
            <feDropShadow dx="0" dy="1" stdDeviation="2" flood-opacity="0.15"/>
          </filter>
        </defs>
        """

        // Wires first (behind nodes)
        for wire in graph.wires {
            guard let src = graph.nodes.first(where: { $0.id == wire.sourceNodeId }),
                  let dst = graph.nodes.first(where: { $0.id == wire.destNodeId }) else { continue }
            let sx = src.x + pinX(index: wire.sourcePin, count: src.outputPins.count, nodeWidth: src.width)
            let sy = src.y + src.height
            let ex = dst.x + pinX(index: wire.destPin, count: dst.inputPins.count, nodeWidth: dst.width)
            let ey = dst.y
            let dy = max(abs(ey - sy) * 0.4, 30)
            svg += """
            <path d="M \(f(sx)) \(f(sy)) C \(f(sx)) \(f(sy+dy)) \(f(ex)) \(f(ey-dy)) \(f(ex)) \(f(ey))"
                  fill="none" stroke="rgba(51,51,51,0.55)" stroke-width="1.5"/>
            """
        }

        // Nodes
        for node in graph.nodes {
            let x = node.x, y = node.y, w = node.width, h = node.height
            let hdr = headerColor(for: node.nodeType)
            let bdy = bodyColor(for: node.nodeType)

            svg += "<g filter=\"url(#shadow)\">"
            // Body rect
            svg += "<rect x=\"\(f(x))\" y=\"\(f(y))\" width=\"\(f(w))\" height=\"\(f(h))\" rx=\"4\" fill=\"\(bdy.svgFill)\"/>"
            // Header rect (clipped to top portion)
            let hh = min(headerH, h)
            svg += "<clipPath id=\"hdr-\(node.id.uuidString.prefix(8))\">"
            svg += "<rect x=\"\(f(x))\" y=\"\(f(y))\" width=\"\(f(w))\" height=\"\(f(hh))\"/>"
            svg += "</clipPath>"
            svg += "<rect x=\"\(f(x))\" y=\"\(f(y))\" width=\"\(f(w))\" height=\"\(f(h))\" rx=\"4\" fill=\"\(hdr.svgFill)\" clip-path=\"url(#hdr-\(node.id.uuidString.prefix(8)))\"/>"
            // Label
            let textY = y + hh / 2
            svg += "<text x=\"\(f(x + w/2))\" y=\"\(f(textY))\" text-anchor=\"middle\" dominant-baseline=\"central\" fill=\"white\" font-weight=\"bold\" font-size=\"13\">\(escapeXML(node.label))</text>"

            // Input pins
            for (i, _) in node.inputPins.enumerated() {
                let px = x + pinX(index: i, count: node.inputPins.count, nodeWidth: w)
                svg += "<circle cx=\"\(f(px))\" cy=\"\(f(y))\" r=\"\(f(pinR))\" fill=\"#3380e6\"/>"
            }
            // Output pins
            for (i, _) in node.outputPins.enumerated() {
                let px = x + pinX(index: i, count: node.outputPins.count, nodeWidth: w)
                svg += "<circle cx=\"\(f(px))\" cy=\"\(f(y + h))\" r=\"\(f(pinR))\" fill=\"#e64d33\"/>"
            }
            svg += "</g>"
        }

        svg += "\n</svg>"
        return svg
    }

    private static func f(_ v: CGFloat) -> String {
        let rounded = (v * 100).rounded() / 100
        if rounded == rounded.rounded() { return String(Int(rounded)) }
        return String(format: "%.2f", rounded)
    }

    private static func escapeXML(_ s: String) -> String {
        s.replacingOccurrences(of: "&", with: "&amp;")
         .replacingOccurrences(of: "<", with: "&lt;")
         .replacingOccurrences(of: ">", with: "&gt;")
         .replacingOccurrences(of: "\"", with: "&quot;")
    }

    // MARK: - PDF Export

    #if os(macOS)
    static func exportPDF(graph: GraphModel, to url: URL) throws {
        let bounds = graphBounds(graph)
        let pinR: CGFloat = 6
        let headerH: CGFloat = 28

        var mediaBox = CGRect(x: 0, y: 0, width: bounds.width, height: bounds.height)
        guard let consumer = CGDataConsumer(url: url as CFURL),
              let ctx = CGContext(consumer: consumer, mediaBox: &mediaBox, nil) else {
            throw NSError(domain: "GraphExporter", code: 1, userInfo: [NSLocalizedDescriptionKey: "Failed to create PDF context"])
        }

        ctx.beginPDFPage(nil)

        // Translate so graph bounds map to page origin, flip Y for top-left origin
        ctx.translateBy(x: -bounds.minX, y: bounds.maxY)
        ctx.scaleBy(x: 1, y: -1)

        // Wires
        for wire in graph.wires {
            guard let src = graph.nodes.first(where: { $0.id == wire.sourceNodeId }),
                  let dst = graph.nodes.first(where: { $0.id == wire.destNodeId }) else { continue }
            let sx = src.x + pinX(index: wire.sourcePin, count: src.outputPins.count, nodeWidth: src.width)
            let sy = src.y + src.height
            let ex = dst.x + pinX(index: wire.destPin, count: dst.inputPins.count, nodeWidth: dst.width)
            let ey = dst.y
            let dy = max(abs(ey - sy) * 0.4, 30)

            ctx.setStrokeColor(CGColor(red: 0.2, green: 0.2, blue: 0.2, alpha: 0.55))
            ctx.setLineWidth(1.5)
            ctx.move(to: CGPoint(x: sx, y: sy))
            ctx.addCurve(to: CGPoint(x: ex, y: ey),
                         control1: CGPoint(x: sx, y: sy + dy),
                         control2: CGPoint(x: ex, y: ey - dy))
            ctx.strokePath()
        }

        // Nodes
        for node in graph.nodes {
            let x = node.x, y = node.y, w = node.width, h = node.height
            let hdr = headerColor(for: node.nodeType)
            let bdy = bodyColor(for: node.nodeType)
            let hh = min(headerH, h)

            // Body rounded rect
            let bodyPath = CGPath(roundedRect: CGRect(x: x, y: y, width: w, height: h), cornerWidth: 4, cornerHeight: 4, transform: nil)
            ctx.setFillColor(bdy.cgColor)
            ctx.addPath(bodyPath)
            ctx.fillPath()

            // Header (clip to top portion)
            ctx.saveGState()
            ctx.clip(to: CGRect(x: x, y: y, width: w, height: hh))
            ctx.setFillColor(hdr.cgColor)
            ctx.addPath(bodyPath)
            ctx.fillPath()
            ctx.restoreGState()

            // Label
            let attrs: [NSAttributedString.Key: Any] = [
                .font: NSFont.boldSystemFont(ofSize: 13),
                .foregroundColor: NSColor.white
            ]
            let label = node.label as NSString
            let labelSize = label.size(withAttributes: attrs)
            let labelX = x + (w - labelSize.width) / 2
            let labelY = y + (hh - labelSize.height) / 2
            label.draw(at: CGPoint(x: labelX, y: labelY), withAttributes: attrs)

            // Input pins
            for (i, _) in node.inputPins.enumerated() {
                let px = x + pinX(index: i, count: node.inputPins.count, nodeWidth: w)
                ctx.setFillColor(CGColor(red: 0.2, green: 0.5, blue: 0.9, alpha: 1))
                ctx.fillEllipse(in: CGRect(x: px - pinR, y: y - pinR, width: pinR * 2, height: pinR * 2))
            }
            // Output pins
            for (i, _) in node.outputPins.enumerated() {
                let px = x + pinX(index: i, count: node.outputPins.count, nodeWidth: w)
                ctx.setFillColor(CGColor(red: 0.9, green: 0.3, blue: 0.2, alpha: 1))
                ctx.fillEllipse(in: CGRect(x: px - pinR, y: y + h - pinR, width: pinR * 2, height: pinR * 2))
            }
        }

        ctx.endPDFPage()
        ctx.closePDF()
    }
    #endif
}
