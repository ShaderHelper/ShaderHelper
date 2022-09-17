// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/ScopeLock.h"
#include "Misc/ScopeRWLock.h"
#include "Math/UnrealMathUtility.h"
#include "Containers/UnrealString.h"
#include "Logging/LogMacros.h"
#include "Templates/UniquePtr.h"
#include "Async/Async.h"
#include "Algo/Find.h"
#include "Algo/IndexOf.h"
#include <atomic>

DECLARE_LOG_CATEGORY_EXTERN(LogPlatformFileManagedStorage, Display, All);

namespace ManagedStorageInternal
{
	// Same as FPaths::IsUnderDirectory, but assume the paths are already full
	// Also is *always* case insensitive since we are concerned with filtering and 
	// not whether a directory actually exists.
	bool IsUnderDirectory(const FString& InPath, const FString& InDirectory);
}

struct FPersistentManagedFile
{
	FString FullFilename;
	int32 Category = INDEX_NONE;

	bool IsValid() const { return Category >= 0; }
	explicit operator bool() const { return Category >= 0; }

	void Clear()
	{
		Category = INDEX_NONE;
	}
};

class FManagedStorageFileLockRegistry
{
private:
	friend class FManagedStorageScopeFileLock;

	FCriticalSection* GetLock(const FString& InFilename)
	{
		uint32 KeyHash = GetTypeHash(InFilename);

		FScopeLock Lock(&FileLockMapCS);

		FLockData& LockData = FileLockMap.FindOrAddByHash(KeyHash, InFilename);
		++LockData.RefCount;

		return LockData.CS.Get();
	}

	void ReleaseLock(const FString& InFilename)
	{
		uint32 KeyHash = GetTypeHash(InFilename);

		FScopeLock Lock(&FileLockMapCS);

		FLockData* LockData = FileLockMap.FindByHash(KeyHash, InFilename);
		check(LockData);

		--LockData->RefCount;

		if (LockData->RefCount == 0)
		{
			FileLockMap.RemoveByHash(KeyHash, InFilename);
		}
	}

private:

	struct FLockData
	{
		TUniquePtr<FCriticalSection> CS{ MakeUnique<FCriticalSection>() };
		int32 RefCount = 0;
	};

	TMap<FString, FLockData> FileLockMap;
	FCriticalSection FileLockMapCS;
};

class FManagedStorageScopeFileLock
{
public:
	FManagedStorageScopeFileLock(FPersistentManagedFile InManagedFile)
		: ManagedFile(MoveTemp(InManagedFile))
	{
		Lock();
	}

	~FManagedStorageScopeFileLock()
	{
		Unlock();
	}

	void Unlock();

private:
	void Lock();

private:
	FPersistentManagedFile ManagedFile;
	FCriticalSection* pFileCS = nullptr;
};

enum class EPersistentStorageManagerFileSizeFlags : uint8
{
	None = 0,
	OnlyUpdateIfLess = (1 << 0),
	RespectQuota = (1 << 1)
};
ENUM_CLASS_FLAGS(EPersistentStorageManagerFileSizeFlags);

struct FPersistentStorageCategory
{
public:
	FPersistentStorageCategory(FString InCategoryName, TArray<FString> InDirectories, const int64 InQuota, const int64 InOptionalQuota)
		: CategoryName(MoveTemp(InCategoryName))
		, Directories(MoveTemp(InDirectories))
		, StorageQuota(InQuota)
		, OptionalStorageQuota(InOptionalQuota)
	{
		check(OptionalStorageQuota < StorageQuota || StorageQuota <= 0); // optional storage quota should be a subset of the total storage quota
	}

	const FString& GetCategoryName() const
	{
		return CategoryName;
	}

	int64 GetCategoryQuota( bool bIncludeOptional = true ) const
	{
		if (bIncludeOptional || StorageQuota < 0)
		{
			return StorageQuota;
		}
		else
		{
			return StorageQuota - OptionalStorageQuota;
		}
	}

	int64 GetCategoryOptionalQuota() const
	{
		return OptionalStorageQuota;
	}

