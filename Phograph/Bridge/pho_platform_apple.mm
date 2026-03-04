#import <Foundation/Foundation.h>
#include "pho_platform.h"
#include <cstdlib>
#include <cstring>
#include <dispatch/dispatch.h>

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

extern "C" {

char* pho_platform_read_file(const char* path, size_t* out_size) {
    @autoreleasepool {
        NSString *nsPath = [NSString stringWithUTF8String:path];
        NSData *data = [NSData dataWithContentsOfFile:nsPath];
        if (!data) return nullptr;

        size_t len = [data length];
        char* buf = (char*)malloc(len + 1);
        memcpy(buf, [data bytes], len);
        buf[len] = '\0';
        if (out_size) *out_size = len;
        return buf;
    }
}

int pho_platform_write_file(const char* path, const void* data, size_t size) {
    @autoreleasepool {
        NSString *nsPath = [NSString stringWithUTF8String:path];
        NSData *nsData = [NSData dataWithBytes:data length:size];
        return [nsData writeToFile:nsPath atomically:YES] ? 0 : -1;
    }
}

int pho_platform_file_exists(const char* path) {
    @autoreleasepool {
        NSString *nsPath = [NSString stringWithUTF8String:path];
        return [[NSFileManager defaultManager] fileExistsAtPath:nsPath] ? 1 : 0;
    }
}

void pho_platform_free(void* ptr) {
    free(ptr);
}

double pho_platform_time_now(void) {
    return [[NSDate date] timeIntervalSince1970];
}

uint64_t pho_platform_timer_after(double delay_seconds, void (*callback)(void* ctx), void* ctx) {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay_seconds * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
        callback(ctx);
    });
    return 0; // simplified - full implementation would track timer ids
}

void pho_platform_timer_cancel(uint64_t timer_id) {
    (void)timer_id;
    // TODO: implement with dispatch source
}

void pho_platform_measure_text(const char* text, const char* font_name,
                                float font_size, float* out_width, float* out_height) {
    @autoreleasepool {
        NSString *nsText = [NSString stringWithUTF8String:text];
        NSString *nsFontName = font_name ? [NSString stringWithUTF8String:font_name] : @"Helvetica";

#if TARGET_OS_IPHONE
        UIFont *font = [UIFont fontWithName:nsFontName size:font_size] ?: [UIFont systemFontOfSize:font_size];
        NSDictionary *attrs = @{NSFontAttributeName: font};
#else
        NSFont *font = [NSFont fontWithName:nsFontName size:font_size] ?: [NSFont systemFontOfSize:font_size];
        NSDictionary *attrs = @{NSFontAttributeName: font};
#endif
        CGSize size = [nsText sizeWithAttributes:attrs];
        if (out_width) *out_width = (float)size.width;
        if (out_height) *out_height = (float)size.height;
    }
}

void pho_platform_log(const char* message) {
    NSLog(@"[pho] %s", message);
}

char* pho_platform_clipboard_get(void) {
    @autoreleasepool {
#if TARGET_OS_IPHONE
        NSString *str = [[UIPasteboard generalPasteboard] string];
#else
        NSString *str = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
#endif
        if (!str) return nullptr;
        const char* cstr = [str UTF8String];
        size_t len = strlen(cstr);
        char* buf = (char*)malloc(len + 1);
        memcpy(buf, cstr, len + 1);
        return buf;
    }
}

void pho_platform_clipboard_set(const char* text) {
    @autoreleasepool {
        NSString *str = [NSString stringWithUTF8String:text];
#if TARGET_OS_IPHONE
        [[UIPasteboard generalPasteboard] setString:str];
#else
        [[NSPasteboard generalPasteboard] clearContents];
        [[NSPasteboard generalPasteboard] setString:str forType:NSPasteboardTypeString];
#endif
    }
}

const char* pho_platform_name(void) {
#if TARGET_OS_IPHONE
    return "iOS";
#else
    return "macOS";
#endif
}

double pho_platform_screen_scale(void) {
#if TARGET_OS_IPHONE
    return (double)[[UIScreen mainScreen] scale];
#else
    return (double)[[NSScreen mainScreen] backingScaleFactor];
#endif
}

} // extern "C"
