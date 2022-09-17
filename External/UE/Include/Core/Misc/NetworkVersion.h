// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/UnrealString.h"
#include "Logging/LogMacros.h"
#include "Delegates/Delegate.h"

// The version number used for determining network compatibility. If zero, uses the engine compatible version.
#define ENGINE_NET_VERSION  0

// The version number used for determining replay compatibility
#define ENGINE_REPLAY_VERSION  ENGINE_NET_VERSION

CORE_API DECLARE_LOG_CATEGORY_EXTERN( LogNetVersion, Log, All );

class FNetworkReplayVersion
{
public:
	FNetworkReplayVersion() : NetworkVersion( 0 ), Changelist( 0 )
	{
	}
	FNetworkReplayVersion( const FString& InAppString, const uint32 InNetworkVersion, const uint32 InChangelist ) : AppString( InAppString ), NetworkVersion( InNetworkVersion ), Changelist( InChangelist )
	{
	}

	FString		AppString;
	uint32		NetworkVersion;
	uint32		Changelist;
};

enum EEngineNetworkVersionHistory
{
	HISTORY_INITIAL = 1,
	HISTORY_REPLAY_BACKWARDS_COMPAT = 2,			// Bump version to get rid of older replays before backwards compat was turned on officially
	HISTORY_MAX_ACTOR_CHANNELS_CUSTOMIZATION = 3,	// Bump version because serialization of the actor channels changed
	HISTORY_REPCMD_CHECKSUM_REMOVE_PRINTF = 4,		// Bump version since the way FRepLayoutCmd::CompatibleChecksum was calculated changed due to an optimization
	HISTORY_NEW_ACTOR_OVERRIDE_LEVEL = 5,			// Bump version since a level reference was added to the new actor information
	HISTORY_CHANNEL_NAMES = 6,						// Bump version since channel type is now an fname
	HISTORY_CHANNEL_CLOSE_REASON = 7,				// Bump version to serialize a channel close reason in bunches instead of bDormant
	HISTORY_ACKS_INCLUDED_IN_HEADER = 8,			// Bump version since acks are now sent as part of the header
	HISTORY_NETEXPORT_SERIALIZATION = 9,			// Bump version due to serialization change to FNetFieldExport
	HISTORY_NETEXPORT_SERIALIZE_FIX = 10,			// Bump version to fix net field export name serialization 
	HISTORY_FAST_ARRAY_DELTA_STRUCT = 11,			// Bump version to allow fast array serialization, delta struct serialization.
	HISTORY_FIX_ENUM_SERIALIZATION = 12,			// Bump version to fix enum net serialization issues.
	HISTORY_OPTIONALLY_QUANTIZE_SPAWN_INFO = 13,	// Bump version to conditionally disable quantization for Scale, Location, and Velocity when spawning network actors.
	HISTORY_JITTER_IN_HEADER = 14,					// Bump version since we added jitter clock time to packet headers and removed remote saturation
	HISTORY_CLASSNETCACHE_FULLNAME = 15,			// Bump version to use full paths in GetNetFieldExportGroupForClassNetCache
	HISTORY_REPLAY_DORMANCY = 16,					// Bump version to support dormancy properly in replays
	HISTORY_ENUM_SERIALIZATION_COMPAT = 17,			// Bump version to include enum bits required for serialization into compat checksums, as well as unify enum and byte property enum serialization
	HISTORY_SUBOBJECT_OUTER_CHAIN = 18,				// Bump version to support subobject outer chains matching on client and server
	HISTORY_HITRESULT_INSTANCEHANDLE = 19,			// Bump version to support FHitResult change of Actor to HitObjectHandle. This change was made in CL 14369221 but a net version wasn't added at the time.
	HISTORY_INTERFACE_PROPERTY_SERIALIZATION = 20,	// Bump version to support net serialization of FInterfaceProperty
	HISTORY_MONTAGE_PLAY_INST_ID_SERIALIZATION = 21,// Bump version to support net serialization of FGameplayAbilityRepAnimMontage, addition of PlayInstanceId and removal of bForcePlayBit
	HISTORY_SERIALIZE_DOUBLE_VECTORS_AS_DOUBLES	= 22,// Bump version to support net serialization of double vector types
	HISTORY_PACKED_VECTOR_LWC_SUPPORT = 23,			// Bump version to support quantized LWC FVector net serialization
	// New history items go above here.

