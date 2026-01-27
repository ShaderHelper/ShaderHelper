#pragma once
#include "AssetObject/AssetObject.h"

#include <HAL/PlatformFileManager.h>
#include <Async/AsyncFileHandle.h>
#include <Async/Async.h>

namespace FW
{
	class GpuTexture;

    template<typename T>
    using AssetPtr = ObjectPtr<T, ObjectOwnerShip::Retain>;

	//Ensure the GuidToPath of AssetManager is always accessed with a normalized path.
	// A/B/Rsource.xxx = A/B\\Resource.xxx
	struct AssetPath
	{
		AssetPath() = default;
		AssetPath(FString InPathStr) : PathStr(MoveTemp(InPathStr)) 
		{
			FPaths::NormalizeFilename(PathStr);
			PathStr = FPaths::ConvertRelativePathToFull(PathStr);
			FPaths::RemoveDuplicateSlashes(PathStr);
		}

		bool operator==(const AssetPath& Other) const
		{
			return PathStr == Other.PathStr;
		}

		operator FString() const { return PathStr; }
		FString PathStr;
	};

	FRAMEWORK_API extern int GAssetVer;
	FRAMEWORK_API extern FCriticalSection GAssetCS;

	class FRAMEWORK_API AssetManager
	{
	public:
		void MountProject(const FString& InProjectContentDir);

		template<typename T>
		AssetPtr<T> LoadAssetByPath(const FString& InAssetPath)
		{
			static_assert(std::is_base_of_v<AssetObject, T>);
			if (!IsValidAsset(InAssetPath))
			{
				return nullptr;
			}

			FGuid Guid = GetGuid(InAssetPath);
			if (Assets.Contains(Guid))
			{
				return { static_cast<T*>(Assets[Guid]) };
			}

			AssetPtr<T> NewAssetObject = ConstructAssetObject<T>(InAssetPath);
			check(NewAssetObject.IsValid());

			TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileReader(*InAssetPath));
			NewAssetObject->Serialize(*Ar);
			NewAssetObject->PostLoad();
			Assets.Add(Guid, NewAssetObject);
			return NewAssetObject;
		}

		//Only can be called from the main/game thread.
		template<typename T>
		void AsyncLoadAssetByPath(const FString& InAssetPath, const TFunction<void(AssetPtr<T>)>& CallBack)
		{
			static_assert(std::is_base_of_v<AssetObject, T>);

			if (!IsValidAsset(InAssetPath))
			{
				CallBack(nullptr);
				return;
			}

			FGuid Guid = GetGuid(InAssetPath);
			if (Assets.Contains(Guid))
			{
				CallBack(static_cast<T*>(Assets[Guid]));
				return;
			}

			if (PendingLoads.Contains(Guid))
			{
				PendingLoads[Guid].Add([CallBack](AssetObject* Obj) {
					CallBack(static_cast<T*>(Obj));
				});
				return;
			}

			PendingLoads.Add(Guid, { [CallBack](AssetObject* Obj) {
				CallBack(static_cast<T*>(Obj));
			} });

			IAsyncReadFileHandle* AsyncHandle{ FPlatformFileManager::Get().GetPlatformFile().OpenAsyncRead(*InAssetPath) };
			TUniquePtr<IAsyncReadRequest> SizeRequest{ AsyncHandle->SizeRequest() };
			SizeRequest->WaitCompletion();
			int64 FileSize = SizeRequest->GetSizeResults();
			FAsyncFileCallBack ReadCompleteCallBack = [=, this](bool bWasCancelled, IAsyncReadRequest* Request) {
				uint8* RawData = Request->GetReadResults();
				AsyncTask(ENamedThreads::GameThread, [=, this] {
					ON_SCOPE_EXIT
					{
						delete Request;
						delete AsyncHandle;
					};

					if (!RawData)
					{
						NotifyPendingLoads(Guid, nullptr);
						return;
					}

					FBufferReader Ar(RawData, FileSize, true);
					AssetPtr<T> NewAssetObject = ConstructAssetObject<T>(InAssetPath);
					check(NewAssetObject.IsValid());
					NewAssetObject->Serialize(Ar);
					NewAssetObject->PostLoad();
					Assets.Add(Guid, NewAssetObject);
					NotifyPendingLoads(Guid, NewAssetObject);
				});
			};
			AsyncHandle->ReadRequest(0, FileSize, AIOP_Normal, &ReadCompleteCallBack);
		}