	int64 GetUsedSize() const
	{
		return UsedQuota;
	}

	int64 GetAvailableSize(bool bIncludeOptionalStorage = true) const
	{
		int64 ActualStorageQuota = StorageQuota;
		if (ActualStorageQuota >= 0)
		{
			if (!bIncludeOptionalStorage)
			{
				ActualStorageQuota -= OptionalStorageQuota;
			}
		}
		else
		{
			ActualStorageQuota = MAX_int64;
		}
		return ActualStorageQuota - UsedQuota;
	}

	bool IsInCategory(const FString& Path) const
	{
		return ShouldManageFile(Path);
	}

	bool IsCategoryFull() const
	{
		return GetAvailableSize() <= 0;
	}

	EPersistentStorageManagerFileSizeFlags AddOrUpdateFile(const FString& Filename, const int64 FileSize, EPersistentStorageManagerFileSizeFlags Flags)
	{
		uint32 KeyHash = GetTypeHash(Filename);

		int64 OldFileSize = 0;
		{
			FReadScopeLock ScopeLock(FileSizesLock);
			int64* pOldFileSize = FileSizes.FindByHash(KeyHash, Filename);
			if (pOldFileSize)
			{
				OldFileSize = *pOldFileSize;
			}
		}

		EPersistentStorageManagerFileSizeFlags Result = TryUpdateQuota(OldFileSize, FileSize, Flags);
		if (Result == EPersistentStorageManagerFileSizeFlags::None)
		{
			FWriteScopeLock ScopeLock(FileSizesLock);
			FileSizes.AddByHash(KeyHash, Filename, FileSize);

			UE_LOG(LogPlatformFileManagedStorage, Log, TEXT("File %s is added to category %s"), *Filename, *CategoryName);
		}

		return Result;
	}

	bool RemoveFile(const FString& Filename)
	{
		FileSizesLock.WriteLock(); // FWriteScopeLock doesn't have an early unlock :(

		int64 OldSize;
		if (FileSizes.RemoveAndCopyValue(Filename, OldSize))
		{
			FileSizesLock.WriteUnlock();

			UsedQuota -= OldSize;

			UE_LOG(LogPlatformFileManagedStorage, Log, TEXT("File %s is removed from category %s"), *Filename, *CategoryName);

			return true;
		}

		FileSizesLock.WriteUnlock();
		return false;
	}

	const TArray<FString>& GetDirectories() const { return Directories; }

	struct CategoryStat
	{
		FString Print() const
		{
			if (TotalSize < 0)
			{
				return FString::Printf(TEXT("Category %s: %.3f MiB (%" INT64_FMT ") / unlimited used"), *CategoryName, (float)UsedSize / 1024.f / 1024.f, UsedSize);
			}
			else
			{
				return FString::Printf(TEXT("Category %s: %.3f MiB (%" INT64_FMT ") / %.3f MiB used"), *CategoryName, (float)UsedSize / 1024.f / 1024.f, UsedSize, (float)TotalSize / 1024.f / 1024.f);
			}
		}

		FString CategoryName;
		int64 UsedSize = 0;
		int64 TotalSize = -1;
		TMap<FString, int64> FileSizes;
		TArray<FString> Directories;
	};

	// Note this will not be accurate if the category is being modified while this is called but it is low level thread safe
	CategoryStat GetStat() const
	{
		FReadScopeLock ScopeLock(FileSizesLock);
		return CategoryStat{ CategoryName, UsedQuota, StorageQuota, FileSizes, Directories };
	}

