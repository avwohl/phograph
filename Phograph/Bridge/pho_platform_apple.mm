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

// ---- HTTP Networking (Phase 24) ----

int pho_platform_http_get_c(const char* url, char** out_body, size_t* out_len) {
    @autoreleasepool {
        *out_body = nullptr;
        *out_len = 0;
        NSURL *nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
        if (!nsUrl) return -1;

        __block NSData *responseData = nil;
        __block NSHTTPURLResponse *httpResponse = nil;
        __block NSError *requestError = nil;
        dispatch_semaphore_t sem = dispatch_semaphore_create(0);

        NSURLSessionDataTask *task = [[NSURLSession sharedSession]
            dataTaskWithURL:nsUrl
            completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
                responseData = data;
                httpResponse = (NSHTTPURLResponse *)response;
                requestError = error;
                dispatch_semaphore_signal(sem);
            }];
        [task resume];
        dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 30 * NSEC_PER_SEC));

        if (requestError || !responseData) return -1;

        size_t len = [responseData length];
        char* buf = (char*)malloc(len + 1);
        memcpy(buf, [responseData bytes], len);
        buf[len] = '\0';
        *out_body = buf;
        *out_len = len;
        return (int)[httpResponse statusCode];
    }
}

int pho_platform_http_post_c(const char* url, const char* body, size_t body_len,
                              const char* content_type, char** out_body, size_t* out_len) {
    @autoreleasepool {
        *out_body = nullptr;
        *out_len = 0;
        NSURL *nsUrl = [NSURL URLWithString:[NSString stringWithUTF8String:url]];
        if (!nsUrl) return -1;

        NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:nsUrl];
        [request setHTTPMethod:@"POST"];
        [request setHTTPBody:[NSData dataWithBytes:body length:body_len]];
        if (content_type) {
            [request setValue:[NSString stringWithUTF8String:content_type] forHTTPHeaderField:@"Content-Type"];
        }

        __block NSData *responseData = nil;
        __block NSHTTPURLResponse *httpResponse = nil;
        __block NSError *requestError = nil;
        dispatch_semaphore_t sem = dispatch_semaphore_create(0);

        NSURLSessionDataTask *task = [[NSURLSession sharedSession]
            dataTaskWithRequest:request
            completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
                responseData = data;
                httpResponse = (NSHTTPURLResponse *)response;
                requestError = error;
                dispatch_semaphore_signal(sem);
            }];
        [task resume];
        dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 30 * NSEC_PER_SEC));

        if (requestError || !responseData) return -1;

        size_t len = [responseData length];
        char* buf = (char*)malloc(len + 1);
        memcpy(buf, [responseData bytes], len);
        buf[len] = '\0';
        *out_body = buf;
        *out_len = len;
        return (int)[httpResponse statusCode];
    }
}

} // extern "C"

// C++ wrappers
namespace pho {
int pho_platform_http_get(const std::string& url, std::string& out_body) {
    char* body = nullptr;
    size_t len = 0;
    int status = pho_platform_http_get_c(url.c_str(), &body, &len);
    if (body) { out_body.assign(body, len); pho_platform_free(body); }
    return status;
}

int pho_platform_http_post(const std::string& url, const std::string& body,
                           const std::string& content_type, std::string& out_body) {
    char* resp = nullptr;
    size_t len = 0;
    int status = pho_platform_http_post_c(url.c_str(), body.c_str(), body.size(),
                                           content_type.c_str(), &resp, &len);
    if (resp) { out_body.assign(resp, len); pho_platform_free(resp); }
    return status;
}
} // namespace pho