	HISTORY_ENGINENETVERSION_PLUS_ONE,
	HISTORY_ENGINENETVERSION_LATEST = HISTORY_ENGINENETVERSION_PLUS_ONE - 1,
};

struct CORE_API FNetworkVersion
{
	/** Called in GetLocalNetworkVersion if bound */
	DECLARE_DELEGATE_RetVal( uint32, FGetLocalNetworkVersionOverride );
	static FGetLocalNetworkVersionOverride GetLocalNetworkVersionOverride;

	/** Called in IsNetworkCompatible if bound */
	DECLARE_DELEGATE_RetVal_TwoParams( bool, FIsNetworkCompatibleOverride, uint32, uint32 );
	static FIsNetworkCompatibleOverride IsNetworkCompatibleOverride;

	/** Called in GetReplayCompatibleChangelist if bound */
	DECLARE_DELEGATE_RetVal(uint32, FGetReplayCompatibleChangeListOverride);
	static FGetReplayCompatibleChangeListOverride GetReplayCompatibleChangeListOverride;

	static uint32 GetNetworkCompatibleChangelist();
	static uint32 GetReplayCompatibleChangelist();
	static uint32 GetEngineNetworkProtocolVersion();
	static uint32 GetGameNetworkProtocolVersion();
	static uint32 GetEngineCompatibleNetworkProtocolVersion();
	static uint32 GetGameCompatibleNetworkProtocolVersion();

	/**
	* Generates a version number, that by default, is based on a checksum of the engine version + project name + project version string
	* Game/project code can completely override what this value returns through the GetLocalNetworkVersionOverride delegate
	* If called with AllowOverrideDelegate=false, we will not call the game project override. (This allows projects to call base implementation in their project implementation)
	*/
	static uint32 GetLocalNetworkVersion( bool AllowOverrideDelegate=true );

	/**
	* Determine if a connection is compatible with this instance
	*
	* @param bRequireEngineVersionMatch should the engine versions match exactly
	* @param LocalNetworkVersion current version of the local machine
	* @param RemoteNetworkVersion current version of the remote machine
	*
	* @return true if the two instances can communicate, false otherwise
	*/
	static bool IsNetworkCompatible( const uint32 LocalNetworkVersion, const uint32 RemoteNetworkVersion );

	/**
	* Generates a special struct that contains information to send to replay server
	*/
	static FNetworkReplayVersion GetReplayVersion();

	/**
	* Sets the project version used for networking. Needs to be a function to verify
	* string and correctly invalidate cached values
	* 
	* @param  InVersion
	* @return void
	*/
	static void SetProjectVersion(const TCHAR* InVersion);

	/**
	* Sets the game network protocol version used for networking and invalidate cached values
	*/
	static void SetGameNetworkProtocolVersion(uint32 GameNetworkProtocolVersion);

	/**
	* Sets the game compatible network protocol version used for networking and invalidate cached values
	*/
	static void SetGameCompatibleNetworkProtocolVersion(uint32 GameCompatibleNetworkProtocolVersion);

	/**
	* Returns the project version used by networking
	* 
	* @return FString
	*/
	static const FString& GetProjectVersion() { return GetProjectVersion_Internal(); }

	/**
	* Invalidates any cached network checksum and forces it to be recalculated on next request
	*/
	static void InvalidateNetworkChecksum() { bHasCachedNetworkChecksum = false; }

protected:

	/**
	* Used to allow BP only projects to override network versions
	*/
	static FString& GetProjectVersion_Internal();

	static bool		bHasCachedNetworkChecksum;
	static uint32	CachedNetworkChecksum;

	static uint32	EngineNetworkProtocolVersion;
	static uint32	GameNetworkProtocolVersion;

	static uint32	EngineCompatibleNetworkProtocolVersion;
	static uint32	GameCompatibleNetworkProtocolVersion;
};
