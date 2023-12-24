#include "CommonHeader.h"
#include "PathHelper.h"

namespace FRAMEWORK {

	FString PathHelper::WorkSpaceDir()
	{
		FString RetDir = FString(FPlatformProcess::BaseDir()) + TEXT("../../");
		FPaths::CollapseRelativeDirectories(RetDir);
		return RetDir;
	}

	FString PathHelper::ResourceDir()
	{
		return WorkSpaceDir() / TEXT("Resource");
	}

	FString PathHelper::ExternalDir()
	{
		return WorkSpaceDir() / TEXT("External");
	}
	
	FString PathHelper::SavedDir()
	{
		return WorkSpaceDir() / TEXT("Saved");
	}

	FString PathHelper::SavedShaderDir()
	{
		return SavedDir() / TEXT("Shader");
	}

	FString PathHelper::SavedConfigDir()
	{
		return SavedDir() / TEXT("Config");
	}

	FString PathHelper::ShaderDir()
	{
		return ResourceDir() / TEXT("Shaders");
	}

}

