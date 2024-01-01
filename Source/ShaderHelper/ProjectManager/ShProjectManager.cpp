#include "CommonHeader.h"
#include "ShProjectManager.h"

namespace SH
{

	void ShProject::Open(const FString& ProjectPath)
	{
		TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileReader(*ProjectPath));
		Serialize(*Ar);
	}

	void ShProject::Save()
	{
		TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*Path));
		Serialize(*Ar);
		
	}
	void ShProject::Serialize(FArchive& Ar)
	{
		Ar.Serialize(&Ver, sizeof(Version));
	}

}