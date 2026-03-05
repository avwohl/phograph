#import "PhographBridge.h"
#include "pho_bridge.h"
#include <string>

static BOOL sPrimsInitialized = NO;

@implementation PhographBridge {
    PhoEngineRef _engine;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        if (!sPrimsInitialized) {
            pho_engine_init_prims();
            sPrimsInitialized = YES;
        }
        _engine = pho_engine_create();
    }
    return self;
}

- (void)dealloc {
    if (_engine) {
        pho_engine_destroy(_engine);
        _engine = nil;
    }
}

- (BOOL)loadProjectJSON:(NSString *)json error:(NSError **)error {
    const char *cstr = [json UTF8String];
    int result = pho_engine_load_json(_engine, cstr, strlen(cstr));
    if (result != 0) {
        if (error) {
            NSString *msg = [NSString stringWithUTF8String:pho_engine_last_error(_engine)];
            *error = [NSError errorWithDomain:@"PhographBridge"
                                         code:1
                                     userInfo:@{NSLocalizedDescriptionKey: msg ?: @"Unknown error"}];
        }
        return NO;
    }
    return YES;
}

- (nullable NSString *)callMethod:(NSString *)name
                       inputsJSON:(nullable NSString *)inputsJSON
                            error:(NSError **)error {
    const char *inputs_cstr = inputsJSON ? [inputsJSON UTF8String] : NULL;
    size_t inputs_len = inputs_cstr ? strlen(inputs_cstr) : 0;

    const char *result = pho_engine_call_method(_engine, [name UTF8String], inputs_cstr, inputs_len);
    if (!result) {
        if (error) {
            *error = [NSError errorWithDomain:@"PhographBridge"
                                         code:2
                                     userInfo:@{NSLocalizedDescriptionKey: @"Method call returned null"}];
        }
        return nil;
    }

    NSString *resultStr = [NSString stringWithUTF8String:result];
    pho_engine_free_string(result);
    return resultStr;
}

- (nullable NSString *)lastError {
    const char *err = pho_engine_last_error(_engine);
    if (!err || err[0] == '\0') return nil;
    return [NSString stringWithUTF8String:err];
}

- (const uint8_t *)pixelBuffer {
    int32_t w, h;
    return pho_engine_pixel_buffer(_engine, &w, &h);
}

- (int32_t)bufferWidth {
    int32_t w, h;
    pho_engine_pixel_buffer(_engine, &w, &h);
    return w;
}

- (int32_t)bufferHeight {
    int32_t w, h;
    pho_engine_pixel_buffer(_engine, &w, &h);
    return h;
}

- (void)resizeBuffer:(int32_t)width height:(int32_t)height {
    pho_engine_resize(_engine, width, height);
}

- (void)tick:(double)dt {
    pho_engine_tick(_engine, dt);
}

- (nullable const uint8_t *)pixelBufferWidth:(int32_t *)outWidth height:(int32_t *)outHeight {
    return pho_engine_pixel_buffer(_engine, outWidth, outHeight);
}

// Console output

- (nullable NSString *)consoleOutput {
    const char *console = pho_engine_get_console(_engine);
    if (!console || console[0] == '\0') return nil;
    return [NSString stringWithUTF8String:console];
}

- (void)clearConsole {
    pho_engine_clear_console(_engine);
}

// Debug support

static void debugTrampoline(void* ctx, const char* json) {
    PhographBridge* bridge = (__bridge PhographBridge*)ctx;
    NSString* str = [NSString stringWithUTF8String:json];
    dispatch_async(dispatch_get_main_queue(), ^{
        if (bridge.debugEventHandler) {
            bridge.debugEventHandler(str);
        }
    });
}

- (void)debugRun:(NSString *)methodName {
    pho_engine_debug_set_callback(_engine, debugTrampoline, (__bridge void*)self);
    pho_engine_debug_run(_engine, [methodName UTF8String]);
}

- (void)debugContinue {
    pho_engine_debug_continue(_engine);
}

- (void)debugStepOver {
    pho_engine_debug_step_over(_engine);
}

- (void)debugStepInto {
    pho_engine_debug_step_into(_engine);
}

- (void)debugStop {
    pho_engine_debug_stop(_engine);
}

- (void)debugAddBreakpoint:(uint32_t)nodeId method:(NSString *)method caseIndex:(int)caseIndex {
    pho_engine_debug_add_breakpoint(_engine, nodeId, [method UTF8String], caseIndex);
}

- (void)debugRemoveBreakpoint:(uint32_t)nodeId {
    pho_engine_debug_remove_breakpoint(_engine, nodeId);
}

// Input event routing

- (void)sendPointerDown:(float)x y:(float)y button:(int32_t)button {
    pho_engine_send_pointer_down(_engine, x, y, button);
}

- (void)sendPointerUp:(float)x y:(float)y button:(int32_t)button {
    pho_engine_send_pointer_up(_engine, x, y, button);
}

- (void)sendPointerMove:(float)x y:(float)y {
    pho_engine_send_pointer_move(_engine, x, y);
}

- (void)sendPointerDrag:(float)x y:(float)y dx:(float)dx dy:(float)dy {
    pho_engine_send_pointer_drag(_engine, x, y, dx, dy);
}

- (void)sendScroll:(float)x y:(float)y dx:(float)dx dy:(float)dy {
    pho_engine_send_scroll(_engine, x, y, dx, dy);
}

- (void)sendKeyDown:(NSString *)key keyCode:(uint32_t)keyCode modifiers:(uint32_t)modifiers {
    pho_engine_send_key_down(_engine, [key UTF8String], keyCode, modifiers);
}

- (void)sendKeyUp:(NSString *)key keyCode:(uint32_t)keyCode modifiers:(uint32_t)modifiers {
    pho_engine_send_key_up(_engine, [key UTF8String], keyCode, modifiers);
}

@end