	FManagedStorageFileLockRegistry& GetLockRegistry() { return FileLockRegistry; }

private:
	EPersistentStorageManagerFileSizeFlags TryUpdateQuota(const int64 OldFileSize, const int64 NewFileSize, EPersistentStorageManagerFileSizeFlags Flags)
	{
		check(OldFileSize >= 0);
		check(NewFileSize >= 0);

		if (NewFileSize <= OldFileSize)
		{
			UsedQuota -= (OldFileSize - NewFileSize);
			return EPersistentStorageManagerFileSizeFlags::None;
		}

		if (EnumHasAnyFlags(Flags, EPersistentStorageManagerFileSizeFlags::OnlyUpdateIfLess))
		{
			return EPersistentStorageManagerFileSizeFlags::OnlyUpdateIfLess;
		}

		if (EnumHasAnyFlags(Flags, EPersistentStorageManagerFileSizeFlags::RespectQuota) && StorageQuota >= 0)
		{
			int64 OldUsedQuota = UsedQuota;
			int64 NewUsedQuota;
			do
			{
				NewUsedQuota = OldUsedQuota + (NewFileSize - OldFileSize);
				if (NewUsedQuota > StorageQuota)
				{
					return EPersistentStorageManagerFileSizeFlags::RespectQuota;
				}
			} while (!UsedQuota.compare_exchange_weak(OldUsedQuota, NewUsedQuota));

			return EPersistentStorageManagerFileSizeFlags::None;
		}

		UsedQuota += (NewFileSize - OldFileSize);
		return EPersistentStorageManagerFileSizeFlags::None;
	}

	bool ShouldManageFile(const FString& Filename) const
	{
		for (const FString& Directory : Directories)
		{
			if (ManagedStorageInternal::IsUnderDirectory(Filename, Directory))
			{
				return true;
			}
		}

		return false;
	}

private:
	const FString CategoryName;
	const TArray<FString> Directories;

	const int64 StorageQuota = -1;
	const int64 OptionalStorageQuota = 0;

	std::atomic<int64> UsedQuota{ 0 };

	TMap<FString, int64> FileSizes;
	mutable FRWLock FileSizesLock;

	FManagedStorageFileLockRegistry FileLockRegistry;
};

// NOTE: CORE_API is not used on the whole class because then FCategoryInfo is exported which appears to force the generation
// of copy constructors for FPersistentStorageCategory which causes a compile error because std::atomic can't be copied.
class FPersistentStorageManager
{
public:
	static bool IsReady()
	{
		// FPersistentStorageManager depends on FPaths which depends on the command line being initialized.
		// FPersistentStorageManager depends on GConfig.
		// FPersistentStorageManager can't be constructed until its dependencies are ready
		return GConfig && GConfig->IsReadyForUse() && FCommandLine::IsInitialized();
	}

	/** Singleton access **/
	static CORE_API FPersistentStorageManager& Get();

	FPersistentStorageManager();

	void CORE_API Initialize()
	{
		if (bInitialized)
		{
			return;
		}

		// Check to add files
		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, []()
		{
			class FInitStorageVisitor : public IPlatformFile::FDirectoryVisitor
			{
			public:
				FInitStorageVisitor() : IPlatformFile::FDirectoryVisitor(EDirectoryVisitorFlags::ThreadSafe) // Go wide with parallel file scan
				{}

				virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
				{
					if (FilenameOrDirectory && !bIsDirectory)
					{
						FPersistentStorageManager& Man = FPersistentStorageManager::Get();
						if (FPersistentManagedFile File = Man.TryManageFile(FilenameOrDirectory))
						{
							FManagedStorageScopeFileLock ScopeFileLock(File);

							// This must be done under the lock because another thread may be modifying or deleting the file while we scan
							FFileStatData StatData = IPlatformFile::GetPlatformPhysical().GetStatData(FilenameOrDirectory);
							if (ensure(StatData.bIsValid))
							{
								Man.AddOrUpdateFile(File, StatData.FileSize);
							}
						}
					}

					return true;
				}
			};

			FInitStorageVisitor Visitor;

			for (const FString& RootDir : FPersistentStorageManager::Get().GetRootDirectories())
			{
				UE_LOG(LogPlatformFileManagedStorage, Log, TEXT("Scan directory %s"), *RootDir);

				IPlatformFile::GetPlatformPhysical().IterateDirectoryRecursively(*RootDir, Visitor);
			}
		});

