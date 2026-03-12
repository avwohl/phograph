#include "pho_prim.h"
#include <cstring>
#include <vector>

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>

// Image handle: wraps a CGImageRef stored as External value
// We use PhoObject with class "Image" and attrs for width, height, and a raw pointer

struct PhoImage {
    CGImageRef cgImage;
    int width;
    int height;
};

static constexpr int kMaxImages = 256;
static PhoImage gImages[kMaxImages] = {};

static int allocImage(CGImageRef img) {
    for (int i = 0; i < kMaxImages; i++) {
        if (!gImages[i].cgImage) {
            gImages[i].cgImage = CGImageRetain(img);
            gImages[i].width = (int)CGImageGetWidth(img);
            gImages[i].height = (int)CGImageGetHeight(img);
            return i;
        }
    }
    return -1;
}

static PhoImage* getImage(int handle) {
    if (handle < 0 || handle >= kMaxImages) return nullptr;
    if (!gImages[handle].cgImage) return nullptr;
    return &gImages[handle];
}

// Create CGImage from RGBA pixel data
static CGImageRef createCGImage(const uint8_t* pixels, int w, int h) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        (void*)pixels, w, h, 8, w * 4, cs,
        kCGImageAlphaPremultipliedLast);
    CGImageRef img = CGBitmapContextCreateImage(ctx);
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
    return img;
}

// Get RGBA pixels from CGImage
static std::vector<uint8_t> getPixels(CGImageRef img) {
    int w = (int)CGImageGetWidth(img);
    int h = (int)CGImageGetHeight(img);
    std::vector<uint8_t> pixels(w * h * 4);
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(
        pixels.data(), w, h, 8, w * 4, cs,
        kCGImageAlphaPremultipliedLast);
    CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), img);
    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
    return pixels;
}
#endif

