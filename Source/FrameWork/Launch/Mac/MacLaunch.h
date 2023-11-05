
FRAMEWORK_API
@interface MacLaunch : NSObject

+(void)launch:(int)argc argv:(char*[])argv runBlock:(void(^)(const TCHAR*))block;
@end
