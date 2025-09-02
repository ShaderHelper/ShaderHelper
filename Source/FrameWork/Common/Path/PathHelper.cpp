#include "CommonHeader.h"
#include "PathHelper.h"

namespace FW {

	FString PathHelper::WorkspaceDir()
	{
		FString RetDir = FString(FPlatformProcess::BaseDir()) + TEXT("../");
		FPaths::CollapseRelativeDirectories(RetDir);
		return RetDir;
	}

	FString PathHelper::ResourceDir()
	{
		return WorkspaceDir() / TEXT("Resource");
	}

	FString PathHelper::PluginDir()
	{
		return WorkspaceDir() / TEXT("Plugins");
	}

	FString PathHelper::ExternalDir()
	{
		return WorkspaceDir() / TEXT("External");
	}
	
	FString PathHelper::SavedDir()
	{
		return WorkspaceDir() / TEXT("Saved") / GAppName;
	}

	FString PathHelper::SavedLogDir()
	{
		return SavedDir() / TEXT("Log");
	}

	FString PathHelper::SavedShaderDir()
	{
		return SavedDir() / TEXT("Shader");
	}

	FString PathHelper::SavedConfigDir()
	{
		return SavedDir() / TEXT("Config");
	}

    FString PathHelper::SavedCaptureDir()
    {
        return SavedDir() / TEXT("Capture");
    }

	FString PathHelper::ShaderDir()
	{
		return ResourceDir() / TEXT("Shaders");
	}

	FString PathHelper::ErrorDir()
	{
		return SavedDir() / TEXT("Error");
	}

    bool PathHelper::ParseProjectPath(const FString& CommandLine, FString& OutPath)
    {
        FString Key = TEXT("-Project=");
        FString Ext = TEXT(".shprj");
        int32 StartIndex = CommandLine.Find(Key);
        if(StartIndex != INDEX_NONE)
        {
            StartIndex += Key.Len();
            int32 EndIndex = CommandLine.Find(Ext, ESearchCase::CaseSensitive, ESearchDir::FromStart, StartIndex);
            if(EndIndex != INDEX_NONE)
            {
                EndIndex += Ext.Len();
                OutPath = CommandLine.Mid(StartIndex, EndIndex - StartIndex);
                return true;
            }
        }
        return false;
    }

}