		bInitialized = true;
	}

	FPersistentManagedFile TryManageFile(const FString& Filename)
	{
		FPersistentManagedFile OutFile;
		OutFile.FullFilename = FPaths::ConvertRelativePathToFull(Filename);

		TryManageFileInternal(OutFile);

		return OutFile;
	}

	FPersistentManagedFile TryManageFile(FString&& Filename)
	{
		FPersistentManagedFile OutFile;
		OutFile.FullFilename = FPaths::ConvertRelativePathToFull(MoveTemp(Filename));

		TryManageFileInternal(OutFile);

		return OutFile;
	}

private:
	void TryManageFileInternal(FPersistentManagedFile& OutFile)
	{
		int32 CategoryIndex = 0;
		for (const FPersistentStorageCategory& Category : Categories.GetCategories())
		{
			if (Category.IsInCategory(OutFile.FullFilename))
			{
				OutFile.Category = CategoryIndex;
				return;
			}
			++CategoryIndex;
		}

		bool bIsUnderRootDir = !!Algo::FindByPredicate(RootDirectories.GetRootDirectories(), [&OutFile](const FString& RootDir) { return ManagedStorageInternal::IsUnderDirectory(OutFile.FullFilename, RootDir); });
		if (bIsUnderRootDir)
		{
			OutFile.Category = Categories.GetDefaultCategoryIndex();
		}
	}

public:
	EPersistentStorageManagerFileSizeFlags AddOrUpdateFile(const FPersistentManagedFile& File, const int64 FileSize, 
		EPersistentStorageManagerFileSizeFlags Flags = EPersistentStorageManagerFileSizeFlags::None)
	{
		if (!File)
		{
			return EPersistentStorageManagerFileSizeFlags::None;
		}

		return Categories.GetCategories()[File.Category].AddOrUpdateFile(File.FullFilename, FileSize, Flags);
	}

	bool RemoveFileFromManager(FPersistentManagedFile& File)
	{
		if (!File)
		{
			return false;
		}

		return Categories.GetCategories()[File.Category].RemoveFile(File.FullFilename);
		File.Clear();
	}

	int64 GetTotalUsedSize() const
	{
		int64 TotalUsedSize = 0LL;
		for(const FPersistentStorageCategory& Category : Categories.GetCategories())
		{
			TotalUsedSize += Category.GetUsedSize();
		}

		return TotalUsedSize;
	}

	/// <summary>
	/// 
	/// </summary>
	/// <param name="Path">path to check for free space</param>
	/// <param name="UsedSpace">amount of used space</param>
	/// <param name="RemainingSpace">amount of remaining free space</param>
	/// <param name="Quota">total storage allocated to the related category</param>
	/// <param name="OptionalQuota">subset of the storage which is optional i.e. will always be smaller then total Quota</param>
	/// <returns>returns if function succeeds</returns>
	bool GetPersistentStorageUsage(FString Path, int64& UsedSpace, int64 &RemainingSpace, int64& Quota, int64* OptionalQuota = nullptr)
	{
		FString FullPath = FPaths::ConvertRelativePathToFull(MoveTemp(Path));

		for (const FPersistentStorageCategory& Category : Categories.GetCategories())
		{
			if (Category.IsInCategory(FullPath))
			{
				UsedSpace = Category.GetUsedSize();
				RemainingSpace = Category.GetAvailableSize();
				Quota = Category.GetCategoryQuota();
				if (OptionalQuota != nullptr)
				{
					*OptionalQuota = Category.GetCategoryOptionalQuota();
				}
				return true;
			}
		}
		return false;
	}

	bool GetPersistentStorageUsageByCategory(const FString& InCategory, int64& UsedSpace, int64& RemainingSpace, int64& Quota, int64* OptionalQuota = nullptr)
	{
		FPersistentStorageCategory* Category = Algo::FindBy(Categories.GetCategories(), InCategory, [](const FPersistentStorageCategory& Cat) { return Cat.GetCategoryName(); });
		if (Category)
		{
			UsedSpace = Category->GetUsedSize();
			RemainingSpace = Category->GetAvailableSize();
			Quota = Category->GetCategoryQuota();
			if (OptionalQuota != nullptr)
			{
				*OptionalQuota = Category->GetCategoryOptionalQuota();
			}
			return true;
		}
		return false;
	}

	/// <summary>
	/// Returns any additional required free space and optional free space
	/// </summary>
	/// <param name="RequiredSpace">Required persistent storage space to run</param>
	/// <param name="OptionalSpace">persistent storage categories marked as optional</param>
	/// <returns></returns>
	bool GetPersistentStorageSize(int64& UsedSpace, int64& RequiredSpace, int64& OptionalSpace) const
	{
		for (const FPersistentStorageCategory& Category : Categories.GetCategories())
		{
			UsedSpace += Category.GetUsedSize();
			RequiredSpace += Category.GetCategoryQuota();
			OptionalSpace += Category.GetCategoryOptionalQuota();
		}
		return true;
	}

	bool IsInitialized() const
	{
		return bInitialized;
	}

	bool IsCategoryForFileFull(const FPersistentManagedFile& File) const
	{
		if (!File)
		{
			return false;
		}

		return Categories.GetCategories()[File.Category].IsCategoryFull();
	}

	TMap<FString, FPersistentStorageCategory::CategoryStat> GenerateCategoryStats() const
	{
		TMap<FString, FPersistentStorageCategory::CategoryStat> CategoryStats;
		CategoryStats.Reserve(Categories.GetCategories().Num());

		for (const FPersistentStorageCategory& Category : Categories.GetCategories())
		{
			CategoryStats.Add(Category.GetCategoryName(), Category.GetStat());
		}

		return CategoryStats;
	}

	TOptional<FPersistentStorageCategory::CategoryStat> GetCategoryStat(const FString& InCategory) const
	{
		TOptional<FPersistentStorageCategory::CategoryStat> Result;

		for (const FPersistentStorageCategory& Category : Categories.GetCategories())
		{
			if (Category.GetCategoryName() == InCategory)
			{
				Result.Emplace(Category.GetStat());
				break;
			}
		}

		return Result;
	}

	TArrayView<const FString> GetRootDirectories() const { return RootDirectories.GetRootDirectories(); }

