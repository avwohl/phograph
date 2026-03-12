#include "pho_prim.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>

namespace pho {

// Software bitmap: ARGB packed integer pixels
struct PhoBitmap {
    int width;
    int height;
    std::vector<uint32_t> pixels; // ARGB
};

static constexpr int kMaxBitmaps = 256;
static PhoBitmap* gBitmaps[kMaxBitmaps] = {};

static int allocBitmap(int w, int h) {
    for (int i = 0; i < kMaxBitmaps; i++) {
        if (!gBitmaps[i]) {
            gBitmaps[i] = new PhoBitmap{w, h, std::vector<uint32_t>(w * h, 0)};
            return i;
        }
    }
    return -1;
}

static PhoBitmap* getBitmap(int handle) {
    if (handle < 0 || handle >= kMaxBitmaps) return nullptr;
    return gBitmaps[handle];
}

static void setPixelSafe(PhoBitmap* bmp, int x, int y, uint32_t color) {
    if (x >= 0 && x < bmp->width && y >= 0 && y < bmp->height)
        bmp->pixels[y * bmp->width + x] = color;
}

// Bresenham's line algorithm
static void drawLine(PhoBitmap* bmp, int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0), dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while (true) {
        setPixelSafe(bmp, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void register_bitmap_prims() {
    auto& r = PrimitiveRegistry::instance();

    // bitmap-create: width height -> handle
    r.register_prim("bitmap-create", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer())
            return PrimResult::fail_with(Value::error("bitmap-create: expected width and height"));
        int w = (int)in[0].as_integer(), h = (int)in[1].as_integer();
        if (w <= 0 || h <= 0 || w > 8192 || h > 8192)
            return PrimResult::fail_with(Value::error("bitmap-create: dimensions must be 1..8192"));
        int handle = allocBitmap(w, h);
        if (handle < 0) return PrimResult::fail_with(Value::error("bitmap-create: too many bitmaps"));
        return PrimResult::success(Value::integer(handle));
    });

    // bitmap-fill: handle color -> handle
    r.register_prim("bitmap-fill", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer())
            return PrimResult::fail_with(Value::error("bitmap-fill: expected handle and color"));
        auto* bmp = getBitmap((int)in[0].as_integer());
        if (!bmp) return PrimResult::fail_with(Value::error("bitmap-fill: invalid handle"));
        uint32_t color = (uint32_t)in[1].as_integer();
        std::fill(bmp->pixels.begin(), bmp->pixels.end(), color);
        return PrimResult::success(in[0]);
    });

    // bitmap-get-pixel: handle x y -> integer (ARGB)
    r.register_prim("bitmap-get-pixel", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("bitmap-get-pixel: expected handle, x, y"));
        auto* bmp = getBitmap((int)in[0].as_integer());
        if (!bmp) return PrimResult::fail_with(Value::error("bitmap-get-pixel: invalid handle"));
        int x = (int)in[1].as_integer(), y = (int)in[2].as_integer();
        if (x < 0 || x >= bmp->width || y < 0 || y >= bmp->height)
            return PrimResult::fail_with(Value::error("bitmap-get-pixel: out of bounds"));
        return PrimResult::success(Value::integer(bmp->pixels[y * bmp->width + x]));
    });

    // bitmap-set-pixel: handle (x,y list) color -> handle
    r.register_prim("bitmap-set-pixel", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_list() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("bitmap-set-pixel: expected handle, xy list, color"));
        auto* bmp = getBitmap((int)in[0].as_integer());
        if (!bmp) return PrimResult::fail_with(Value::error("bitmap-set-pixel: invalid handle"));
        auto* xy = in[1].as_list();
        if (xy->size() < 2) return PrimResult::fail_with(Value::error("bitmap-set-pixel: need [x,y]"));
        int x = (int)xy->at(0).as_integer(), y = (int)xy->at(1).as_integer();
        if (x < 0 || x >= bmp->width || y < 0 || y >= bmp->height)
            return PrimResult::fail_with(Value::error("bitmap-set-pixel: out of bounds"));
        bmp->pixels[y * bmp->width + x] = (uint32_t)in[2].as_integer();
        return PrimResult::success(in[0]);
    });

    // bitmap-width: handle -> integer
    r.register_prim("bitmap-width", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("bitmap-width: expected handle"));
        auto* bmp = getBitmap((int)in[0].as_integer());
        if (!bmp) return PrimResult::fail_with(Value::error("bitmap-width: invalid handle"));
        return PrimResult::success(Value::integer(bmp->width));
    });

    // bitmap-height: handle -> integer
    r.register_prim("bitmap-height", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("bitmap-height: expected handle"));
        auto* bmp = getBitmap((int)in[0].as_integer());
        if (!bmp) return PrimResult::fail_with(Value::error("bitmap-height: invalid handle"));
        return PrimResult::success(Value::integer(bmp->height));
    });

    // bitblt-copy: dest-handle src-handle rect(x,y,w,h list) -> dest-handle
    r.register_prim("bitblt-copy", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer() || !in[2].is_list())
            return PrimResult::fail_with(Value::error("bitblt-copy: expected dest, src, rect"));
        auto* dst = getBitmap((int)in[0].as_integer());
        auto* src = getBitmap((int)in[1].as_integer());
        if (!dst || !src) return PrimResult::fail_with(Value::error("bitblt-copy: invalid handle"));
        auto* rect = in[2].as_list();
        if (rect->size() < 4) return PrimResult::fail_with(Value::error("bitblt-copy: need [x,y,w,h]"));
        int rx = (int)rect->at(0).as_integer(), ry = (int)rect->at(1).as_integer();
        int rw = (int)rect->at(2).as_integer(), rh = (int)rect->at(3).as_integer();
        for (int y = 0; y < rh; y++) {
            for (int x = 0; x < rw; x++) {
                int sx = rx + x, sy = ry + y;
                if (sx >= 0 && sx < src->width && sy >= 0 && sy < src->height &&
                    x < dst->width && y < dst->height) {
                    dst->pixels[y * dst->width + x] = src->pixels[sy * src->width + sx];
                }
            }
        }
        return PrimResult::success(in[0]);
    });

    // bitblt-blend: dest-handle src-handle pos(x,y list) -> dest-handle
    r.register_prim("bitblt-blend", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer() || !in[2].is_list())
            return PrimResult::fail_with(Value::error("bitblt-blend: expected dest, src, pos"));
        auto* dst = getBitmap((int)in[0].as_integer());
        auto* src = getBitmap((int)in[1].as_integer());
        if (!dst || !src) return PrimResult::fail_with(Value::error("bitblt-blend: invalid handle"));
        auto* pos = in[2].as_list();
        if (pos->size() < 2) return PrimResult::fail_with(Value::error("bitblt-blend: need [x,y]"));
        int ox = (int)pos->at(0).as_integer(), oy = (int)pos->at(1).as_integer();

        for (int y = 0; y < src->height; y++) {
            for (int x = 0; x < src->width; x++) {
                int dx = ox + x, dy = oy + y;
                if (dx < 0 || dx >= dst->width || dy < 0 || dy >= dst->height) continue;
                uint32_t sp = src->pixels[y * src->width + x];
                uint32_t dp = dst->pixels[dy * dst->width + dx];
                uint8_t sa = (sp >> 24) & 0xFF;
                if (sa == 255) { dst->pixels[dy * dst->width + dx] = sp; continue; }
                if (sa == 0) continue;
                uint8_t sr = (sp >> 16) & 0xFF, sg = (sp >> 8) & 0xFF, sb = sp & 0xFF;
                uint8_t dr = (dp >> 16) & 0xFF, dg = (dp >> 8) & 0xFF, db = dp & 0xFF;
                uint8_t da = (dp >> 24) & 0xFF;
                uint8_t rr = (sr * sa + dr * (255 - sa)) / 255;
                uint8_t rg = (sg * sa + dg * (255 - sa)) / 255;
                uint8_t rb = (sb * sa + db * (255 - sa)) / 255;
                uint8_t ra = sa + (da * (255 - sa)) / 255;
                dst->pixels[dy * dst->width + dx] = ((uint32_t)ra << 24) | ((uint32_t)rr << 16) | ((uint32_t)rg << 8) | rb;
            }
        }
        return PrimResult::success(in[0]);
    });

    // b2d-draw-line: handle start(x,y) end(x,y) -> handle
    r.register_prim("b2d-draw-line", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_list() || !in[2].is_list())
            return PrimResult::fail_with(Value::error("b2d-draw-line: expected handle, start, end"));
        auto* bmp = getBitmap((int)in[0].as_integer());
        if (!bmp) return PrimResult::fail_with(Value::error("b2d-draw-line: invalid handle"));
        auto* start = in[1].as_list();
        auto* end = in[2].as_list();
        if (start->size() < 2 || end->size() < 2)
            return PrimResult::fail_with(Value::error("b2d-draw-line: need [x,y] lists"));
        drawLine(bmp,
            (int)start->at(0).as_integer(), (int)start->at(1).as_integer(),
            (int)end->at(0).as_integer(), (int)end->at(1).as_integer(),
            0xFFFFFFFF); // white default
        return PrimResult::success(in[0]);
    });

    // b2d-draw-rect: handle origin(x,y) size(w,h) -> handle
    r.register_prim("b2d-draw-rect", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_list() || !in[2].is_list())
            return PrimResult::fail_with(Value::error("b2d-draw-rect: expected handle, origin, size"));
        auto* bmp = getBitmap((int)in[0].as_integer());
        if (!bmp) return PrimResult::fail_with(Value::error("b2d-draw-rect: invalid handle"));
        auto* origin = in[1].as_list();
        auto* size = in[2].as_list();
        int x = (int)origin->at(0).as_integer(), y = (int)origin->at(1).as_integer();
        int w = (int)size->at(0).as_integer(), h = (int)size->at(1).as_integer();
        uint32_t color = 0xFFFFFFFF;
        drawLine(bmp, x, y, x+w-1, y, color);
        drawLine(bmp, x+w-1, y, x+w-1, y+h-1, color);
        drawLine(bmp, x+w-1, y+h-1, x, y+h-1, color);
        drawLine(bmp, x, y+h-1, x, y, color);
        return PrimResult::success(in[0]);
    });

    // b2d-fill-rect: handle rect(x,y,w,h) color -> handle
    r.register_prim("b2d-fill-rect", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_list() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("b2d-fill-rect: expected handle, rect, color"));
        auto* bmp = getBitmap((int)in[0].as_integer());
        if (!bmp) return PrimResult::fail_with(Value::error("b2d-fill-rect: invalid handle"));
        auto* rect = in[1].as_list();
        if (rect->size() < 4) return PrimResult::fail_with(Value::error("b2d-fill-rect: need [x,y,w,h]"));
        int rx = (int)rect->at(0).as_integer(), ry = (int)rect->at(1).as_integer();
        int rw = (int)rect->at(2).as_integer(), rh = (int)rect->at(3).as_integer();
        uint32_t color = (uint32_t)in[2].as_integer();
        for (int y = ry; y < ry + rh; y++)
            for (int x = rx; x < rx + rw; x++)
                setPixelSafe(bmp, x, y, color);
        return PrimResult::success(in[0]);
    });

    // b2d-draw-oval: handle rect(x,y,w,h) color -> handle
    r.register_prim("b2d-draw-oval", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_list() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("b2d-draw-oval: expected handle, rect, color"));
        auto* bmp = getBitmap((int)in[0].as_integer());
        if (!bmp) return PrimResult::fail_with(Value::error("b2d-draw-oval: invalid handle"));
        auto* rect = in[1].as_list();
        if (rect->size() < 4) return PrimResult::fail_with(Value::error("b2d-draw-oval: need [x,y,w,h]"));
        int rx = (int)rect->at(0).as_integer(), ry = (int)rect->at(1).as_integer();
        int rw = (int)rect->at(2).as_integer(), rh = (int)rect->at(3).as_integer();
        uint32_t color = (uint32_t)in[2].as_integer();
        double cx = rx + rw / 2.0, cy = ry + rh / 2.0;
        double a = rw / 2.0, b = rh / 2.0;
        int steps = std::max(rw, rh) * 4;
        for (int i = 0; i < steps; i++) {
            double angle = 2.0 * M_PI * i / steps;
            int x = (int)(cx + a * cos(angle));
            int y = (int)(cy + b * sin(angle));
            setPixelSafe(bmp, x, y, color);
        }
        return PrimResult::success(in[0]);
    });

    // b2d-fill-oval: handle rect(x,y,w,h) color -> handle
    r.register_prim("b2d-fill-oval", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_list() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("b2d-fill-oval: expected handle, rect, color"));
        auto* bmp = getBitmap((int)in[0].as_integer());
        if (!bmp) return PrimResult::fail_with(Value::error("b2d-fill-oval: invalid handle"));
        auto* rect = in[1].as_list();
        if (rect->size() < 4) return PrimResult::fail_with(Value::error("b2d-fill-oval: need [x,y,w,h]"));
        int rx = (int)rect->at(0).as_integer(), ry = (int)rect->at(1).as_integer();
        int rw = (int)rect->at(2).as_integer(), rh = (int)rect->at(3).as_integer();
        uint32_t color = (uint32_t)in[2].as_integer();
        double cx = rx + rw / 2.0, cy = ry + rh / 2.0;
        double a = rw / 2.0, b = rh / 2.0;
        for (int y = ry; y < ry + rh; y++) {
            double dy = (y - cy) / b;
            if (dy * dy > 1.0) continue;
            double dx = a * sqrt(1.0 - dy * dy);
            int x0 = (int)(cx - dx), x1 = (int)(cx + dx);
            for (int x = x0; x <= x1; x++)
                setPixelSafe(bmp, x, y, color);
        }
        return PrimResult::success(in[0]);
    });

    // b2d-draw-polygon: handle points-list -> handle
    r.register_prim("b2d-draw-polygon", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_list())
            return PrimResult::fail_with(Value::error("b2d-draw-polygon: expected handle and points list"));
        auto* bmp = getBitmap((int)in[0].as_integer());
        if (!bmp) return PrimResult::fail_with(Value::error("b2d-draw-polygon: invalid handle"));
        auto* pts = in[1].as_list();
        if (pts->size() < 2) return PrimResult::success(in[0]);
        uint32_t color = 0xFFFFFFFF;
        for (size_t i = 0; i < pts->size(); i++) {
            auto* p0 = pts->at(i).as_list();
            auto* p1 = pts->at((i + 1) % pts->size()).as_list();
            if (p0 && p1 && p0->size() >= 2 && p1->size() >= 2) {
                drawLine(bmp,
                    (int)p0->at(0).as_integer(), (int)p0->at(1).as_integer(),
                    (int)p1->at(0).as_integer(), (int)p1->at(1).as_integer(),
                    color);
            }
        }
        return PrimResult::success(in[0]);
    });

    // b2d-draw-bezier: handle control-points(4 x,y lists) color -> handle
    r.register_prim("b2d-draw-bezier", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_list() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("b2d-draw-bezier: expected handle, 4 control points, color"));
        auto* bmp = getBitmap((int)in[0].as_integer());
        if (!bmp) return PrimResult::fail_with(Value::error("b2d-draw-bezier: invalid handle"));
        auto* cps = in[1].as_list();
        if (cps->size() < 4) return PrimResult::fail_with(Value::error("b2d-draw-bezier: need 4 control points"));
        uint32_t color = (uint32_t)in[2].as_integer();

        double px[4], py[4];
        for (int i = 0; i < 4; i++) {
            auto* pt = cps->at(i).as_list();
            if (!pt || pt->size() < 2) return PrimResult::fail_with(Value::error("b2d-draw-bezier: invalid point"));
            px[i] = pt->at(0).as_number();
            py[i] = pt->at(1).as_number();
        }

        int steps = 100;
        int lastX = (int)px[0], lastY = (int)py[0];
        for (int i = 1; i <= steps; i++) {
            double t = (double)i / steps;
            double u = 1.0 - t;
            double x = u*u*u*px[0] + 3*u*u*t*px[1] + 3*u*t*t*px[2] + t*t*t*px[3];
            double y = u*u*u*py[0] + 3*u*u*t*py[1] + 3*u*t*t*py[2] + t*t*t*py[3];
            drawLine(bmp, lastX, lastY, (int)x, (int)y, color);
            lastX = (int)x;
            lastY = (int)y;
        }
        return PrimResult::success(in[0]);
    });

    // color-rgb: r g b -> integer (ARGB with full alpha)
    r.register_prim("color-rgb", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("color-rgb: expected 3 integers"));
        uint32_t color = 0xFF000000
            | (((uint32_t)in[0].as_integer() & 0xFF) << 16)
            | (((uint32_t)in[1].as_integer() & 0xFF) << 8)
            | ((uint32_t)in[2].as_integer() & 0xFF);
        return PrimResult::success(Value::integer(color));
    });

    // color-rgba: rgb-list alpha -> integer (ARGB)
    r.register_prim("color-rgba", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_list() || !in[1].is_integer())
            return PrimResult::fail_with(Value::error("color-rgba: expected [r,g,b] list and alpha"));
        auto* rgb = in[0].as_list();
        if (rgb->size() < 3) return PrimResult::fail_with(Value::error("color-rgba: need [r,g,b]"));
        uint32_t color = (((uint32_t)in[1].as_integer() & 0xFF) << 24)
            | (((uint32_t)rgb->at(0).as_integer() & 0xFF) << 16)
            | (((uint32_t)rgb->at(1).as_integer() & 0xFF) << 8)
            | ((uint32_t)rgb->at(2).as_integer() & 0xFF);
        return PrimResult::success(Value::integer(color));
    });

    // float-array-add: list-a list-b -> list
    r.register_prim("float-array-add", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_list() || !in[1].is_list())
            return PrimResult::fail_with(Value::error("float-array-add: expected two lists"));
        auto* a = in[0].as_list();
        auto* b = in[1].as_list();
        size_t len = std::min(a->size(), b->size());
        std::vector<Value> result;
        result.reserve(len);
        for (size_t i = 0; i < len; i++)
            result.push_back(Value::real(a->at(i).as_number() + b->at(i).as_number()));
        return PrimResult::success(Value::list(std::move(result)));
    });

    // float-array-mul: list-a list-b -> list
    r.register_prim("float-array-mul", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_list() || !in[1].is_list())
            return PrimResult::fail_with(Value::error("float-array-mul: expected two lists"));
        auto* a = in[0].as_list();
        auto* b = in[1].as_list();
        size_t len = std::min(a->size(), b->size());
        std::vector<Value> result;
        result.reserve(len);
        for (size_t i = 0; i < len; i++)
            result.push_back(Value::real(a->at(i).as_number() * b->at(i).as_number()));
        return PrimResult::success(Value::list(std::move(result)));
    });

    // float-array-scale: list scalar -> list
    r.register_prim("float-array-scale", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_list() || !in[1].is_numeric())
            return PrimResult::fail_with(Value::error("float-array-scale: expected list and number"));
        auto* a = in[0].as_list();
        double s = in[1].as_number();
        std::vector<Value> result;
        result.reserve(a->size());
        for (size_t i = 0; i < a->size(); i++)
            result.push_back(Value::real(a->at(i).as_number() * s));
        return PrimResult::success(Value::list(std::move(result)));
    });

    // float-array-sum: list -> number
    r.register_prim("float-array-sum", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_list()) return PrimResult::fail_with(Value::error("float-array-sum: expected list"));
        auto* a = in[0].as_list();
        double sum = 0.0;
        for (size_t i = 0; i < a->size(); i++) sum += a->at(i).as_number();
        return PrimResult::success(Value::real(sum));
    });
}

} // namespace pho
