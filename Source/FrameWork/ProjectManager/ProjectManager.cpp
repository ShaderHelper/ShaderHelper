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
		if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Classes\\.shprj"), 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
		{
			FString value = "Sh project file";
			RegSetValueEx(hKey, TEXT(""), 0, REG_SZ, (const BYTE*)*value, sizeof(TCHAR)*value.Len());
			RegCloseKey(hKey);

			if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Classes\\Sh project file"), 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
			{
				FString IconPath = FPlatformProcess::ExecutablePath();
				RegSetKeyValueW(hKey, TEXT("DefaultIcon"), TEXT(""), REG_SZ, (const BYTE*)*IconPath, sizeof(TCHAR) * IconPath.Len());
				HKEY openCmdKey;
				if(RegCreateKeyEx(hKey, TEXT("shell\\open\\command"), 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &openCmdKey, NULL) == ERROR_SUCCESS)
				{
					FString OpenCommand = FPlatformProcess::ExecutablePath() + FString(" -Project=%1");

					DWORD DataType;
					DWORD DataLen;
					RegQueryValueEx(openCmdKey, TEXT(""), 0, &DataType, 0, &DataLen);
					TArray<uint8> Data;
					Data.SetNum(DataLen);
					RegQueryValueEx(openCmdKey, TEXT(""), 0, 0, Data.GetData(), &DataLen);
					FString QueryValue = FString((int32)DataLen, (TCHAR*)Data.GetData());
					if(FCStringWide::Strcmp(*QueryValue, *OpenCommand) != 0)
					{
						RegSetValueEx(openCmdKey, TEXT(""), 0, REG_SZ, (const BYTE*)*OpenCommand, sizeof(TCHAR) * OpenCommand.Len());
						SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
					}
					RegCloseKey(openCmdKey);
				}

				RegCloseKey(hKey);
			}
		}
#elif PLATFORM_MAC
        OSStatus Stat = LSSetDefaultRoleHandlerForContentType(CFSTR("com.shaderhelper.shprj"), kLSRolesAll, CFSTR("com.shaderhelper.app"));
#endif
	}

	void Project::Save()
	{
		SaveAs(Path);
	}

    bool Project::AnyPendingAsset() const
    {
        return !PendingAssets.IsEmpty();
    }

	void Project::SavePendingAssets()
	{
		TArray<AssetPtr<AssetObject>> Temp = PendingAssets;
		for (auto Asset : Temp)
		{
            Asset->Save();
		}
	}

}