namespace pho {

void register_image_prims() {
    auto& r = PrimitiveRegistry::instance();

#ifdef __APPLE__
    // image-load: path -> handle
    r.register_prim("image-load", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_string()) return PrimResult::fail_with(Value::error("image-load: expected string path"));
        CFStringRef path = CFStringCreateWithCString(nullptr, in[0].as_string()->c_str(), kCFStringEncodingUTF8);
        CFURLRef url = CFURLCreateWithFileSystemPath(nullptr, path, kCFURLPOSIXPathStyle, false);
        CFRelease(path);
        CGImageSourceRef source = CGImageSourceCreateWithURL(url, nullptr);
        CFRelease(url);
        if (!source) return PrimResult::fail_with(Value::error("image-load: could not load image"));
        CGImageRef img = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
        CFRelease(source);
        if (!img) return PrimResult::fail_with(Value::error("image-load: could not decode image"));
        int handle = allocImage(img);
        CGImageRelease(img);
        if (handle < 0) return PrimResult::fail_with(Value::error("image-load: too many images"));
        return PrimResult::success(Value::integer(handle));
    });

    // image-save-jpeg: handle path quality -> boolean
    r.register_prim("image-save-jpeg", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_string() || !in[2].is_numeric())
            return PrimResult::fail_with(Value::error("image-save-jpeg: expected handle, path, quality"));
        auto* img = getImage((int)in[0].as_integer());
        if (!img) return PrimResult::fail_with(Value::error("image-save-jpeg: invalid handle"));

        CFStringRef path = CFStringCreateWithCString(nullptr, in[1].as_string()->c_str(), kCFStringEncodingUTF8);
        CFURLRef url = CFURLCreateWithFileSystemPath(nullptr, path, kCFURLPOSIXPathStyle, false);
        CFRelease(path);
        CGImageDestinationRef dest = CGImageDestinationCreateWithURL(url, CFSTR("public.jpeg"), 1, nullptr);
        CFRelease(url);
        if (!dest) return PrimResult::fail_with(Value::error("image-save-jpeg: could not create destination"));

        float quality = (float)(in[2].as_number() / 100.0);
        CFNumberRef qualNum = CFNumberCreate(nullptr, kCFNumberFloatType, &quality);
        CFStringRef keys[] = { kCGImageDestinationLossyCompressionQuality };
        CFTypeRef vals[] = { qualNum };
        CFDictionaryRef opts = CFDictionaryCreate(nullptr, (const void**)keys, (const void**)vals, 1,
                                                   &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CGImageDestinationAddImage(dest, img->cgImage, opts);
        bool ok = CGImageDestinationFinalize(dest);
        CFRelease(opts);
        CFRelease(qualNum);
        CFRelease(dest);
        return PrimResult::success(Value::boolean(ok));
    });

    // image-save-png: handle path -> boolean
    r.register_prim("image-save-png", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_string())
            return PrimResult::fail_with(Value::error("image-save-png: expected handle and path"));
        auto* img = getImage((int)in[0].as_integer());
        if (!img) return PrimResult::fail_with(Value::error("image-save-png: invalid handle"));

        CFStringRef path = CFStringCreateWithCString(nullptr, in[1].as_string()->c_str(), kCFStringEncodingUTF8);
        CFURLRef url = CFURLCreateWithFileSystemPath(nullptr, path, kCFURLPOSIXPathStyle, false);
        CFRelease(path);
        CGImageDestinationRef dest = CGImageDestinationCreateWithURL(url, CFSTR("public.png"), 1, nullptr);
        CFRelease(url);
        if (!dest) return PrimResult::fail_with(Value::error("image-save-png: could not create destination"));
        CGImageDestinationAddImage(dest, img->cgImage, nullptr);
        bool ok = CGImageDestinationFinalize(dest);
        CFRelease(dest);
        return PrimResult::success(Value::boolean(ok));
    });

    // image-width: handle -> integer
    r.register_prim("image-width", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("image-width: expected handle"));
        auto* img = getImage((int)in[0].as_integer());
        if (!img) return PrimResult::fail_with(Value::error("image-width: invalid handle"));
        return PrimResult::success(Value::integer(img->width));
    });

    // image-height: handle -> integer
    r.register_prim("image-height", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("image-height: expected handle"));
        auto* img = getImage((int)in[0].as_integer());
        if (!img) return PrimResult::fail_with(Value::error("image-height: invalid handle"));
        return PrimResult::success(Value::integer(img->height));
    });

    // image-resize: handle new-width new-height -> handle
    r.register_prim("image-resize", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("image-resize: expected handle, width, height"));
        auto* img = getImage((int)in[0].as_integer());
        if (!img) return PrimResult::fail_with(Value::error("image-resize: invalid handle"));

        int nw = (int)in[1].as_integer(), nh = (int)in[2].as_integer();
        CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
        CGContextRef ctx = CGBitmapContextCreate(nullptr, nw, nh, 8, nw * 4, cs, kCGImageAlphaPremultipliedLast);
        CGColorSpaceRelease(cs);
        CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
        CGContextDrawImage(ctx, CGRectMake(0, 0, nw, nh), img->cgImage);
        CGImageRef newImg = CGBitmapContextCreateImage(ctx);
        CGContextRelease(ctx);

        int handle = allocImage(newImg);
        CGImageRelease(newImg);
        if (handle < 0) return PrimResult::fail_with(Value::error("image-resize: too many images"));
        return PrimResult::success(Value::integer(handle));
    });

    // image-crop: handle origin(x,y list) size(w,h list) -> handle
    r.register_prim("image-crop", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_list() || !in[2].is_list())
            return PrimResult::fail_with(Value::error("image-crop: expected handle, origin list, size list"));
        auto* img = getImage((int)in[0].as_integer());
        if (!img) return PrimResult::fail_with(Value::error("image-crop: invalid handle"));
        auto* origin = in[1].as_list();
        auto* size = in[2].as_list();
        if (origin->size() < 2 || size->size() < 2)
            return PrimResult::fail_with(Value::error("image-crop: lists must have 2 elements"));

        CGRect rect = CGRectMake(
            origin->at(0).as_number(), origin->at(1).as_number(),
            size->at(0).as_number(), size->at(1).as_number());
        CGImageRef cropped = CGImageCreateWithImageInRect(img->cgImage, rect);
        if (!cropped) return PrimResult::fail_with(Value::error("image-crop: failed"));
        int handle = allocImage(cropped);
        CGImageRelease(cropped);
        if (handle < 0) return PrimResult::fail_with(Value::error("image-crop: too many images"));
        return PrimResult::success(Value::integer(handle));
    });

    // image-get-pixel: handle x y -> list [r, g, b, a]
    r.register_prim("image-get-pixel", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer() || !in[2].is_integer())
            return PrimResult::fail_with(Value::error("image-get-pixel: expected handle, x, y"));
        auto* img = getImage((int)in[0].as_integer());
        if (!img) return PrimResult::fail_with(Value::error("image-get-pixel: invalid handle"));
        int x = (int)in[1].as_integer(), y = (int)in[2].as_integer();
        if (x < 0 || x >= img->width || y < 0 || y >= img->height)
            return PrimResult::fail_with(Value::error("image-get-pixel: out of bounds"));

        auto pixels = getPixels(img->cgImage);
        int idx = (y * img->width + x) * 4;
        std::vector<Value> rgba = {
            Value::integer(pixels[idx]), Value::integer(pixels[idx+1]),
            Value::integer(pixels[idx+2]), Value::integer(pixels[idx+3])
        };
        return PrimResult::success(Value::list(std::move(rgba)));
    });

    // image-create: width height color(RGBA list) -> handle
    r.register_prim("image-create", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_integer() || !in[2].is_list())
            return PrimResult::fail_with(Value::error("image-create: expected width, height, color list"));
        int w = (int)in[0].as_integer(), h = (int)in[1].as_integer();
        auto* color = in[2].as_list();
        if (color->size() < 4) return PrimResult::fail_with(Value::error("image-create: color needs [r,g,b,a]"));

        uint8_t r_val = (uint8_t)color->at(0).as_integer();
        uint8_t g_val = (uint8_t)color->at(1).as_integer();
        uint8_t b_val = (uint8_t)color->at(2).as_integer();
        uint8_t a_val = (uint8_t)color->at(3).as_integer();

        std::vector<uint8_t> pixels(w * h * 4);
        for (int i = 0; i < w * h; i++) {
            pixels[i*4] = r_val; pixels[i*4+1] = g_val;
            pixels[i*4+2] = b_val; pixels[i*4+3] = a_val;
        }
        CGImageRef cgImg = createCGImage(pixels.data(), w, h);
        int handle = allocImage(cgImg);
        CGImageRelease(cgImg);
        if (handle < 0) return PrimResult::fail_with(Value::error("image-create: too many images"));
        return PrimResult::success(Value::integer(handle));
    });

    // image-flip-h: handle -> handle
    r.register_prim("image-flip-h", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("image-flip-h: expected handle"));
        auto* img = getImage((int)in[0].as_integer());
        if (!img) return PrimResult::fail_with(Value::error("image-flip-h: invalid handle"));

        auto pixels = getPixels(img->cgImage);
        int w = img->width, h = img->height;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w / 2; x++) {
                int l = (y * w + x) * 4;
                int r_idx = (y * w + (w - 1 - x)) * 4;
                for (int c = 0; c < 4; c++) std::swap(pixels[l+c], pixels[r_idx+c]);
            }
        }
        CGImageRef newImg = createCGImage(pixels.data(), w, h);
        int handle = allocImage(newImg);
        CGImageRelease(newImg);
        if (handle < 0) return PrimResult::fail_with(Value::error("image-flip-h: too many images"));
        return PrimResult::success(Value::integer(handle));
    });

    // image-flip-v: handle -> handle
    r.register_prim("image-flip-v", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("image-flip-v: expected handle"));
        auto* img = getImage((int)in[0].as_integer());
        if (!img) return PrimResult::fail_with(Value::error("image-flip-v: invalid handle"));

        auto pixels = getPixels(img->cgImage);
        int w = img->width, h = img->height;
        for (int y = 0; y < h / 2; y++) {
            int top = y * w * 4;
            int bot = (h - 1 - y) * w * 4;
            for (int i = 0; i < w * 4; i++) std::swap(pixels[top+i], pixels[bot+i]);
        }
        CGImageRef newImg = createCGImage(pixels.data(), w, h);
        int handle = allocImage(newImg);
        CGImageRelease(newImg);
        if (handle < 0) return PrimResult::fail_with(Value::error("image-flip-v: too many images"));
        return PrimResult::success(Value::integer(handle));
    });

    // image-grayscale: handle -> handle
    r.register_prim("image-grayscale", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer()) return PrimResult::fail_with(Value::error("image-grayscale: expected handle"));
        auto* img = getImage((int)in[0].as_integer());
        if (!img) return PrimResult::fail_with(Value::error("image-grayscale: invalid handle"));

        auto pixels = getPixels(img->cgImage);
        int w = img->width, h = img->height;
        for (int i = 0; i < w * h; i++) {
            int idx = i * 4;
            uint8_t gray = (uint8_t)(0.299 * pixels[idx] + 0.587 * pixels[idx+1] + 0.114 * pixels[idx+2]);
            pixels[idx] = pixels[idx+1] = pixels[idx+2] = gray;
        }
        CGImageRef newImg = createCGImage(pixels.data(), w, h);
        int handle = allocImage(newImg);
        CGImageRelease(newImg);
        if (handle < 0) return PrimResult::fail_with(Value::error("image-grayscale: too many images"));
        return PrimResult::success(Value::integer(handle));
    });

    // Stubs for decode/encode (in-memory JPEG operations)
    r.register_prim("image-decode-jpeg", 1, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_data()) return PrimResult::fail_with(Value::error("image-decode-jpeg: expected data"));
        auto* d = in[0].as_data();
        CFDataRef cfData = CFDataCreate(nullptr, d->bytes().data(), d->length());
        CGImageSourceRef source = CGImageSourceCreateWithData(cfData, nullptr);
        CFRelease(cfData);
        if (!source) return PrimResult::fail_with(Value::error("image-decode-jpeg: decode failed"));
        CGImageRef img = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
        CFRelease(source);
        if (!img) return PrimResult::fail_with(Value::error("image-decode-jpeg: decode failed"));
        int handle = allocImage(img);
        CGImageRelease(img);
        if (handle < 0) return PrimResult::fail_with(Value::error("image-decode-jpeg: too many images"));
        return PrimResult::success(Value::integer(handle));
    });

    r.register_prim("image-encode-jpeg", 2, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_numeric())
            return PrimResult::fail_with(Value::error("image-encode-jpeg: expected handle and quality"));
        auto* img = getImage((int)in[0].as_integer());
        if (!img) return PrimResult::fail_with(Value::error("image-encode-jpeg: invalid handle"));

        CFMutableDataRef cfData = CFDataCreateMutable(nullptr, 0);
        CGImageDestinationRef dest = CGImageDestinationCreateWithData(cfData, CFSTR("public.jpeg"), 1, nullptr);
        float quality = (float)(in[1].as_number() / 100.0);
        CFNumberRef qualNum = CFNumberCreate(nullptr, kCFNumberFloatType, &quality);
        CFStringRef keys[] = { kCGImageDestinationLossyCompressionQuality };
        CFTypeRef vals[] = { qualNum };
        CFDictionaryRef opts = CFDictionaryCreate(nullptr, (const void**)keys, (const void**)vals, 1,
                                                   &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CGImageDestinationAddImage(dest, img->cgImage, opts);
        CGImageDestinationFinalize(dest);
        CFRelease(opts);
        CFRelease(qualNum);
        CFRelease(dest);

        const UInt8* bytes = CFDataGetBytePtr(cfData);
        CFIndex len = CFDataGetLength(cfData);
        std::vector<uint8_t> result(bytes, bytes + len);
        CFRelease(cfData);
        return PrimResult::success(Value::data(make_ref<PhoData>(std::move(result))));
    });

    // image-set-pixel: handle (x,y list) color(RGBA list) -> handle
    r.register_prim("image-set-pixel", 3, 1, [](const std::vector<Value>& in) -> PrimResult {
        if (!in[0].is_integer() || !in[1].is_list() || !in[2].is_list())
            return PrimResult::fail_with(Value::error("image-set-pixel: expected handle, xy list, color list"));
        auto* img = getImage((int)in[0].as_integer());
        if (!img) return PrimResult::fail_with(Value::error("image-set-pixel: invalid handle"));
        auto* xy = in[1].as_list();
        auto* color = in[2].as_list();
        if (xy->size() < 2 || color->size() < 4)
            return PrimResult::fail_with(Value::error("image-set-pixel: need [x,y] and [r,g,b,a]"));

        int x = (int)xy->at(0).as_integer(), y = (int)xy->at(1).as_integer();
        if (x < 0 || x >= img->width || y < 0 || y >= img->height)
            return PrimResult::fail_with(Value::error("image-set-pixel: out of bounds"));

        auto pixels = getPixels(img->cgImage);
        int idx = (y * img->width + x) * 4;
        pixels[idx] = (uint8_t)color->at(0).as_integer();
        pixels[idx+1] = (uint8_t)color->at(1).as_integer();
        pixels[idx+2] = (uint8_t)color->at(2).as_integer();
        pixels[idx+3] = (uint8_t)color->at(3).as_integer();

        CGImageRef newImg = createCGImage(pixels.data(), img->width, img->height);
        int handle = allocImage(newImg);
        CGImageRelease(newImg);
        if (handle < 0) return PrimResult::fail_with(Value::error("image-set-pixel: too many images"));
        return PrimResult::success(Value::integer(handle));
    });

