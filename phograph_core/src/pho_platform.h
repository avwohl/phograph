#pragma once
// Platform abstraction header.
// All functions declared here must be implemented per-platform.
// The C++ core has ZERO platform #includes - all I/O goes through these functions.

#include <cstdint>
#include <cstddef>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

// ---- File I/O ----
// Read entire file contents. Returns null-terminated string (caller must free via pho_platform_free).
// Returns nullptr on failure.
char* pho_platform_read_file(const char* path, size_t* out_size);

// Write data to file. Returns 0 on success, -1 on failure.
int pho_platform_write_file(const char* path, const void* data, size_t size);

// Check if file exists
int pho_platform_file_exists(const char* path);

// Free memory allocated by platform functions
void pho_platform_free(void* ptr);

// ---- Time ----
// Current time in seconds since Unix epoch (with sub-second precision)
double pho_platform_time_now(void);

// Schedule a callback after delay_seconds. Returns timer id.
uint64_t pho_platform_timer_after(double delay_seconds, void (*callback)(void* ctx), void* ctx);

// Cancel a timer
void pho_platform_timer_cancel(uint64_t timer_id);

// ---- Text ----
// Measure text size in pixels. Returns width and height.
void pho_platform_measure_text(const char* text, const char* font_name,
                                float font_size, float* out_width, float* out_height);

// ---- Logging ----
void pho_platform_log(const char* message);

// ---- Clipboard ----
// Get clipboard text (caller must free)
char* pho_platform_clipboard_get(void);

// Set clipboard text
void pho_platform_clipboard_set(const char* text);

// ---- System Info ----
const char* pho_platform_name(void);  // "macOS", "iOS"
double pho_platform_screen_scale(void);

// ---- HTTP Networking (Phase 24) ----
// HTTP GET. Returns status code (>= 0 success, < 0 error).
// out_body is allocated by platform, caller must free via pho_platform_free.
int pho_platform_http_get_c(const char* url, char** out_body, size_t* out_len);

// HTTP POST. Returns status code.
int pho_platform_http_post_c(const char* url, const char* body, size_t body_len,
                              const char* content_type, char** out_body, size_t* out_len);

#ifdef __cplusplus
}

// C++ wrappers in pho namespace
namespace pho {
int pho_platform_http_get(const std::string& url, std::string& out_body);
int pho_platform_http_post(const std::string& url, const std::string& body,
                           const std::string& content_type, std::string& out_body);
}

#endif
