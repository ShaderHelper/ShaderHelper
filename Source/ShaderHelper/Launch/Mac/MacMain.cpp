#include "CommonHeader.h"
#include "App/ShaderHelperApp.h"
#include "UI/Widgets/ShaderHelperWindow/SShaderHelperWindow.h"
#include <Mac/CocoaThread.h>
#include "Renderer/ShRenderer.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Styles/FAppCommonStyle.h"

static FString GSavedCommandLine;

@interface AppDelegate : NSObject<NSApplicationDelegate> {}
@end

@implementation AppDelegate
-(void)run:(id)Arg
{
    FPlatformMisc::SetCrashHandler(nullptr);
    
	SH::UE_Init(*GSavedCommandLine);
	SH::FAppCommonStyle::Init();
	SH::FShaderHelperStyle::Init();

	TUniquePtr<SH::ShRenderer> Renderer = MakeUnique<SH::ShRenderer>();
	TSharedRef<SH::SShaderHelperWindow> Window = SNew(SH::SShaderHelperWindow).Renderer(Renderer.Get());
	SH::ShaderHelperApp app(MoveTemp(Window), MoveTemp(Renderer));
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