#else
    // Non-Apple stubs
    auto stub = [](const char* name) {
        return [name](const std::vector<Value>&) -> PrimResult {
            return PrimResult::fail_with(Value::error(std::string(name) + ": not available on this platform"));
        };
    };
    r.register_prim("image-load", 1, 1, stub("image-load"));
    r.register_prim("image-save-jpeg", 3, 1, stub("image-save-jpeg"));
    r.register_prim("image-save-png", 2, 1, stub("image-save-png"));
    r.register_prim("image-width", 1, 1, stub("image-width"));
    r.register_prim("image-height", 1, 1, stub("image-height"));
    r.register_prim("image-resize", 3, 1, stub("image-resize"));
    r.register_prim("image-crop", 3, 1, stub("image-crop"));
    r.register_prim("image-get-pixel", 3, 1, stub("image-get-pixel"));
    r.register_prim("image-set-pixel", 3, 1, stub("image-set-pixel"));
    r.register_prim("image-create", 3, 1, stub("image-create"));
    r.register_prim("image-flip-h", 1, 1, stub("image-flip-h"));
    r.register_prim("image-flip-v", 1, 1, stub("image-flip-v"));
    r.register_prim("image-grayscale", 1, 1, stub("image-grayscale"));
    r.register_prim("image-decode-jpeg", 1, 1, stub("image-decode-jpeg"));
    r.register_prim("image-encode-jpeg", 2, 1, stub("image-encode-jpeg"));
#endif
}

} // namespace pho
