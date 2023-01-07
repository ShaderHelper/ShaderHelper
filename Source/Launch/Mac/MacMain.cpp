#include "CommonHeader.h"
#include "App/ShaderHelperApp.h"
//Ensures the consistency of memory allocation between current project and UE modules.
PER_MODULE_DEFINITION()

static FString GSavedCommandLine;

@interface AppDelegate : NSObject<NSApplicationDelegate> {}
@end

@implementation AppDelegate
-(void)applicationDidFinishLaunching:(NSNotification *)notification
{
    SH::ShaderHelperApp app(*GSavedCommandLine);
    app.Run();
    [NSApp terminate:self];
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
