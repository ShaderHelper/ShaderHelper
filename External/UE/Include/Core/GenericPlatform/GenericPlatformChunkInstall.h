// Copyright Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	GenericPlatformChunkInstall.h: Generic platform chunk based install classes.
==============================================================================================*/

#pragma once

#include "CoreTypes.h"
#include "Logging/LogMacros.h"
#include "Delegates/IDelegateInstance.h"
#include "Delegates/Delegate.h"
#include "Modules/ModuleInterface.h"

class IPlatformChunkInstall;

DECLARE_LOG_CATEGORY_EXTERN(LogChunkInstaller, Log, All);

namespace EChunkLocation
{
	enum Type
	{
		// note: higher quality locations should have higher enum values, we sort by these in AssetRegistry.cpp
		DoesNotExist,	// chunk does not exist
		NotAvailable,	// chunk has not been installed yet
		LocalSlow,		// chunk is on local slow media (optical)
		LocalFast,		// chunk is on local fast media (HDD)

		BestLocation=LocalFast
	};
}


namespace EChunkInstallSpeed
{
	enum Type
	{
		Paused,					// chunk installation is paused
		Slow,					// installation is lower priority than Game IO
		Fast					// installation is higher priority than Game IO
	};
}

namespace EChunkPriority
{
	enum Type
	{
		Immediate,	// Chunk install is of highest priority, this can cancel lower priority installs.
		High	 ,	// Chunk is probably required soon so grab is as soon as possible.
		Low		 ,	// Install this chunk only when other chunks are not needed.
	};
}

namespace EChunkProgressReportingType
{
	enum Type
	{
		ETA,					// time remaining in seconds
		PercentageComplete		// percentage complete in 99.99 format
	};
}

/**
 * Platform Chunk Install Module Interface
 */
class IPlatformChunkInstallModule : public IModuleInterface
{
public:

	virtual IPlatformChunkInstall* GetPlatformChunkInstall() = 0;
};

/** Deprecated delegate */
DECLARE_DELEGATE_OneParam(FPlatformChunkInstallCompleteDelegate, uint32);

/** Delegate called when a chunk either successfully installs or fails to install, bool is success */
DECLARE_DELEGATE_TwoParams(FPlatformChunkInstallDelegate, uint32, bool);
DECLARE_MULTICAST_DELEGATE_TwoParams(FPlatformChunkInstallMultiDelegate, uint32, bool);

enum class ECustomChunkType : uint8
{
	OnDemandChunk,
	LanguageChunk
};

struct FCustomChunk
{
	FString ChunkTag;
	uint32	ChunkID;
	ECustomChunkType ChunkType;

	FCustomChunk(FString InTag, uint32 InID, ECustomChunkType InChunkType) :
		ChunkTag(InTag), ChunkID(InID), ChunkType(InChunkType)
	{}
};

struct FCustomChunkMapping
{
	enum class CustomChunkMappingType : uint8
	{
		Main,
		Optional
	};

	FString Pattern;
	uint32	ChunkID;
	CustomChunkMappingType MappingType;

	FCustomChunkMapping(FString InPattern, uint32 InChunkID, CustomChunkMappingType InMappingType) :
		Pattern(InPattern), ChunkID(InChunkID), MappingType(InMappingType)
	{}
};

/**
* Interface for platform specific chunk based install
**/
class CORE_API IPlatformChunkInstall
{
public:

	/** Virtual destructor */
	virtual ~IPlatformChunkInstall() {}

	/**
	 * Get the current location of a chunk with pakchunk index.
	 * @param PakchunkIndex	The id of the pak chunk.
	 * @return				Enum specifying whether the chunk is available to use, waiting to install, or does not exist.
	 **/
	virtual EChunkLocation::Type GetPakchunkLocation( int32 PakchunkIndex) = 0;

	/** 
	 * Check if a given reporting type is supported.
	 * @param ReportType	Enum specifying how progress is reported.
	 * @return				true if reporting type is supported on the current platform.
	 **/
	virtual bool GetProgressReportingTypeSupported(EChunkProgressReportingType::Type ReportType) = 0;		

