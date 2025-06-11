#include "CommonHeader.h"
#include "MacLaunch.h"
#include <Mac/CocoaThread.h>

@interface AppDelegate : NSObject<NSApplicationDelegate>

@property (nonatomic) void(^runBlock)(const TCHAR*);
@property (nonatomic) FString SavedCommandLine;

@end

@implementation AppDelegate

-(void)run:(id)Arg
{
    FPlatformMisc::SetCrashHandler(nullptr);
    _runBlock(*_SavedCommandLine);
	FPlatformMisc::RequestExit(true);
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification
{
    RunGameThread(self, @selector(run:));
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)Sender;
{
	FPlatformMisc::RequestExit(true);
}

- (BOOL)application:(NSApplication *)sender openFile:(NSString *)filename
{
    self.SavedCommandLine += TEXT(" -Project=") + FString(filename);
}
@end

@implementation MacLaunch
{
    
}

+(void)launch:(int)argc argv:(char*[])argv runBlock:(void(^)(const TCHAR*))block
{
    
    AppDelegate* Delegate = [AppDelegate new];
    Delegate.runBlock = block;
    
    FString CommandLine;
    for (int32 Option = 1; Option < argc; Option++)
    {
        CommandLine += TEXT(" ");
        FString Argument(ANSI_TO_TCHAR(argv[Option]));
        if (Argument.Contains(TEXT(" ")))
        {
            if (Argument.Contains(TEXT("=")))
            {
                FString ArgName;
                FString ArgValue;
                Argument.Split( TEXT("="), &ArgName, &ArgValue );
                Argument = FString::Printf( TEXT("%s=\"%s\""), *ArgName, *ArgValue );
            }
            else
            {
                Argument = FString::Printf(TEXT("\"%s\""), *Argument);
            }
        }
        CommandLine += Argument;
    }
    Delegate.SavedCommandLine = MoveTemp(CommandLine);

    SCOPED_AUTORELEASE_POOL;
    [NSApplication sharedApplication];
    [NSApp setDelegate:Delegate];
    [NSApp run];
}
@end
