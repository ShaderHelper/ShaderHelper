#include "CommonHeader.h"
#include "PathHelper.h"

namespace FRAMEWORK {

	FString PathHelper::WorkspaceDir()
	{
		FString RetDir = FString(FPlatformProcess::BaseDir()) + TEXT("../../");
		FPaths::CollapseRelativeDirectories(RetDir);
		return RetDir;
	}

	FString PathHelper::ResourceDir()
	{
		return WorkspaceDir() / TEXT("Resource");
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

}