private:
	friend class FManagedStorageScopeFileLock; // For access to Categories

	bool bInitialized;

	// Top level of all managed storage
	// Wrapper to prevent changing or resizing after init
	struct FRootDirInfo
	{
	private:
		TArray<FString> RootDirectories;

	public:
		void Init(TArrayView<const FPersistentStorageCategory> Categories);
		TArrayView<const FString> GetRootDirectories() const { return MakeArrayView(RootDirectories); }
	};
	FRootDirInfo RootDirectories;

	// Wrapper for Category Array to prevent resizing after init
	struct FCategoryInfo
	{
	private:
		TArray<FPersistentStorageCategory> Categories;
		int32 DefaultCategory = -1;

	public:
		FCategoryInfo(TArray<FPersistentStorageCategory>&& InCategories, int32 InDefaultCategory);

		TArrayView<FPersistentStorageCategory> GetCategories() { return MakeArrayView(Categories); }
		TArrayView<const FPersistentStorageCategory> GetCategories() const { return MakeArrayView(Categories); }

		FPersistentStorageCategory* GetDefaultCategory() { return DefaultCategory >= 0 ? &Categories[DefaultCategory] : nullptr; }
		const FPersistentStorageCategory* GetDefaultCategory() const { return DefaultCategory >= 0 ? &Categories[DefaultCategory] : nullptr; }

		int32 GetDefaultCategoryIndex() const { return DefaultCategory; }
	};
	FCategoryInfo Categories;

	static FCategoryInfo InitCategories();
};

// Only write handle 
class CORE_API FManagedStorageFileWriteHandle : public IFileHandle
{
private:
	static bool IsReady()
	{
		// FManagedStoragePlatformFile will just pass through to LowerLevel until FPersistentStorageManager is ready
		return FPersistentStorageManager::IsReady();
	}

	bool TryManageFile()
	{
		if (!File && IsReady())
		{
			File = FPersistentStorageManager::Get().TryManageFile(File.FullFilename);
		}

		return File.IsValid();
	}

public:
	FManagedStorageFileWriteHandle(IFileHandle* InFileHandle, FPersistentManagedFile InFile)
		: FileHandle(InFileHandle)
		, File(MoveTemp(InFile))
	{
	}

