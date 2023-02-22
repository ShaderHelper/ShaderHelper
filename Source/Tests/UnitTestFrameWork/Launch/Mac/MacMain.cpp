#include "CommonHeader.h"
#include "App/UnitTestConSole.h"
#include <Mac/CocoaThread.h>
//Ensures the consistency of memory allocation between current project and UE modules.
PER_MODULE_DEFINITION()

static FString GSavedCommandLine;

@interface AppDelegate : NSObject<NSApplicationDelegate> {}
@end

@implementation AppDelegate
-(void)run:(id)Arg
{
    FPlatformMisc::SetGracefulTerminationHandler();
    FPlatformMisc::SetCrashHandler(nullptr);
    
    UnitTestConSole app(*GSavedCommandLine);
	app.SetClientSize(FVector2D(800, 600));
    app.Run();
    [NSApp terminate:self];
}

//handler for the quit apple event used by the Dock menu
- (void)handleQuitEvent:(NSAppleEventDescriptor*)Event withReplyEvent:(NSAppleEventDescriptor*)ReplyEvent
{
    [NSApp terminate:self];
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification
{
    //install the custom quit event handler
    NSAppleEventManager* appleEventManager = [NSAppleEventManager sharedAppleEventManager];
    [appleEventManager setEventHandler:self andSelector:@selector(handleQuitEvent:withReplyEvent:) forEventClass:kCoreEventClass andEventID:kAEQuitApplication];
    
    RunGameThread(self, @selector(run:));
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)Sender;
{
    if(!IsEngineExitRequested() || ([NSThread gameThread] && [NSThread gameThread] != [NSThread mainThread]))
    {
        RequestEngineExit(TEXT("applicationShouldTerminate"));
        return NSTerminateLater;
    }
    else
    {
        return NSTerminateNow;
    }
}
@end

int main(int argc, char *argv[])
{
    for (int32 Option = 1; Option < argc; Option++)
    {
        GSavedCommandLine += TEXT(" ");
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
        GSavedCommandLine += Argument;
    }

    SCOPED_AUTORELEASE_POOL;
    [NSApplication sharedApplication];
    [NSApp setDelegate:[AppDelegate new]];
    [NSApp run];
    return 0;
}
