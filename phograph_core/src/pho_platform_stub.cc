// Stub platform implementation for testing (non-Apple builds).
// Provides minimal implementations of pho_platform.h functions.

#include "pho_platform.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>

extern "C" {

char* pho_platform_read_file(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) return nullptr;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = static_cast<char*>(malloc(sz + 1));
    if (!buf) { fclose(f); return nullptr; }
    size_t read = fread(buf, 1, sz, f);
    fclose(f);
    buf[read] = '\0';
    if (out_size) *out_size = read;
    return buf;
}

int pho_platform_write_file(const char* path, const void* data, size_t size) {
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    size_t written = fwrite(data, 1, size, f);
    fclose(f);
    return (written == size) ? 0 : -1;
}

int pho_platform_file_exists(const char* path) {
    FILE* f = fopen(path, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

void pho_platform_free(void* ptr) {
    free(ptr);
}

double pho_platform_time_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

uint64_t pho_platform_timer_after(double, void (*)(void*), void*) {
    return 0; // Stub - timers handled by EngineRunLoop in tests
}

void pho_platform_timer_cancel(uint64_t) {}

void pho_platform_measure_text(const char*, const char*, float, float* out_w, float* out_h) {
    // Rough estimate: 8px per char width, font_size height
    *out_w = 100;
    *out_h = 14;
}

void pho_platform_log(const char* message) {
    fprintf(stderr, "[pho] %s\n", message);
}

char* pho_platform_clipboard_get(void) {
    return nullptr; // No clipboard in test stub
}

void pho_platform_clipboard_set(const char*) {}

const char* pho_platform_name(void) {
    return "test-stub";
}

double pho_platform_screen_scale(void) {
    return 1.0;
}

// Phase 24: HTTP stubs
int pho_platform_http_get_c(const char*, char** out_body, size_t* out_len) {
    *out_body = nullptr;
    *out_len = 0;
    return -1; // Not available in stub
}

int pho_platform_http_post_c(const char*, const char*, size_t, const char*, char** out_body, size_t* out_len) {
    *out_body = nullptr;
    *out_len = 0;
    return -1;
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