	virtual ~FManagedStorageFileWriteHandle()
	{
		if (!TryManageFile())
		{
			return;
		}

		FManagedStorageScopeFileLock ScopeFileLock(File);

		FPersistentStorageManager::Get().AddOrUpdateFile(File, FileHandle->Size());
	}

	virtual int64 Tell() override
	{
		return FileHandle->Tell();
	}

	virtual bool Seek(int64 NewPosition) override
	{
		return FileHandle->Seek(NewPosition);
	}

	virtual bool SeekFromEnd(int64 NewPositionRelativeToEnd = 0) override
	{
		return FileHandle->SeekFromEnd(NewPositionRelativeToEnd);
	}

	virtual bool Read(uint8* Destination, int64 BytesToRead) override
	{
		return FileHandle->Read(Destination, BytesToRead);
	}

	virtual bool Write(const uint8* Source, int64 BytesToWrite) override
	{
		if (!TryManageFile())
		{
			return FileHandle->Write(Source, BytesToWrite);
		}

		FPersistentStorageManager& Manager = FPersistentStorageManager::Get();

		FManagedStorageScopeFileLock ScopeFileLock(File);

		int64 NewSize = FMath::Max(FileHandle->Tell() + BytesToWrite, FileHandle->Size());

		EPersistentStorageManagerFileSizeFlags Result = Manager.AddOrUpdateFile(File, NewSize, EPersistentStorageManagerFileSizeFlags::RespectQuota);
		bool bIsFileCategoryFull = Result != EPersistentStorageManagerFileSizeFlags::None;
		if (bIsFileCategoryFull)
		{
			UE_LOG(LogPlatformFileManagedStorage, Error, TEXT("Failed to write to file %s.  The category of the file has reach quota limit in peristent storage."), *File.FullFilename);
			return false;
		}

		bool bSuccess = FileHandle->Write(Source, BytesToWrite);
		if (!bSuccess)
		{
			int64 FileSize = FileHandle->Size();
			if (ensureAlways(FileSize >= 0))
			{
				verify(Manager.AddOrUpdateFile(File, FileSize) == EPersistentStorageManagerFileSizeFlags::None);
			}
		}

		return bSuccess;
	}

	virtual int64 Size() override
	{
		return FileHandle->Size();
	}

	virtual bool Flush(const bool bFullFlush = false) override
	{
		const bool bOldIsValid = File.IsValid();
		if (!TryManageFile())
		{
			return FileHandle->Flush(bFullFlush);
		}

		FManagedStorageScopeFileLock ScopeFileLock(File);

		const bool bSuccess = FileHandle->Flush(bFullFlush);
		const bool bForceSizeUpdate = !bOldIsValid;
		if (!bSuccess || bForceSizeUpdate)
		{
			int64 FileSize = FileHandle->Size();
			if (ensureAlways(FileSize >= 0))
			{
				verify(FPersistentStorageManager::Get().AddOrUpdateFile(File, FileSize) == EPersistentStorageManagerFileSizeFlags::None);
			}
		}

		return bSuccess;
	}

	virtual bool Truncate(int64 NewSize) override
	{
		if (!TryManageFile())
		{
			return FileHandle->Truncate(NewSize);
		}

		FPersistentStorageManager& Manager = FPersistentStorageManager::Get();

		FManagedStorageScopeFileLock ScopeFileLock(File);

		int64 FileSize = FileHandle->Size();

		if (NewSize <= FileSize)
		{
			bool bSuccess = FileHandle->Truncate(NewSize);
			FileSize = FileHandle->Size();
			verify(Manager.AddOrUpdateFile(File, FileSize) == EPersistentStorageManagerFileSizeFlags::None);
			return bSuccess;
		}

		if (Manager.AddOrUpdateFile(File, NewSize, EPersistentStorageManagerFileSizeFlags::RespectQuota) != EPersistentStorageManagerFileSizeFlags::None)
		{
			UE_LOG(LogPlatformFileManagedStorage, Error, TEXT("Failed to truncate file %s.  The category of the file has reach quota limit in peristent storage."), *File.FullFilename);
			return false;
		}

		if (!FileHandle->Truncate(NewSize))
		{
			FileSize = FileHandle->Size();
			verify(Manager.AddOrUpdateFile(File, FileSize) == EPersistentStorageManagerFileSizeFlags::None);
			return false;
		}

		return true;
	}

