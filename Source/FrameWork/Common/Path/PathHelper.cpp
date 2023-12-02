#include "CommonHeader.h"
#include "PathHelper.h"

namespace FRAMEWORK {

	FString PathHelper::InitialDir()
	{
		FString RetDir = FString(FPlatformProcess::BaseDir()) + TEXT("../../");
		FPaths::CollapseRelativeDirectories(RetDir);
		return RetDir;
	}

	FString PathHelper::ResourceDir()
	{
		return InitialDir() / TEXT("Resource");
	}

	FString PathHelper::ExternalDir()
	{
		return InitialDir() / TEXT("External");
	}
	
	FString PathHelper::SavedDir()
	{
		return InitialDir() / TEXT("Saved");
	}

}

