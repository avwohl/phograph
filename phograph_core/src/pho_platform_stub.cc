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

} // extern "C"