	/**
	 * Get the current install progress of a chunk.  Let the user specify report type for platforms that support more than one.
	 * @param ChunkID		The id of the chunk to check.
	 * @param ReportType	The type of progress report you want.
	 * @return				A value whose meaning is dependent on the ReportType param.
	 **/
	virtual float GetChunkProgress( uint32 ChunkID, EChunkProgressReportingType::Type ReportType ) = 0;

	/**
	 * Inquire about the priority of chunk installation vs. game IO.
	 * @return				Paused, low or high priority.
	 **/
	virtual EChunkInstallSpeed::Type GetInstallSpeed() = 0;
	/**
	 * Specify the priority of chunk installation vs. game IO.
	 * @param InstallSpeed	Pause, low or high priority.
	 * @return				false if the operation is not allowed, otherwise true.
	 **/
	virtual bool SetInstallSpeed( EChunkInstallSpeed::Type InstallSpeed ) = 0;
	
	/**
	 * Hint to the installer that we would like to prioritize a specific chunk
	 * @param PakchunkIndex	The index of the pakchunk to prioritize.
	 * @param Priority		The priority for the chunk.
	 * @return				false if the operation is not allowed or the chunk doesn't exist, otherwise true.
	 **/
	virtual bool PrioritizePakchunk( int32 PakchunkIndex, EChunkPriority::Type Priority ) = 0;

	/**
	 * For platforms that support emulation of the Chunk install.  Starts transfer of the next chunk.
	 * Does nothing in a shipping build.
	 * @return				true if the operation succeeds.
	 **/
	virtual bool DebugStartNextChunk() = 0;

	/**
	 * Allow an external system to notify that a particular chunk ID has become available
	 * Initial use-case is for dynamically encrypted pak files to signal to the outside world that
	 * it has become available.
	 *
	 * @param InChunkID - ID of the chunk that has just become available
	 */
	virtual void ExternalNotifyChunkAvailable(uint32 InChunkID) = 0;

	/** 
	 * Request a delegate callback on chunk install completion or failure. Request may not be respected.
	 * @param Delegate		The delegate to call when any chunk is installed or fails to install
	 * @return				Handle to the bound delegate
	 */
	virtual FDelegateHandle AddChunkInstallDelegate( FPlatformChunkInstallDelegate Delegate ) = 0;

	/**
	 * Remove a delegate callback on chunk install completion.
	 * @param Delegate		The delegate to remove.
	 */
	virtual void RemoveChunkInstallDelegate( FDelegateHandle Delegate ) = 0;

	UE_DEPRECATED(4.18, "Call AddChunkInstallDelegate instead, which is now bound for all chunk ids")
	virtual FDelegateHandle SetChunkInstallDelgate( uint32 ChunkID, FPlatformChunkInstallCompleteDelegate Delegate ) = 0;

	UE_DEPRECATED(4.18, "Call RemoveChunkInstallDelegate instead")
	virtual void RemoveChunkInstallDelgate( uint32 ChunkID, FDelegateHandle Delegate ) = 0;

	/**
	* Check whether current platform supports intelligent chunk installation
	* @return				whether Intelligent Install is supported
	*/
	virtual bool SupportsIntelligentInstall() = 0;

	/**
	* Check whether installation of chunks are pending
	* @return				whether installation task has been kicked
	*/
	virtual bool IsChunkInstallationPending(const TArray<FCustomChunk>& ChunkTagsID) = 0;

	/**
	* Install chunks with Intelligent Delivery API
	* @return				whether installation task has been kicked
	*/
	virtual bool InstallChunks(const TArray<FCustomChunk>& ChunkTagsID) = 0;