	virtual void ShrinkBuffers() override
	{
		FileHandle->ShrinkBuffers();
	}

private:
	TUniquePtr<IFileHandle>	FileHandle;
	FPersistentManagedFile	File;
};

// NOTE: This is templated rather than a polymorphic wrapper because a lot code expects the physical layer not to be a wrapper.
// It also has the benefit of not needing updating every time a new function is added to IPlatformFile.
template<class BaseClass>
class CORE_API TManagedStoragePlatformFile : public BaseClass
{
private:
	static bool IsReady()
	{
		// FManagedStoragePlatformFile will just pass through to LowerLevel until FPersistentStorageManager is ready
		return FPersistentStorageManager::IsReady();
	}

public:

	TManagedStoragePlatformFile() : BaseClass()
	{
		// NOTE: using LowLevelFatalError here because UE_LOG is not yet available at static init time

		// Book keeping is handled by the base implementation calling DeleteFile()
		bool bUsingGenericDeleteDirectoryRecursively = (&BaseClass::DeleteDirectoryRecursively == &IPlatformFile::DeleteDirectoryRecursively);
		check(bUsingGenericDeleteDirectoryRecursively);
		if (!bUsingGenericDeleteDirectoryRecursively)
		{
			LowLevelFatalError(TEXT("TManagedStoragePlatformFile cannot track all deletes!"));
		}

		// Book keeping is handled by DeleteFile() and CopyFile() which will be called by the base implementation
		bool bUsingGenericCopyDirectoryTree = (&BaseClass::CopyDirectoryTree == &IPlatformFile::CopyDirectoryTree);
		check(bUsingGenericCopyDirectoryTree);
		if (!bUsingGenericCopyDirectoryTree)
		{
			LowLevelFatalError(TEXT("TManagedStoragePlatformFile cannot track all deletes and copies!"));
		}
	}

	virtual bool DeleteFile(const TCHAR* Filename) override
	{
		if (!IsReady())
		{
			return BaseClass::DeleteFile(Filename);
		}

		FPersistentStorageManager& Manager = FPersistentStorageManager::Get();

		FPersistentManagedFile ManagedFile = Manager.TryManageFile(Filename);

		FManagedStorageScopeFileLock ScopeFileLock(ManagedFile);

		bool bSuccess = BaseClass::DeleteFile(Filename);
		if (bSuccess)
		{
			Manager.RemoveFileFromManager(ManagedFile);
		}

		return bSuccess;
	}

	virtual bool MoveFile(const TCHAR* To, const TCHAR* From) override
	{
		if (!IsReady())
		{
			return BaseClass::MoveFile(To, From);
		}

		FPersistentStorageManager& Manager = FPersistentStorageManager::Get();

		FPersistentManagedFile ManagedTo = Manager.TryManageFile(To);
		FPersistentManagedFile ManagedFrom = Manager.TryManageFile(From);

		FManagedStorageScopeFileLock ScopeFileLockTo(ManagedTo);
		FManagedStorageScopeFileLock ScopeFileLockFrom(ManagedFrom);

		int64 Size = this->FileSize(From);
		if (Size < 0)
		{
			return false;
		}

		EPersistentStorageManagerFileSizeFlags Result = Manager.AddOrUpdateFile(ManagedTo, Size, EPersistentStorageManagerFileSizeFlags::OnlyUpdateIfLess | EPersistentStorageManagerFileSizeFlags::RespectQuota);
		if (EnumHasAnyFlags(Result, EPersistentStorageManagerFileSizeFlags::RespectQuota))
		{
			UE_LOG(LogPlatformFileManagedStorage, Error, TEXT("Failed to move file to %s.  The target category of the destination has reach quota limit in peristent storage."), To);
			return false;
		}

		bool bSuccess = BaseClass::MoveFile(To, From);
		if (bSuccess)
		{
			Manager.RemoveFileFromManager(ManagedFrom);
			Manager.AddOrUpdateFile(ManagedTo, Size);
		}
		else
		{
			Manager.RemoveFileFromManager(ManagedTo);
		}

		return bSuccess;
	}

