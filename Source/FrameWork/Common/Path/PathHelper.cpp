#include "CommonHeader.h"
#include "PathHelper.h"

namespace FRAMEWORK {

	FString PathHelper::ProjectDir()
	{
		FString RetDir = FString(FPlatformProcess::BaseDir()) + TEXT("../../");
		FPaths::CollapseRelativeDirectories(RetDir);
		return RetDir;
	}

	FString PathHelper::ResourceDir()
	{
		return ProjectDir() / TEXT("Resource/");
	}

	FString PathHelper::ExternalDir()
	{
		return ProjectDir() / TEXT("External/");
	}
	
}

