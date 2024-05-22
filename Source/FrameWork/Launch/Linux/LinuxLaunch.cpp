#include "CommonHeader.h"
#include "LinuxLaunch.h"

FString LinuxCmdLine(int argc, char* argv[])
{
	FString CommandLine;
	for (int32 Option = 1; Option < argc; Option++)
	{
		CommandLine += TEXT(" ");
		FString Temp = UTF8_TO_TCHAR(argv[Option]);
		if (Temp.Contains(TEXT(" ")))
		{
			int32 Quote = 0;
			if(Temp.StartsWith(TEXT("-")))
			{
				int32 Separator;
				if (Temp.FindChar('=', Separator))
				{
					Quote = Separator + 1;
				}
			}
			Temp = Temp.Left(Quote) + TEXT("\"") + Temp.Mid(Quote) + TEXT("\"");
		}
		CommandLine += Temp;
	}
	return CommandLine;
}