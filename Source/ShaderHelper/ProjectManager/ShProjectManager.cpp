#include "CommonHeader.h"
#include "ShProjectManager.h"
#include <Serialization/JsonSerializer.h>

using namespace FRAMEWORK;

namespace SH
{

	ShProject::ShProject(FString InPath) : Project(MoveTemp(InPath))
	{
		GProjectVer = static_cast<int>(ShProjectVersion::Initial);
	}

	void ShProject::Open(const FString& ProjectPath)
	{
		TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileReader(*ProjectPath));
		Serialize(*Ar);
	}

	void ShProject::Save()
	{
		Save(Path);
	}

	void ShProject::Save(const FString& InPath)
	{
		Path = InPath;
		TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*InPath));
		Serialize(*Ar);
	}

	void ShProject::Serialize(FArchive& Ar)
	{
		Project::Serialize(Ar);

		//### AssetBrowserState ###
		FString RelSelectedDirectory = TSingleton<ShProjectManager>::Get().GetRelativePathToProject(AssetBrowserState.DirectoryTreeState.CurSelectedDirectory);
		Ar << RelSelectedDirectory;
		for (const FString& Directory : AssetBrowserState.DirectoryTreeState.DirectoriesToExpand)
		{
			FString RelDir = TSingleton<ShProjectManager>::Get().GetRelativePathToProject(RelSelectedDirectory);
			Ar << RelDir;
		}
		Ar << AssetBrowserState.AssetViewState.AssetViewSize;
		//###

		Ar << Graph;
		
		int StShaderNum = OpenedStShaders.Num();
		Ar << StShaderNum;

		FString CodeTabLayoutContents;
		TSharedPtr<FJsonObject> CodeTabLayoutJsonObject;
		bool HasCodeTabLayout = CodeTabLayout.IsValid();
		Ar << HasCodeTabLayout;
		if (Ar.IsSaving())
		{
			for (auto& [OpenedStShader, _] : OpenedStShaders)
			{
				Ar << OpenedStShader;
			}

			if (HasCodeTabLayout)
			{
				CodeTabLayoutJsonObject = CodeTabLayout->ToJson();
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&CodeTabLayoutContents);
				FJsonSerializer::Serialize(CodeTabLayoutJsonObject.ToSharedRef(), Writer);
				Ar << CodeTabLayoutContents;
			}

		}
		else
		{
			OpenedStShaders.Reserve(StShaderNum);
			for (int i = 0; i < StShaderNum; i++)
			{
				AssetPtr<StShader> LoadedStShader;
				Ar << LoadedStShader;
				OpenedStShaders.Add(MoveTemp(LoadedStShader), nullptr);
			}

			if (HasCodeTabLayout)
			{
				Ar << CodeTabLayoutContents;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(CodeTabLayoutContents);
				FJsonSerializer::Deserialize(Reader, CodeTabLayoutJsonObject);
				CodeTabLayout = FTabManager::FLayout::NewFromJson(CodeTabLayoutJsonObject);
			}

		}

	}

}