		template<typename T>
		AssetPtr<T> LoadAssetByGuid(const FGuid& InGuid)
		{
			return IsValidAsset(InGuid) ? LoadAssetByPath<T>(GetPath(InGuid)) : nullptr;
		}

		template<typename T>
		void AsyncLoadAssetByGuid(const FGuid& InGuid, const TFunction<void(AssetPtr<T>)>& CallBack)
		{
			if (IsValidAsset(InGuid))
			{
				AsyncLoadAssetByPath(GetPath(InGuid), CallBack);
			}
			else
			{
				CallBack(nullptr);
			}
		}

		AssetObject* FindLoadedAsset(const FGuid& Id)
		{
			return Assets.Contains(Id) ? Assets[Id] : nullptr;
		}
        AssetObject* FindLoadedAsset(const FString& InPath)
        {
			if (!TryGetGuid(InPath))
			{
				return nullptr;
			}
            return FindLoadedAsset(GetGuid(InPath));
        }

        FGuid ReadAssetGuidInDisk(const FString& InPath);
		void UpdateGuidToPath(const FString& InPath);
        void RemoveGuidToPath(const FString& InPath);

		FString GetPath(const FGuid& InGuid) const;
		FGuid GetGuid(const FString& InPath) const;
		TOptional<FGuid> TryGetGuid(const FString& InPath) const;

		//The asset may be deleted outside
		bool IsValidAsset(const FGuid& Id) const { return GuidToPath.Contains(Id); }
		bool IsValidAsset(const FString& InPath) const { return GuidToPath.FindKey(InPath) != nullptr; }

        void Clear();
        void DestroyAsset(const FString& InAssetPath);

		void AddAssetThumbnail(const FGuid& InGuid, TRefCountPtr<GpuTexture> InThumbnail);
		GpuTexture* FindAssetThumbnail(const FGuid& InGuid) const;

        void RemoveAsset(AssetObject* InAsset);
		TArray<FString> GetManageredExts() const;

	private:
		template<typename T>
		AssetPtr<T> ConstructAssetObject(const FString& InAssetPath)
		{
			AssetPtr<T> NewAssetObject = nullptr;
			FString AssetExt = FPaths::GetExtension(InAssetPath);
			TArray<MetaType*> MetaTypes = GetMetaTypes<T>();
			for (auto MetaTypePtr : MetaTypes)
			{
				void* DefaultObject = MetaTypePtr->GetDefaultObject();
				if (DefaultObject)
				{
					T* RelDefaultObject = static_cast<T*>(DefaultObject);
					if (RelDefaultObject->FileExtension().Contains(AssetExt))
					{
						NewAssetObject = NewShObject<T>(MetaTypePtr, nullptr);
						break;
					}
				}
			}
			return NewAssetObject;
		}

		void NotifyPendingLoads(const FGuid& Guid, AssetObject* Result)
		{
			if (auto* Callbacks = PendingLoads.Find(Guid))
			{
				for (auto& Cb : *Callbacks)
				{
					Cb(Result);
				}
				PendingLoads.Remove(Guid);
			}
		}

	private:
		TMap<FGuid, AssetPath> GuidToPath;
		TMap<FGuid, AssetObject*> Assets; //Loaded asset
		TMap<FGuid, TRefCountPtr<GpuTexture>> AssetThumbnailPool;
		TMap<FGuid, TArray<TFunction<void(AssetObject*)>>> PendingLoads;
	};

    template<typename T>
    FArchive& operator<<(FArchive& Ar, AssetPtr<T>& InOutAssetPtr)
    {
        bool IsValid = !!InOutAssetPtr;
        Ar << IsValid;
        if (Ar.IsLoading())
        {
            if (IsValid)
            {
                FGuid AssetGuid;
                Ar << AssetGuid;
                //Maybe return a null AssetPtr because of outside deleting.
                InOutAssetPtr = TSingleton<AssetManager>::Get().LoadAssetByGuid<T>(AssetGuid);
            }
        }
        else
        {
            if (IsValid)
            {
                FGuid AssetGuid = InOutAssetPtr->GetGuid();;
                Ar << AssetGuid;
            }
        }
        return Ar;
    }
}
