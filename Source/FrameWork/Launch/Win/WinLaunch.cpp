#include "CommonHeader.h"
#include "WinLaunch.h"
#include <HAL/ExceptionHandling.h>

void WinLaunch(TFunctionRef<void(const TCHAR*)> RunBlock, const TCHAR* CmdLine)
{
#if SH_SHIPPING
	__try
#endif
	{
		GIsGuarded = 1;
		RunBlock(CmdLine);
		GIsGuarded = 0;
        FPlatformMisc::RequestExit(true);
	}
#if SH_SHIPPING
	__except (ReportCrash(GetExceptionInformation()))
	{
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, 
			TEXT("An unexpected error occurred. Please checkout the log or dmp file in saved folder and contact https://github.com/ShaderHelper/ShaderHelper."), TEXT("Crashed Program"));
		if (GError)
		{
			GError->HandleError();
		}
		FPlatformMisc::RequestExit(true);
	}
#endif
}