	/**
	* Uninstall chunks with Intelligent Delivery API
	* @return				whether uninstallation task has been kicked
	*/
	virtual bool UninstallChunks(const TArray<FCustomChunk>& ChunkTagsID) = 0;

protected:
		/**
		 * Get the current location of a chunk.
		 * Pakchunk index and platform chunk id are not always the same.  Call GetPakchunkLocation instead of calling from outside.
		 * @param ChunkID		The id of the chunk to check.
		 * @return				Enum specifying whether the chunk is available to use, waiting to install, or does not exist.
		 **/
		virtual EChunkLocation::Type GetChunkLocation(uint32 ChunkID) = 0;

		/**
		 * Hint to the installer that we would like to prioritize a specific chunk
		 * @param ChunkID		The id of the chunk to prioritize.
		 * @param Priority		The priority for the chunk.
		 * @return				false if the operation is not allowed or the chunk doesn't exist, otherwise true.
		 **/
		virtual bool PrioritizeChunk(uint32 ChunkID, EChunkPriority::Type Priority) = 0;

};

PRAGMA_DISABLE_DEPRECATION_WARNINGS
/**
 * Generic implementation of chunk based install
 */
class CORE_API FGenericPlatformChunkInstall : public IPlatformChunkInstall
{
public:
	virtual EChunkLocation::Type GetPakchunkLocation( int32 PakchunkIndex ) override
	{
		return GetChunkLocation(PakchunkIndex);
	}

	virtual bool PrioritizePakchunk(int32 PakchunkIndex, EChunkPriority::Type Priority)
	{
		return PrioritizeChunk(PakchunkIndex, Priority);
	}

	virtual bool GetProgressReportingTypeSupported(EChunkProgressReportingType::Type ReportType) override
	{
		if (ReportType == EChunkProgressReportingType::PercentageComplete)
		{
			return true;
		}

		return false;
	}

	virtual float GetChunkProgress( uint32 ChunkID, EChunkProgressReportingType::Type ReportType ) override
	{
		if (ReportType == EChunkProgressReportingType::PercentageComplete)
		{
			return 100.0f;
		}		
		return 0.0f;
	}

	virtual EChunkInstallSpeed::Type GetInstallSpeed() override
	{
		return EChunkInstallSpeed::Paused;
	}

	virtual bool SetInstallSpeed( EChunkInstallSpeed::Type InstallSpeed ) override
	{
		return false;
	}
	
	virtual bool PrioritizeChunk( uint32 ChunkID, EChunkPriority::Type Priority ) override
	{
		return false;
	}

	virtual bool DebugStartNextChunk() override
	{
		return true;
	}

	virtual void ExternalNotifyChunkAvailable(uint32 InChunkID) override
	{
		InstallDelegate.Broadcast(InChunkID, true);
	}

	virtual FDelegateHandle AddChunkInstallDelegate(FPlatformChunkInstallDelegate Delegate) override
	{
		return InstallDelegate.Add(Delegate);
	}

	virtual void RemoveChunkInstallDelegate(FDelegateHandle Delegate) override
	{
		InstallDelegate.Remove(Delegate);
	}

	virtual FDelegateHandle SetChunkInstallDelgate(uint32 ChunkID, FPlatformChunkInstallCompleteDelegate Delegate) override
	{
		return FDelegateHandle();
	}

	virtual void RemoveChunkInstallDelgate(uint32 ChunkID, FDelegateHandle Delegate) override
	{
		return;
	}

	virtual bool SupportsIntelligentInstall() override
	{
		return false;
	}

	virtual bool IsChunkInstallationPending(const TArray<FCustomChunk>& ChunkTags) override
	{
		return false;
	}

	virtual bool InstallChunks(const TArray<FCustomChunk>& ChunkTagIDs) override
	{
		return false;
	}

	virtual bool UninstallChunks(const TArray<FCustomChunk>& ChunkTagsID) override
	{
		return false;
	}

protected:

	/** Delegate called when installation succeeds or fails */
	FPlatformChunkInstallMultiDelegate InstallDelegate;

	virtual EChunkLocation::Type GetChunkLocation(uint32 ChunkID) override
	{
		return EChunkLocation::LocalFast;
	}
};

PRAGMA_ENABLE_DEPRECATION_WARNINGS