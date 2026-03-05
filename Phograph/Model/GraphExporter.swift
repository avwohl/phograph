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
        case "input_bar":          return RGBColor(r: 0.2,  g: 0.55, b: 0.3)
        case "output_bar":         return RGBColor(r: 0.7,  g: 0.2,  b: 0.2)
        case "constant":           return RGBColor(r: 0.5,  g: 0.45, b: 0.2)
        case "primitive":          return RGBColor(r: 0.25, g: 0.35, b: 0.6)
        case "method_call":        return RGBColor(r: 0.4,  g: 0.25, b: 0.55)
        case "instance_generator": return RGBColor(r: 0.8,  g: 0.5,  b: 0.0)
        case "get":                return RGBColor(r: 0.2,  g: 0.5,  b: 0.45)
        case "set":                return RGBColor(r: 0.55, g: 0.3,  b: 0.2)
        default:                   return RGBColor(r: 0.4,  g: 0.4,  b: 0.4)
        }
    }

    private static func bodyColor(for nodeType: String) -> RGBColor {
        switch nodeType {
        case "input_bar":          return RGBColor(r: 0.85, g: 0.95, b: 0.87)
        case "output_bar":         return RGBColor(r: 0.97, g: 0.88, b: 0.88)
        case "constant":           return RGBColor(r: 0.98, g: 0.96, b: 0.88)
        case "primitive":          return RGBColor(r: 0.92, g: 0.94, b: 0.98)
        case "method_call":        return RGBColor(r: 0.95, g: 0.92, b: 0.98)
        case "instance_generator": return RGBColor(r: 0.98, g: 0.94, b: 0.85)
        case "get":                return RGBColor(r: 0.88, g: 0.96, b: 0.94)
        case "set":                return RGBColor(r: 0.97, g: 0.92, b: 0.88)
        default:                   return RGBColor(r: 0.95, g: 0.95, b: 0.95)
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

    /// Cubic bezier midpoint at t=0.5
    private static func cubicMidpoint(p0: CGPoint, c1: CGPoint, c2: CGPoint, p3: CGPoint) -> CGPoint {
        CGPoint(
            x: p0.x / 8 + 3 * c1.x / 8 + 3 * c2.x / 8 + p3.x / 8,
            y: p0.y / 8 + 3 * c1.y / 8 + 3 * c2.y / 8 + p3.y / 8
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
            guard let src = graph.nodes.first(where: { $0.id == wire.sourceNodeId }) else { continue }
            let sx = src.x + pinX(index: wire.sourcePin, count: src.outputPins.count, nodeWidth: src.width)
            let sy = src.y + src.height

            let ex: CGFloat, ey: CGFloat
            let isDangling: Bool
            if let destId = wire.destNodeId, let destPin = wire.destPin,
               let dst = graph.nodes.first(where: { $0.id == destId }) {
                ex = dst.x + pinX(index: destPin, count: dst.inputPins.count, nodeWidth: dst.width)
                ey = dst.y
                isDangling = false
            } else if let dp = wire.danglingEndpoint {
                ex = dp.x; ey = dp.y
                isDangling = true
            } else {
                continue
            }

            let dy = max(abs(ey - sy) * 0.4, 30)
            let dashAttr = isDangling ? " stroke-dasharray=\"6,4\"" : ""
            let strokeColor = isDangling ? "rgb(255,149,0)" : "rgba(51,51,51,0.55)"
            svg += """
            <path d="M \(f(sx)) \(f(sy)) C \(f(sx)) \(f(sy+dy)) \(f(ex)) \(f(ey-dy)) \(f(ex)) \(f(ey))"
                  fill="none" stroke="\(strokeColor)" stroke-width="1.5"\(dashAttr)/>
            """

            // Dangling endpoint circle
            if isDangling {
                svg += "<circle cx=\"\(f(ex))\" cy=\"\(f(ey))\" r=\"5\" fill=\"rgb(255,149,0)\"/>"
            }

            // Wire name label
            if !wire.name.isEmpty {
                let mid = cubicMidpoint(
                    p0: CGPoint(x: sx, y: sy),
                    c1: CGPoint(x: sx, y: sy + dy),
                    c2: CGPoint(x: ex, y: ey - dy),
                    p3: CGPoint(x: ex, y: ey)
                )
                svg += "<rect x=\"\(f(mid.x - 20))\" y=\"\(f(mid.y - 8))\" width=\"40\" height=\"16\" rx=\"3\" fill=\"white\" stroke=\"#ccc\" stroke-width=\"0.5\"/>"
                svg += "<text x=\"\(f(mid.x))\" y=\"\(f(mid.y))\" text-anchor=\"middle\" dominant-baseline=\"central\" font-size=\"10\" fill=\"#333\">\(escapeXML(wire.name))</text>"
            }
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

    /// Standard paper size in points (72 pt = 1 inch)
    struct PaperSize {
        let width: CGFloat
        let height: CGFloat
        static let letter = PaperSize(width: 612, height: 792)
        static let a4 = PaperSize(width: 595.28, height: 841.89)
    }

    static func exportPDF(graph: GraphModel, to url: URL, paperSize: PaperSize = .letter) throws {
        let bounds = graphBounds(graph)
        let margin: CGFloat = 36
        let overlap: CGFloat = 36
        let printableW = paperSize.width - margin * 2
        let printableH = paperSize.height - margin * 2
        let footerH: CGFloat = 14

        // If graph fits on one page, produce a single page
        let singlePage = bounds.width <= printableW && bounds.height <= (printableH - footerH)

        let tileStepX = printableW - overlap
        let tileStepY = printableH - footerH - overlap
        let cols = singlePage ? 1 : max(1, Int(ceil(bounds.width / tileStepX)))
        let rows = singlePage ? 1 : max(1, Int(ceil(bounds.height / tileStepY)))
        let totalPages = cols * rows

        var mediaBox = CGRect(x: 0, y: 0, width: paperSize.width, height: paperSize.height)
        guard let consumer = CGDataConsumer(url: url as CFURL),
              let ctx = CGContext(consumer: consumer, mediaBox: &mediaBox, nil) else {
            throw NSError(domain: "GraphExporter", code: 1, userInfo: [NSLocalizedDescriptionKey: "Failed to create PDF context"])
        }

        var pageNum = 0
        for row in 0..<rows {
            for col in 0..<cols {
                pageNum += 1
                ctx.beginPDFPage(nil)

                // Tile viewport in graph coordinates
                let tileX = bounds.minX + CGFloat(col) * tileStepX
                let tileY = bounds.minY + CGFloat(row) * tileStepY
                let tileW = singlePage ? bounds.width : printableW
                let tileH = singlePage ? bounds.height : (printableH - footerH)

                // Clip to printable area
                ctx.saveGState()
                ctx.clip(to: CGRect(x: margin, y: margin + footerH, width: printableW, height: tileH))

                // Transform: map tile viewport to printable area, flip Y
                ctx.translateBy(x: margin, y: margin + footerH + tileH)
                ctx.scaleBy(x: 1, y: -1)
                ctx.translateBy(x: -tileX, y: -tileY)

                // Render all graph content (CG clips what's outside)
                renderGraphContent(graph: graph, ctx: ctx)

                ctx.restoreGState()

                // Footer
                let footerAttrs: [NSAttributedString.Key: Any] = [
                    .font: NSFont.systemFont(ofSize: 9),
                    .foregroundColor: NSColor.gray
                ]

                if totalPages > 1 {
                    // Page number centered
                    let pageStr = "Page \(pageNum) of \(totalPages)" as NSString
                    let pageSize = pageStr.size(withAttributes: footerAttrs)
                    pageStr.draw(at: CGPoint(
                        x: (paperSize.width - pageSize.width) / 2,
                        y: margin
                    ), withAttributes: footerAttrs)

                    // Grid reference top-right (e.g., A1, B2)
                    let colLetter = String(UnicodeScalar(65 + col)!) // A, B, C...
                    let gridRef = "\(colLetter)\(row + 1)" as NSString
                    let gridSize = gridRef.size(withAttributes: footerAttrs)
                    gridRef.draw(at: CGPoint(
                        x: paperSize.width - margin - gridSize.width,
                        y: paperSize.height - margin - gridSize.height
                    ), withAttributes: footerAttrs)

                    // Light dashed border around printable area
                    ctx.setStrokeColor(CGColor(red: 0.7, green: 0.7, blue: 0.7, alpha: 0.5))
                    ctx.setLineWidth(0.5)
                    ctx.setLineDash(phase: 0, lengths: [4, 4])
                    ctx.stroke(CGRect(x: margin, y: margin + footerH, width: printableW, height: tileH))
                    ctx.setLineDash(phase: 0, lengths: [])
                }

                ctx.endPDFPage()
            }
        }

        ctx.closePDF()
    }

    /// Renders all wires and nodes into the current CGContext (graph coordinates)
    private static func renderGraphContent(graph: GraphModel, ctx: CGContext) {
        let pinR: CGFloat = 6
        let headerH: CGFloat = 28

        // Wires
        for wire in graph.wires {
            guard let src = graph.nodes.first(where: { $0.id == wire.sourceNodeId }) else { continue }
            let sx = src.x + pinX(index: wire.sourcePin, count: src.outputPins.count, nodeWidth: src.width)
            let sy = src.y + src.height

            let ex: CGFloat, ey: CGFloat
            let isDangling: Bool
            if let destId = wire.destNodeId, let destPin = wire.destPin,
               let dst = graph.nodes.first(where: { $0.id == destId }) {
                ex = dst.x + pinX(index: destPin, count: dst.inputPins.count, nodeWidth: dst.width)
                ey = dst.y
                isDangling = false
            } else if let dp = wire.danglingEndpoint {
                ex = dp.x; ey = dp.y
                isDangling = true
            } else {
                continue
            }

            let dy = max(abs(ey - sy) * 0.4, 30)

            if isDangling {
                ctx.setStrokeColor(CGColor(red: 1.0, green: 0.58, blue: 0.0, alpha: 1.0))
                ctx.setLineDash(phase: 0, lengths: [6, 4])
            } else {
                ctx.setStrokeColor(CGColor(red: 0.2, green: 0.2, blue: 0.2, alpha: 0.55))
                ctx.setLineDash(phase: 0, lengths: [])
            }
            ctx.setLineWidth(1.5)
            ctx.move(to: CGPoint(x: sx, y: sy))
            ctx.addCurve(to: CGPoint(x: ex, y: ey),
                         control1: CGPoint(x: sx, y: sy + dy),
                         control2: CGPoint(x: ex, y: ey - dy))
            ctx.strokePath()
            ctx.setLineDash(phase: 0, lengths: [])

            // Dangling endpoint circle
            if isDangling {
                ctx.setFillColor(CGColor(red: 1.0, green: 0.58, blue: 0.0, alpha: 1.0))
                ctx.fillEllipse(in: CGRect(x: ex - 5, y: ey - 5, width: 10, height: 10))
            }

            // Wire name label
            if !wire.name.isEmpty {
                let mid = cubicMidpoint(
                    p0: CGPoint(x: sx, y: sy),
                    c1: CGPoint(x: sx, y: sy + dy),
                    c2: CGPoint(x: ex, y: ey - dy),
                    p3: CGPoint(x: ex, y: ey)
                )
                let nameAttrs: [NSAttributedString.Key: Any] = [
                    .font: NSFont.systemFont(ofSize: 10),
                    .foregroundColor: NSColor.darkGray
                ]
                let nameStr = wire.name as NSString
                let nameSize = nameStr.size(withAttributes: nameAttrs)
                // White pill background
                let pillRect = CGRect(
                    x: mid.x - nameSize.width / 2 - 3,
                    y: mid.y - nameSize.height / 2 - 1,
                    width: nameSize.width + 6,
                    height: nameSize.height + 2
                )
                ctx.setFillColor(CGColor.white)
                ctx.fill(pillRect)
                ctx.setStrokeColor(CGColor(red: 0.8, green: 0.8, blue: 0.8, alpha: 1))
                ctx.setLineWidth(0.5)
                ctx.stroke(pillRect)
                nameStr.draw(at: CGPoint(x: mid.x - nameSize.width / 2, y: mid.y - nameSize.height / 2), withAttributes: nameAttrs)
            }
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
    }
    #endif
}
