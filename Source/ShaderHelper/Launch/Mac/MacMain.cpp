#include "CommonHeader.h"
#include "App/ShaderHelperApp.h"
#include "UI/Widgets/SShaderHelperWindow.h"
#include <Mac/CocoaThread.h>

static FString GSavedCommandLine;

@interface AppDelegate : NSObject<NSApplicationDelegate> {}
@end

@implementation AppDelegate
-(void)run:(id)Arg
{
	SH::UE_Init(*GSavedCommandLine);
	SH::ShaderHelperApp app(SNew(SH::SShaderHelperWindow));
	app.Run();
	SH::UE_ShutDown();
    [NSApp terminate:self];
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification
{
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
