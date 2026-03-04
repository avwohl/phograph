#pragma once
// C-compatible API for bridging to Swift/ObjC.
// Uses opaque handles, no C++ types exposed.

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to the engine
typedef struct PhoEngine* PhoEngineRef;

// Create/destroy engine
PhoEngineRef pho_engine_create(void);
void pho_engine_destroy(PhoEngineRef engine);

// Load a project from JSON string. Returns 0 on success.
int pho_engine_load_json(PhoEngineRef engine, const char* json, size_t json_len);

// Call a method by name. Returns JSON-encoded result.
// Caller must free the returned string via pho_engine_free_string.
const char* pho_engine_call_method(PhoEngineRef engine, const char* method_name,
                                     const char* inputs_json, size_t inputs_len);

// Get last error message (static, do not free)
const char* pho_engine_last_error(PhoEngineRef engine);

// Free a string returned by engine functions
void pho_engine_free_string(const char* str);

// Register all primitives (call once at startup)
void pho_engine_init_prims(void);

// ---- Pixel buffer for rendering (Phase 5+) ----
// Get the pixel buffer pointer and dimensions
const uint8_t* pho_engine_pixel_buffer(PhoEngineRef engine, int32_t* out_width, int32_t* out_height);

// Notify engine of a resize
void pho_engine_resize(PhoEngineRef engine, int32_t width, int32_t height);

// Tick the engine (call per frame)
void pho_engine_tick(PhoEngineRef engine, double dt);

// ---- Input events (Phase 6+) ----
void pho_engine_send_pointer_down(PhoEngineRef engine, float x, float y, int32_t button);
void pho_engine_send_pointer_up(PhoEngineRef engine, float x, float y, int32_t button);
void pho_engine_send_pointer_move(PhoEngineRef engine, float x, float y);
void pho_engine_send_pointer_drag(PhoEngineRef engine, float x, float y, float dx, float dy);
void pho_engine_send_scroll(PhoEngineRef engine, float x, float y, float dx, float dy);
void pho_engine_send_key_down(PhoEngineRef engine, const char* key, uint32_t key_code, uint32_t modifiers);
void pho_engine_send_key_up(PhoEngineRef engine, const char* key, uint32_t key_code, uint32_t modifiers);

#ifdef __cplusplus
}
#endif
