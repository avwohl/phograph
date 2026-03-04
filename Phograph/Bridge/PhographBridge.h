#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface PhographBridge : NSObject

- (instancetype)init;
- (void)dealloc;

/// Load a project from JSON string. Returns YES on success.
- (BOOL)loadProjectJSON:(NSString *)json error:(NSError **)error;

/// Call a method by name with optional JSON-encoded inputs.
/// Returns JSON-encoded result string.
- (nullable NSString *)callMethod:(NSString *)name
                       inputsJSON:(nullable NSString *)inputsJSON
                            error:(NSError **)error;

/// Get last error message
@property (nonatomic, readonly, nullable) NSString *lastError;

/// Pixel buffer access (for Metal rendering)
@property (nonatomic, readonly) const uint8_t *pixelBuffer;
@property (nonatomic, readonly) int32_t bufferWidth;
@property (nonatomic, readonly) int32_t bufferHeight;

- (void)resizeBuffer:(int32_t)width height:(int32_t)height;
- (void)tick:(double)dt;

/// Get pixel buffer with dimensions
- (nullable const uint8_t *)pixelBufferWidth:(int32_t *)outWidth height:(int32_t *)outHeight;

/// Input event routing
- (void)sendPointerDown:(float)x y:(float)y button:(int32_t)button;
- (void)sendPointerUp:(float)x y:(float)y button:(int32_t)button;
- (void)sendPointerMove:(float)x y:(float)y;
- (void)sendPointerDrag:(float)x y:(float)y dx:(float)dx dy:(float)dy;
- (void)sendScroll:(float)x y:(float)y dx:(float)dx dy:(float)dy;
- (void)sendKeyDown:(NSString *)key keyCode:(uint32_t)keyCode modifiers:(uint32_t)modifiers;
- (void)sendKeyUp:(NSString *)key keyCode:(uint32_t)keyCode modifiers:(uint32_t)modifiers;

@end

NS_ASSUME_NONNULL_END
