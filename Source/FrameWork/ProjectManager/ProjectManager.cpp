#include "CommonHeader.h"
#include "ProjectManager.h"
#if PLATFORM_WINDOWS
#include <Windows/AllowWindowsPlatformTypes.h>
	#include <shlobj_core.h>
#include <Windows/HideWindowsPlatformTypes.h>
#elif PLATFORM_MAC
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#endif

namespace FW
{
	int GProjectVer;
	Project* GProject;

	void AddProjectAssociation()
	{
#if PLATFORM_WINDOWS
		HKEY hKey;
		if (RegCreateKeyEx(HKEY_CLASSES_ROOT, TEXT(".shprj"), 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
		{
			FString value = "Sh project file";
			RegSetValueEx(hKey, TEXT(""), 0, REG_SZ, (const BYTE*)*value, sizeof(TCHAR)*value.Len());
			RegCloseKey(hKey);

			if (RegCreateKeyEx(HKEY_CLASSES_ROOT, TEXT("Sh project file"), 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
			{
				FString IconPath = FPlatformProcess::ExecutablePath();
				RegSetKeyValueW(hKey, TEXT("DefaultIcon"), TEXT(""), REG_SZ, (const BYTE*)*IconPath, sizeof(TCHAR) * IconPath.Len());
				HKEY openCmdKey;
				if(RegCreateKeyEx(hKey, TEXT("shell\\open\\command"), 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &openCmdKey, NULL) == ERROR_SUCCESS)
				{
					FString OpenCommand = FPlatformProcess::ExecutablePath() + FString(" -Project=%1");
					RegSetValueEx(openCmdKey, TEXT(""), 0, REG_SZ, (const BYTE*)*OpenCommand, sizeof(TCHAR) * OpenCommand.Len());
					RegCloseKey(openCmdKey);
				}

				RegCloseKey(hKey);
				SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
			}
		}
#elif PLATFORM_MAC
        OSStatus Stat = LSSetDefaultRoleHandlerForContentType(CFSTR("com.shaderhelper.shprj"), kLSRolesAll, CFSTR("com.shaderhelper.app"));
#endif
	}

	void Project::SavePendingAssets()
	{
		TArray<AssetObject*> Temp = PendingAssets;
		for (auto Asset : Temp)
		{
			Asset->Save();
		}
	}

}