	virtual IFileHandle* OpenWrite(const TCHAR* Filename, bool bAppend = false, bool bAllowRead = false) override
	{
		if (!IsReady())
		{
			// Always wrap handle if not ready.  FManagedStorageFileWriteHandle will start managing the file
			// internally when we become ready.
			IFileHandle* InnerHandle = BaseClass::OpenWrite(Filename, bAppend, bAllowRead);
			if (!InnerHandle)
			{
				return nullptr;
			}

			FPersistentManagedFile ManagedFile;
			ManagedFile.FullFilename = FPaths::ConvertRelativePathToFull(Filename);
			return new FManagedStorageFileWriteHandle(InnerHandle, MoveTemp(ManagedFile));
		}

		FPersistentStorageManager& Manager = FPersistentStorageManager::Get();

		FPersistentManagedFile ManagedFile = Manager.TryManageFile(Filename);

		if (Manager.IsCategoryForFileFull(ManagedFile))
		{
			UE_LOG(LogPlatformFileManagedStorage, Error, TEXT("Failed to open file %s for write.  The category of the file has reach quota limit in peristent storage."), Filename);
			return nullptr;
		}

		FManagedStorageScopeFileLock ScopeFileLock(ManagedFile);

		IFileHandle* InnerHandle = BaseClass::OpenWrite(Filename, bAppend, bAllowRead);
		if (!InnerHandle)
		{
			return nullptr;
		}

		Manager.AddOrUpdateFile(ManagedFile, InnerHandle->Size());
		if (ManagedFile)
		{
			return new FManagedStorageFileWriteHandle(InnerHandle, MoveTemp(ManagedFile));
		}
		else
		{
			return InnerHandle;
		}
	}

	virtual bool CopyFile(const TCHAR* To, const TCHAR* From, EPlatformFileRead ReadFlags = EPlatformFileRead::None, EPlatformFileWrite WriteFlags = EPlatformFileWrite::None) override
	{
		if (!IsReady())
		{
			return BaseClass::CopyFile(To, From, ReadFlags, WriteFlags);
		}

		FPersistentStorageManager& Manager = FPersistentStorageManager::Get();

		FPersistentManagedFile ManagedTo = Manager.TryManageFile(To);
		FPersistentManagedFile ManagedFrom = Manager.TryManageFile(From);

		FManagedStorageScopeFileLock ScopeFileLockTo(ManagedTo);
		FManagedStorageScopeFileLock ScopeFileLockFrom(ManagedFrom);

		int64 Size = this->FileSize(From);
		if (Size < 0)
		{
			return false;
		}

		EPersistentStorageManagerFileSizeFlags Result = Manager.AddOrUpdateFile(ManagedTo, Size, EPersistentStorageManagerFileSizeFlags::OnlyUpdateIfLess | EPersistentStorageManagerFileSizeFlags::RespectQuota);
		if (EnumHasAnyFlags(Result, EPersistentStorageManagerFileSizeFlags::RespectQuota))
		{
			UE_LOG(LogPlatformFileManagedStorage, Error, TEXT("Failed to copy file to %s.  The category of the destination has reach quota limit in peristent storage."), To);
			return false;
		}

		bool bSuccess = BaseClass::CopyFile(To, From, ReadFlags, WriteFlags);
		if (bSuccess)
		{
			Manager.AddOrUpdateFile(ManagedTo, Size);
		}
		else
		{
			if (!this->FileExists(To))
			{
				Manager.RemoveFileFromManager(ManagedTo);
			}
			else
			{
				Size = this->FileSize(To);
				if (Size < 0)
				{
					Manager.RemoveFileFromManager(ManagedTo);
				}
				else
				{
					Manager.AddOrUpdateFile(ManagedTo, Size);
				}
			}
		}

		return bSuccess;
	}
};