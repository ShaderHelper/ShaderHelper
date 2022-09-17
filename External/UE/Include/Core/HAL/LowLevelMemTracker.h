// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "ProfilingDebugging/TagTrace.h"

#ifndef ALLOW_LOW_LEVEL_MEM_TRACKER_IN_TEST
	#define ALLOW_LOW_LEVEL_MEM_TRACKER_IN_TEST 0
#endif

// LLM is currently incompatible with PLATFORM_USES_FIXED_GMalloc_CLASS, because LLM is activated way too early
// Inability to use LLM with PLATFORM_USES_FIXED_GMalloc_CLASS is not a problem, because fixed GMalloc is only used in Test/Shipping builds
#define LLM_ENABLED_ON_PLATFORM (PLATFORM_SUPPORTS_LLM && !PLATFORM_USES_FIXED_GMalloc_CLASS)

// *** enable/disable LLM here ***
#if !defined(ENABLE_LOW_LEVEL_MEM_TRACKER) || !LLM_ENABLED_ON_PLATFORM 
	#define ENABLE_LOW_LEVEL_MEM_TRACKER (LLM_ENABLED_ON_PLATFORM && !UE_BUILD_SHIPPING && (!UE_BUILD_TEST || ALLOW_LOW_LEVEL_MEM_TRACKER_IN_TEST) && WITH_ENGINE && 1)
#endif

#if ENABLE_LOW_LEVEL_MEM_TRACKER

// Public defines configuring LLM; see also private defines in LowLevelMemTracker.cpp

// LLM_ALLOW_ASSETS_TAGS: Set to 1 to report the asset in scope for each allocation in addition to the callstack, at the cost of more LLM memory usage per allocation. Set to 0 to omit this information
// using asset tagging requires a significantly higher number of per-thread tags, so make it optional
// even if this is on, we still need to run with -llmtagsets=assets because of the sheer number of stat ids it makes
// LLM Assets can be viewed in game using 'Stat LLMAssets'
#ifndef LLM_ALLOW_ASSETS_TAGS
	#define LLM_ALLOW_ASSETS_TAGS 0
#endif

// LLM_ALLOW_STATS: Set to 1 to allow stats to be used as tags (LLM_SCOPED_TAG_WITH_STAT et al), at the cost of more LLM memory usage per allocation. Set to 0 to disable these stat-specific scope macros.
// Turning this on uses the same amount of memory per allocation as LLM_ALLOW_NAMES_TAGS. Turning both of them on has no extra cost.
#ifndef LLM_ALLOW_STATS
	#define LLM_ALLOW_STATS 0
#endif

// Enable stat tags if: (1) Stats are allowed or (2) Asset tags are allowed (asset tags use the stat macros to record asset scopes)
#define LLM_ENABLED_STAT_TAGS LLM_ALLOW_STATS || LLM_ALLOW_ASSETS_TAGS

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "HAL/CriticalSection.h"
#include "Templates/AlignmentTemplates.h"
#include "Templates/UnrealTemplate.h"
#include "UObject/NameTypes.h"

#if DO_CHECK

	namespace UE
	{
	namespace LLMPrivate
	{
		bool HandleAssert(bool bLog, const TCHAR* Format, ...);

		// This is used by ensure to generate a bool per instance
		// by passing a lambda which will uniquely instantiate the template.
		template <typename Type>
		bool TrueOnFirstCallOnly(const Type&)
		{
			static bool bValue = true;
			bool Result = bValue;
			bValue = false;
			return Result;
		}
	}
	}

#if !USING_CODE_ANALYSIS
	#define LLMTrueOnFirstCallOnly			UE::LLMPrivate::TrueOnFirstCallOnly([]{})
#else
	#define LLMTrueOnFirstCallOnly			false
#endif

#define LLMCheckMessage(expr)          TEXT("LLM check failed: %s [File:%s] [Line: %d]\r\n"),             TEXT(#expr), TEXT(__FILE__), __LINE__
#define LLMCheckfMessage(expr, format) TEXT("LLM check failed: %s [File:%s] [Line: %d]\r\n") format TEXT("\r\n"),      TEXT(#expr), TEXT(__FILE__), __LINE__
#define LLMEnsureMessage(expr)         TEXT("LLM ensure failed: %s [File:%s] [Line: %d]\r\n"),            TEXT(#expr), TEXT(__FILE__), __LINE__

#define LLMCheck(expr)					do { if (UNLIKELY(!(expr))) { UE::LLMPrivate::HandleAssert(true, LLMCheckMessage(expr));                         FPlatformMisc::RaiseException(1); } } while(false)
#define LLMCheckf(expr,format,...)		do { if (UNLIKELY(!(expr))) { UE::LLMPrivate::HandleAssert(true, LLMCheckfMessage(expr, format), ##__VA_ARGS__); FPlatformMisc::RaiseException(1); } } while(false)
#define LLMEnsure(expr)					(LIKELY(!!(expr)) || UE::LLMPrivate::HandleAssert(LLMTrueOnFirstCallOnly, LLMEnsureMessage(expr)))

#else

#define LLMCheck(expr)
#define LLMCheckf(expr,...)
#define LLMEnsure(expr)			(!!(expr))

#endif

#define LLM_TAG_TYPE uint8

// estimate the maximum amount of memory LLM will need to run on a game with around 4 million allocations.
// Make sure that you have debug memory enabled on consoles (on screen warning will show if you don't)
// (currently only used on PS4 to stop it reserving a large chunk up front. This will go away with the new memory system)
#define LLM_MEMORY_OVERHEAD (600LL*1024*1024)

/*
 * LLM Trackers
 */
enum class ELLMTracker : uint8
{
	Platform,
	Default,

	Max,
};

/*
 * optional tags that need to be enabled with -llmtagsets=x,y,z on the commandline
 */
enum class ELLMTagSet : uint8
{
	None,
	Assets,
	AssetClasses,
	
	Max,	// note: check out FLowLevelMemTracker::ShouldReduceThreads and IsAssetTagForAssets if you add any asset-style tagsets
};

#define LLM_ENUM_GENERIC_TAGS(macro) \
	macro(Untagged,								"Untagged",						NAME_None,													NAME_None,										-1)\
	macro(Paused,								"Paused",						NAME_None,													NAME_None,										-1)\
	macro(Total,								"Total",						GET_STATFNAME(STAT_TotalLLM),								GET_STATFNAME(STAT_TrackedTotalSummaryLLM),		-1)\
	macro(Untracked,							"Untracked",					GET_STATFNAME(STAT_UntrackedLLM),							GET_STATFNAME(STAT_TrackedTotalSummaryLLM),		-1)\
	macro(PlatformTotal,						"Total",						GET_STATFNAME(STAT_PlatformTotalLLM),						NAME_None,										-1)\
	macro(TrackedTotal,							"TrackedTotal",					GET_STATFNAME(STAT_TrackedTotalLLM),						GET_STATFNAME(STAT_TrackedTotalSummaryLLM),		-1)\
	macro(UntaggedTotal,						"Untagged",						GET_STATFNAME(STAT_UntaggedTotalLLM),						NAME_None,										-1)\
	macro(WorkingSetSize,						"WorkingSetSize",				GET_STATFNAME(STAT_WorkingSetSizeLLM),						GET_STATFNAME(STAT_TrackedTotalSummaryLLM),		-1)\
	macro(PagefileUsed,							"PagefileUsed",					GET_STATFNAME(STAT_PagefileUsedLLM),						GET_STATFNAME(STAT_TrackedTotalSummaryLLM),		-1)\
	macro(PlatformTrackedTotal,					"TrackedTotal",					GET_STATFNAME(STAT_PlatformTrackedTotalLLM),				NAME_None,										-1)\
	macro(PlatformUntaggedTotal,				"Untagged",						GET_STATFNAME(STAT_PlatformUntaggedTotalLLM),				NAME_None,										-1)\
	macro(PlatformUntracked,					"Untracked",					GET_STATFNAME(STAT_PlatformUntrackedLLM),					NAME_None,										-1)\
	macro(PlatformOverhead,						"LLMOverhead",					GET_STATFNAME(STAT_PlatformOverheadLLM),					NAME_None,										-1)\
	macro(PlatformOSAvailable,					"OSAvailable",					GET_STATFNAME(STAT_PlatformOSAvailableLLM),					NAME_None,										-1)\
	macro(FMalloc,								"FMalloc",						GET_STATFNAME(STAT_FMallocLLM),								NAME_None,										-1)\
	macro(FMallocUnused,						"FMallocUnused",				GET_STATFNAME(STAT_FMallocUnusedLLM),						GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(ThreadStack,							"ThreadStack",					GET_STATFNAME(STAT_ThreadStackLLM),							GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(ThreadStackPlatform,					"ThreadStack",					GET_STATFNAME(STAT_ThreadStackPlatformLLM),					NAME_None,										-1)\
	macro(ProgramSizePlatform,					"ProgramSize",					GET_STATFNAME(STAT_ProgramSizePlatformLLM),					NAME_None,										-1)\
	macro(ProgramSize,							"ProgramSize",					GET_STATFNAME(STAT_ProgramSizeLLM),							GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(BackupOOMMemoryPoolPlatform,			"OOMBackupPool",				GET_STATFNAME(STAT_OOMBackupPoolPlatformLLM),				NAME_None,										-1)\
	macro(BackupOOMMemoryPool,					"OOMBackupPool",				GET_STATFNAME(STAT_OOMBackupPoolLLM),						GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(GenericPlatformMallocCrash,			"GenericPlatformMallocCrash",	GET_STATFNAME(STAT_GenericPlatformMallocCrashLLM),			GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(GenericPlatformMallocCrashPlatform,	"GenericPlatformMallocCrash",	GET_STATFNAME(STAT_GenericPlatformMallocCrashPlatformLLM),	GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	/* Any low-level memory that is not tracked in any other category. */ \
	macro(EngineMisc,							"EngineMisc",					GET_STATFNAME(STAT_EngineMiscLLM),							GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	/* Any task kicked off from the task graph that doesn't have its own category. Should be fairly low. */ \
	macro(TaskGraphTasksMisc,					"TaskGraphMiscTasks",			GET_STATFNAME(STAT_TaskGraphTasksMiscLLM),					GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(LinearAllocator,						"LinearAllocator",				GET_STATFNAME(STAT_LinearAllocatorLLM),						NAME_None,										-1)\
	macro(Audio,								"Audio",						GET_STATFNAME(STAT_AudioLLM),								GET_STATFNAME(STAT_AudioSummaryLLM),			-1)\
	macro(AudioMisc,							"AudioMisc",					GET_STATFNAME(STAT_AudioMiscLLM),							GET_STATFNAME(STAT_AudioSummaryLLM),			ELLMTag::Audio)\
	macro(AudioSoundWaves,						"AudioSoundWaves",				GET_STATFNAME(STAT_AudioSoundWavesLLM),						GET_STATFNAME(STAT_AudioSummaryLLM),			ELLMTag::Audio)\
	macro(AudioSoundWaveProxies,				"AudioSoundWaveProxies",		GET_STATFNAME(STAT_AudioSoundWaveProxiesLLM),				GET_STATFNAME(STAT_AudioSummaryLLM),			ELLMTag::Audio)\
	macro(AudioMixer,							"AudioMixer",					GET_STATFNAME(STAT_AudioMixerLLM),							GET_STATFNAME(STAT_AudioSummaryLLM),			ELLMTag::Audio)\
	macro(AudioMixerPlugins,					"AudioMixerPlugins",			GET_STATFNAME(STAT_AudioMixerPluginsLLM),					GET_STATFNAME(STAT_AudioSummaryLLM),			ELLMTag::Audio)\
	macro(AudioPrecache,						"AudioPrecache",				GET_STATFNAME(STAT_AudioPrecacheLLM),						GET_STATFNAME(STAT_AudioSummaryLLM),			ELLMTag::Audio)\
	macro(AudioDecompress,						"AudioDecompress",				GET_STATFNAME(STAT_AudioDecompressLLM),						GET_STATFNAME(STAT_AudioSummaryLLM),			ELLMTag::Audio)\
	macro(AudioRealtimePrecache,				"AudioRealtimePrecache",		GET_STATFNAME(STAT_AudioRealtimePrecacheLLM),				GET_STATFNAME(STAT_AudioSummaryLLM),			ELLMTag::Audio)\
	macro(AudioFullDecompress,					"AudioFullDecompress",			GET_STATFNAME(STAT_AudioFullDecompressLLM),					GET_STATFNAME(STAT_AudioSummaryLLM),			ELLMTag::Audio)\
	macro(AudioStreamCache,						"AudioStreamCache",				GET_STATFNAME(STAT_AudioStreamCacheLLM),					GET_STATFNAME(STAT_AudioSummaryLLM),			ELLMTag::Audio)\
	macro(AudioStreamCacheCompressedData,		"AudioStreamCacheCompressedData",GET_STATFNAME(STAT_AudioStreamCacheCompressedDataLLM),		GET_STATFNAME(STAT_AudioSummaryLLM),			ELLMTag::Audio)\
	macro(AudioSynthesis,						"AudioSynthesis",				GET_STATFNAME(STAT_AudioSynthesisLLM),						GET_STATFNAME(STAT_AudioSummaryLLM),			ELLMTag::Audio)\
	macro(RealTimeCommunications,				"RealTimeCommunications",		GET_STATFNAME(STAT_RealTimeCommunicationsLLM),				NAME_None,										-1)\
	macro(FName,								"FName",						GET_STATFNAME(STAT_FNameLLM),								GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(Networking,							"Networking",					GET_STATFNAME(STAT_NetworkingLLM),							GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(Meshes,								"Meshes",						GET_STATFNAME(STAT_MeshesLLM),								GET_STATFNAME(STAT_MeshesSummaryLLM),			-1)\
	macro(Stats,								"Stats",						GET_STATFNAME(STAT_StatsLLM),								GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(Shaders,								"Shaders",						GET_STATFNAME(STAT_ShadersLLM),								GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(PSO,									"PSO",							GET_STATFNAME(STAT_PSOLLM),									GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(Textures,								"Textures",						GET_STATFNAME(STAT_TexturesLLM),							GET_STATFNAME(STAT_TexturesSummaryLLM),			-1)\
	macro(TextureMetaData,						"TextureMetaData",				GET_STATFNAME(STAT_TextureMetaDataLLM),						GET_STATFNAME(STAT_TexturesSummaryLLM),			-1)\
	macro(VirtualTextureSystem,					"VirtualTextureSystem",			GET_STATFNAME(STAT_VirtualTextureSystemLLM),				GET_STATFNAME(STAT_TexturesSummaryLLM),			-1)\
	macro(RenderTargets,						"RenderTargets",				GET_STATFNAME(STAT_RenderTargetsLLM),						GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(SceneRender,							"SceneRender",					GET_STATFNAME(STAT_SceneRenderLLM),							GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(RHIMisc,								"RHIMisc",						GET_STATFNAME(STAT_RHIMiscLLM),								GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(AsyncLoading,							"AsyncLoading",					GET_STATFNAME(STAT_AsyncLoadingLLM),						GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
    /* UObject is a catch-all for all Engine and game memory that is not tracked in any other category. */ \
	/* it includes any class inherited from UObject and anything that is serialized by that class including properties. */ \
	/* Note that this stat doesn't include Mesh or Animation data which are tracked separately. */ \
	/* It is correlated to the number of Objects placed in the Level. */ \
	macro(UObject,								"UObject",						GET_STATFNAME(STAT_UObjectLLM),								GET_STATFNAME(STAT_UObjectSummaryLLM),			-1)\
	macro(Animation,							"Animation",					GET_STATFNAME(STAT_AnimationLLM),							GET_STATFNAME(STAT_AnimationSummaryLLM),		-1)\
	/* This is the UStaticMesh class and related properties, and does not include the actual mesh data. */ \
	macro(StaticMesh,							"StaticMesh",					GET_STATFNAME(STAT_StaticMeshLLM),							GET_STATFNAME(STAT_StaticMeshSummaryLLM),		ELLMTag::Meshes)\
	macro(Materials,							"Materials",					GET_STATFNAME(STAT_MaterialsLLM),							GET_STATFNAME(STAT_MaterialsSummaryLLM),		-1)\
	macro(Particles,							"Particles",					GET_STATFNAME(STAT_ParticlesLLM),							GET_STATFNAME(STAT_ParticlesSummaryLLM),		-1)\
	macro(Niagara,								"Niagara",						GET_STATFNAME(STAT_NiagaraLLM),								GET_STATFNAME(STAT_NiagaraSummaryLLM),			-1)\
	macro(GPUSort,								"GPUSort",						GET_STATFNAME(STAT_GPUSortLLM),								GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(GC,									"GC",							GET_STATFNAME(STAT_GCLLM),									GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(UI,									"UI",							GET_STATFNAME(STAT_UILLM),									GET_STATFNAME(STAT_UISummaryLLM),				-1)\
	macro(NavigationRecast,						"NavigationRecast",				GET_STATFNAME(STAT_NavigationRecastLLM),					GET_STATFNAME(STAT_NavigationSummaryLLM),		-1)\
	macro(Physics,								"Physics",						GET_STATFNAME(STAT_PhysicsLLM),								GET_STATFNAME(STAT_PhysicsSummaryLLM),			-1)\
	macro(PhysX,								"PhysX",						GET_STATFNAME(STAT_PhysXLLM),								GET_STATFNAME(STAT_PhysXSummaryLLM),			ELLMTag::Physics)\
	macro(PhysXGeometry,						"PhysXGeometry",				GET_STATFNAME(STAT_PhysXGeometryLLM),						GET_STATFNAME(STAT_PhysXSummaryLLM),			ELLMTag::Physics)\
	macro(PhysXTrimesh,							"PhysXTrimesh",					GET_STATFNAME(STAT_PhysXTrimeshLLM),						GET_STATFNAME(STAT_PhysXSummaryLLM),			ELLMTag::Physics)\
	macro(PhysXConvex,							"PhysXConvex",					GET_STATFNAME(STAT_PhysXConvexLLM),							GET_STATFNAME(STAT_PhysXSummaryLLM),			ELLMTag::Physics)\
	macro(PhysXAllocator,						"PhysXAllocator",				GET_STATFNAME(STAT_PhysXAllocatorLLM),						GET_STATFNAME(STAT_PhysXSummaryLLM),			ELLMTag::Physics)\
	macro(PhysXLandscape,						"PhysXLandscape",				GET_STATFNAME(STAT_PhysXLandscapeLLM),						GET_STATFNAME(STAT_PhysXSummaryLLM),			ELLMTag::Physics)\
	macro(Chaos,								"Chaos",						GET_STATFNAME(STAT_ChaosLLM),								GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(ChaosGeometry,						"ChaosGeometry",				GET_STATFNAME(STAT_ChaosGeometryLLM),						GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(ChaosAcceleration,					"ChaosAcceleration",			GET_STATFNAME(STAT_ChaosAccelerationLLM),					GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(ChaosParticles,						"ChaosParticles",				GET_STATFNAME(STAT_ChaosParticlesLLM),						GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(ChaosLandscape,						"ChaosLandscape",				GET_STATFNAME(STAT_ChaosLandscapeLLM),						GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(ChaosTrimesh,							"ChaosTrimesh",					GET_STATFNAME(STAT_ChaosTrimeshLLM),						GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(ChaosConvex,							"ChaosConvex",					GET_STATFNAME(STAT_ChaosConvexLLM),							GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(ChaosScene,							"ChaosScene",					GET_STATFNAME(STAT_ChaosSceneLLM),							GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(ChaosUpdate,							"ChaosUpdate",					GET_STATFNAME(STAT_ChaosUpdateLLM),							GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(ChaosActor,							"ChaosActor",					GET_STATFNAME(STAT_ChaosActorLLM),							GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(ChaosBody,							"ChaosBody",					GET_STATFNAME(STAT_ChaosBodyLLM),							GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(ChaosConstraint,						"ChaosConstraint",				GET_STATFNAME(STAT_ChaosConstraintLLM),						GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(ChaosMaterial,						"ChaosMaterial",				GET_STATFNAME(STAT_ChaosMaterialLLM),						GET_STATFNAME(STAT_ChaosSummaryLLM),			ELLMTag::Physics)\
	macro(EnginePreInitMemory,					"EnginePreInit",				GET_STATFNAME(STAT_EnginePreInitLLM),						GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(EngineInitMemory,						"EngineInit",					GET_STATFNAME(STAT_EngineInitLLM),							GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(RenderingThreadMemory,				"RenderingThread",				GET_STATFNAME(STAT_RenderingThreadLLM),						GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(LoadMapMisc,							"LoadMapMisc",					GET_STATFNAME(STAT_LoadMapMiscLLM),							GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(StreamingManager,						"StreamingManager",				GET_STATFNAME(STAT_StreamingManagerLLM),					GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(GraphicsPlatform,						"Graphics",						GET_STATFNAME(STAT_GraphicsPlatformLLM),					NAME_None,										-1)\
	macro(FileSystem,							"FileSystem",					GET_STATFNAME(STAT_FileSystemLLM),							GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(Localization,							"Localization",					GET_STATFNAME(STAT_LocalizationLLM),						GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(AssetRegistry,						"AssetRegistry",				GET_STATFNAME(STAT_AssetRegistryLLM),						GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(ConfigSystem,							"ConfigSystem",					GET_STATFNAME(STAT_ConfigSystemLLM),						GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(InitUObject,							"InitUObject",					GET_STATFNAME(STAT_InitUObjectLLM),							GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(VideoRecording,						"VideoRecording",				GET_STATFNAME(STAT_VideoRecordingLLM),						GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(Replays,								"Replays",						GET_STATFNAME(STAT_ReplaysLLM),								GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(MaterialInstance,						"MaterialInstance",				GET_STATFNAME(STAT_MaterialInstanceLLM),					GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(SkeletalMesh,							"SkeletalMesh",					GET_STATFNAME(STAT_SkeletalMeshLLM),						GET_STATFNAME(STAT_EngineSummaryLLM),			ELLMTag::Meshes)\
	macro(InstancedMesh,						"InstancedMesh",				GET_STATFNAME(STAT_InstancedMeshLLM),						GET_STATFNAME(STAT_EngineSummaryLLM),			ELLMTag::Meshes)\
	macro(Landscape,							"Landscape",					GET_STATFNAME(STAT_LandscapeLLM),							GET_STATFNAME(STAT_EngineSummaryLLM),			ELLMTag::Meshes)\
	macro(CsvProfiler,							"CsvProfiler",					GET_STATFNAME(STAT_CsvProfilerLLM),							GET_STATFNAME(STAT_EngineSummaryLLM),			-1)\
	macro(MediaStreaming,						"MediaStreaming",				GET_STATFNAME(STAT_MediaStreamingLLM),						GET_STATFNAME(STAT_MediaStreamingSummaryLLM),	-1)\
	macro(ElectraPlayer,						"ElectraPlayer",				GET_STATFNAME(STAT_ElectraPlayerLLM),						GET_STATFNAME(STAT_MediaStreamingSummaryLLM),	ELLMTag::MediaStreaming)\
	macro(WMFPlayer,							"WMFPlayer",					GET_STATFNAME(STAT_WMFPlayerLLM),							GET_STATFNAME(STAT_MediaStreamingSummaryLLM),	ELLMTag::MediaStreaming)\
	macro(PlatformMMIO,							"MMIO",							GET_STATFNAME(STAT_PlatformMMIOLLM),						NAME_None,										-1)\
	macro(PlatformVM,							"Virtual Memory",				GET_STATFNAME(STAT_PlatformVMLLM),							NAME_None,										-1)\
	macro(CustomName,							"CustomName",					GET_STATFNAME(STAT_CustomName),								NAME_None,										-1)\

/*
 * Enum values to be passed in to LLM_SCOPE() macro
 */
enum class ELLMTag : LLM_TAG_TYPE
{
#define LLM_ENUM(Enum,Str,Stat,Group,Parent) Enum,
	LLM_ENUM_GENERIC_TAGS(LLM_ENUM)
#undef LLM_ENUM

	GenericTagCount,

	//------------------------------
	// Platform tags
	PlatformTagStart = 108,
	PlatformTagEnd = 149,

	//------------------------------
	// Project tags
	ProjectTagStart = 150,
	ProjectTagEnd = 255,

	// anything above this value is treated as an FName for a stat section
};
static_assert( ELLMTag::GenericTagCount <= ELLMTag::PlatformTagStart, "too many LLM tags defined"); 

static constexpr uint32 LLM_TAG_COUNT = 256;
static constexpr uint32 LLM_CUSTOM_TAG_START = (int32)ELLMTag::PlatformTagStart;
static constexpr uint32 LLM_CUSTOM_TAG_END = (int32)ELLMTag::ProjectTagEnd;
static constexpr uint32 LLM_CUSTOM_TAG_COUNT = LLM_CUSTOM_TAG_END + 1 - LLM_CUSTOM_TAG_START;


/**
 * Passed in to OnLowLevelAlloc to specify the type of allocation. Used to track FMalloc total
 * and pausing for a specific allocation type.
 */
enum class ELLMAllocType
{
	None = 0,
	FMalloc,
	System,

	Count
};

extern const ANSICHAR* LLMGetTagNameANSI(ELLMTag Tag);
extern const TCHAR* LLMGetTagName(ELLMTag Tag);
UE_DEPRECATED(4.27, "This function was an unused implementation detail; contact Epic if you need to keep its functionality.")
extern FName LLMGetTagStatGroup(ELLMTag Tag);
UE_DEPRECATED(4.27, "This function was an unused implementation detail; contact Epic if you need to keep its functionality.")
extern FName LLMGetTagStat(ELLMTag Tag);

/*
 * LLM utility macros
 */
#define LLM(x) x
#define LLM_IF_ENABLED(x) if (!FLowLevelMemTracker::bIsDisabled) { x; }
#define SCOPE_NAME PREPROCESSOR_JOIN(LLMScope,__LINE__)

///////////////////////////////////////////////////////////////////////////////////////
// These are the main macros to use externally when tracking memory
///////////////////////////////////////////////////////////////////////////////////////

/**
 * LLM scope macros
 */
#define LLM_SCOPE(Tag)												FLLMScope SCOPE_NAME(Tag, false /* bIsStatTag */, ELLMTagSet::None, ELLMTracker::Default);\
																	UE_MEMSCOPE(Tag) 
#define LLM_SCOPE_BYNAME(Tag) 										static FName PREPROCESSOR_JOIN(LLMScope_Name,__LINE__)(Tag);\
																	FLLMScope SCOPE_NAME(PREPROCESSOR_JOIN(LLMScope_Name,__LINE__), false /* bIsStatTag */, ELLMTagSet::None, ELLMTracker::Default);\
																	UE_MEMSCOPE(PREPROCESSOR_JOIN(LLMScope_Name,__LINE__));
#define LLM_SCOPE_BYTAG(TagDeclName)								FLLMScope SCOPE_NAME(PREPROCESSOR_JOIN(LLMTagDeclaration_, TagDeclName).GetUniqueName(), false /* bIsStatTag */, ELLMTagSet::None, ELLMTracker::Default);\
																	UE_MEMSCOPE(PREPROCESSOR_JOIN(LLMTagDeclaration_,TagDeclName).GetUniqueName());
#define LLM_PLATFORM_SCOPE(Tag)										FLLMScope SCOPE_NAME(Tag, false /* bIsStatTag */, ELLMTagSet::None, ELLMTracker::Platform);
#define LLM_PLATFORM_SCOPE_BYNAME(Tag) 								static FName PREPROCESSOR_JOIN(LLMScope_Name,__LINE__)(Tag);\
																	FLLMScope SCOPE_NAME(PREPROCESSOR_JOIN(LLMLLMScope_NameScope,__LINE__), false /* bIsStatTag */, ELLMTagSet::None, ELLMTracker::Platform);
#define LLM_PLATFORM_SCOPE_BYTAG(TagDeclName)						FLLMScope SCOPE_NAME(PREPROCESSOR_JOIN(LLMTagDeclaration_, TagDeclName).GetUniqueName(), false /* bIsStatTag */, ELLMTagSet::None, ELLMTracker::Platform);

 /**
 * LLM Pause scope macros
 */
#define LLM_SCOPED_PAUSE_TRACKING(AllocType) 						FLLMPauseScope SCOPE_NAME(ELLMTag::Untagged, false /* bIsStatTag */, 0, ELLMTracker::Max, AllocType);
#define LLM_SCOPED_PAUSE_TRACKING_FOR_TRACKER(Tracker, AllocType) 	FLLMPauseScope SCOPE_NAME(ELLMTag::Untagged, false /* bIsStatTag */, 0, Tracker, AllocType);
#define LLM_SCOPED_PAUSE_TRACKING_WITH_ENUM_AND_AMOUNT(Tag, Amount, Tracker, AllocType) FLLMPauseScope SCOPE_NAME(Tag, false /* bIsStatTag */, Amount, Tracker, AllocType);

/**
 * LLM realloc scope macros. Used when reallocating a pointer and you wish to retain the tagging from the source pointer
 */
#define LLM_REALLOC_SCOPE(Ptr)										FLLMScopeFromPtr SCOPE_NAME(Ptr, ELLMTracker::Default);
#define LLM_REALLOC_PLATFORM_SCOPE(Ptr)								FLLMScopeFromPtr SCOPE_NAME(Ptr, ELLMTracker::Platform);

/**
 * LLM tag dumping, to help with identifying mis-tagged items. Probably don't want to check in with these in use!
 */
#define LLM_DUMP_TAG()  FLowLevelMemTracker::Get().DumpTag(ELLMTracker::Default,__FILE__,__LINE__)
#define LLM_DUMP_PLATFORM_TAG()  FLowLevelMemTracker::Get().DumpTag(ELLMTracker::Platform,__FILE__,__LINE__)

/**
 * Define a tag which can be used in LLM_SCOPE_BYTAG or referenced by name in other LLM_SCOPEs.
 * @param UniqueNameWithUnderscores - Modified version of the name of the tag. Used for looking up by name, must be unique across all tags passed to LLM_DEFINE_TAG, LLM_SCOPE, or ELLMTag.
 *                                    The modification: the usual separator / for parents must be replaced with _ in LLM_DEFINE_TAGs.
 * @param DisplayName - (Optional) - The name to display when tracing the tag; joined with "/" to the name of its parent if it has a parent, or NAME_None to use the UniqueName.
 * @param ParentTagName - (Optional) - The unique name of the parent tag, or NAME_None if it has no parent.
 * @param StatName - (Optional) - The name of the stat to populate with this tag's amount when publishing LLM data each frame, or NAME_None if no stat should be populated.
 * @param SummaryStatName - (Optional) - The name of the stat group to add on this tag's amount when publishing LLM data each frame, or NAME_None if no stat group should be added to.
 */
#define LLM_DEFINE_TAG(UniqueNameWithUnderscores, ...) FLLMTagDeclaration PREPROCESSOR_JOIN(LLMTagDeclaration_, UniqueNameWithUnderscores)(TEXT(#UniqueNameWithUnderscores), ##__VA_ARGS__)

 /**
  * Declare a tag which is defined elsewhere which can used in LLM_SCOPE_BYTAG or referenced by name in other LLM_SCOPEs.
  * @param UniqueName - The name of the tag for looking up by name, must be unique across all tags passed to LLM_DEFINE_TAG, LLM_SCOPE, or ELLMTag.
  * @param ModuleName - (Optional, only in LLM_DECLARE_TAG_API) - The MODULENAME_API symbol to use to declare module linkage, aka ENGINE_API. If omitted, no module linkage will be used and the tag will create link errors if used from another module.
  */
#define LLM_DECLARE_TAG(UniqueNameWithUnderscores) extern FLLMTagDeclaration PREPROCESSOR_JOIN(LLMTagDeclaration_, UniqueNameWithUnderscores)
#define LLM_DECLARE_TAG_API(UniqueNameWithUnderscores, ModuleAPI) extern ModuleAPI FLLMTagDeclaration PREPROCESSOR_JOIN(LLMTagDeclaration_, UniqueNameWithUnderscores)

/**
 * The BootStrap versions of LLM_DEFINE_TAG, LLM_SCOPE_BYTAG, and LLM_DECLARE_TAG support use in scopes during global
 * c++ constructors, before Main. These tags are slightly more expensive (even when LLM is disabled) in all
 * configurations other than shipping, because they use a function static rather than a global variable.
 */
#define LLM_DEFINE_BOOTSTRAP_TAG(UniqueNameWithUnderscores, ...) \
	FLLMTagDeclaration& PREPROCESSOR_JOIN(GetLLMTagDeclaration_, UniqueNameWithUnderscores)() \
	{ \
		static FLLMTagDeclaration PREPROCESSOR_JOIN(LLMTagDeclaration_, UniqueNameWithUnderscores)(TEXT(#UniqueNameWithUnderscores), ##__VA_ARGS__); \
		return PREPROCESSOR_JOIN(LLMTagDeclaration_, UniqueNameWithUnderscores); \
	}
#define LLM_SCOPE_BY_BOOTSTRAP_TAG(TagDeclName) FLLMScope SCOPE_NAME(PREPROCESSOR_JOIN(GetLLMTagDeclaration_, TagDeclName)().GetUniqueName(), false /* bIsStatTag */, ELLMTagSet::None, ELLMTracker::Default);\
												UE_MEMSCOPE(PREPROCESSOR_JOIN(GetLLMTagDeclaration_,TagDeclName)().GetUniqueName());
#define LLM_DECLARE_BOOTSTRAP_TAG(UniqueNameWithUnderscores) extern FLLMTagDeclaration& PREPROCESSOR_JOIN(GetLLMTagDeclaration_, UniqueNameWithUnderscores)();
#define LLM_DECLARE_BOOTSTRAP_TAG_API(UniqueNameWithUnderscores, ModuleAPI) extern ModuleAPI FLLMTagDeclaration& PREPROCESSOR_JOIN(GetLLMTagDeclaration_, UniqueNameWithUnderscores)();

typedef void*(*LLMAllocFunction)(size_t);
typedef void(*LLMFreeFunction)(void*, size_t);

class FLLMTagDeclaration;

namespace UE
{
namespace LLMPrivate
{

	class FTagData;
	class FTagDataArray;
	class FTagDataNameMap;
	class FLLMCsvWriter;
	class FLLMThreadState;
	class FLLMTraceWriter;
	class FLLMCsvProfilerWriter;
	class FLLMTracker;

	namespace AllocatorPrivate
	{
		struct FPage;
		struct FBin;
	}
	/**
	 * The allocator LLM uses to allocate internal memory. Uses platform defined
	 * allocation functions to grab memory directly from the OS.
	 */
	class FLLMAllocator
	{
	public:
		FLLMAllocator();
		~FLLMAllocator();

		static FLLMAllocator*& Get();

		void Initialise(LLMAllocFunction InAlloc, LLMFreeFunction InFree, int32 InPageSize);
		void Clear();
		void* Alloc(size_t Size);
		void* Malloc(size_t Size);
		void Free(void* Ptr, size_t Size);
		void* Realloc(void* Ptr, size_t OldSize, size_t NewSize);
		int64 GetTotal() const;

		template <typename T, typename... ArgsType>
		T* New(ArgsType&&... Args)
		{
			T* Ptr = reinterpret_cast<T*>(Alloc(sizeof(T)));
			new (Ptr) T(Forward<ArgsType>(Args)...);
			return Ptr;
		}

		template <typename T>
		void Delete(T* Ptr)
		{
			if (Ptr)
			{
				Ptr->~T();
				Free(Ptr, sizeof(T));
			}
		}

		LLMAllocFunction GetPlatformAlloc()
		{
			return PlatformAlloc;
		}
		LLMFreeFunction GetPlatformFree()
		{
			return PlatformFree;
		}

	private:
		void* AllocPages(size_t Size);
		void FreePages(void* Ptr, size_t Size);
		int32 GetBinIndex(size_t Size) const;

		FCriticalSection CriticalSection;
		LLMAllocFunction PlatformAlloc;
		LLMFreeFunction PlatformFree;
		AllocatorPrivate::FBin* Bins;
		int64 Total;
		int32 PageSize;
		int32 NumBins;

		friend struct UE::LLMPrivate::AllocatorPrivate::FPage;
		friend struct UE::LLMPrivate::AllocatorPrivate::FBin;
	};

	enum class ETagReferenceSource
	{
		Scope,
		Declare,
		EnumTag,
		CustomEnumTag,
		FunctionAPI
	};
}
}

struct UE_DEPRECATED(4.27, "FLLMCustomTag was an implementation detail that has been modified, switch to FLLMTagInfo or to your own local struct") FLLMCustomTag
{
	int32 Tag;
	const TCHAR* Name;
	FName StatName;
	FName SummaryStatName;
};

/* A convenient struct for gathering the fields needed to report in RegisterProjectTag */
struct FLLMTagInfo
{
	const TCHAR* Name;
	FName StatName;				// shows in the LLMFULL stat group
	FName SummaryStatName;		// shows in the LLM summary stat group
	int32 ParentTag = -1;
};

/*
 * The main LLM tracker class
 */
class CORE_API FLowLevelMemTracker
{
public:

	// get the singleton, which makes sure that we always have a valid object
	inline static FLowLevelMemTracker& Get()
	{
		if (TrackerInstance)
			return *TrackerInstance;
		else
			return Construct();
	}

	static FLowLevelMemTracker& Construct();

	static bool IsEnabled();

	// we always start up running, but if the commandline disables us, we will do it later after main
	// (can't get the commandline early enough in a cross-platform way)
	void ProcessCommandLine(const TCHAR* CmdLine);

	// Return the total amount of memory being tracked
	uint64 GetTotalTrackedMemory(ELLMTracker Tracker);

	// this is the main entry point for the class - used to track any pointer that was allocated or freed 
	void OnLowLevelAlloc(ELLMTracker Tracker, const void* Ptr, uint64 Size, ELLMTag DefaultTag = ELLMTag::Untagged, ELLMAllocType AllocType = ELLMAllocType::None, bool bTrackInMemPro = true);		// DefaultTag is used if no other tag is set
	void OnLowLevelAlloc(ELLMTracker Tracker, const void* Ptr, uint64 Size, FName DefaultTag, ELLMAllocType AllocType = ELLMAllocType::None, bool bTrackInMemPro = true);		// DefaultTag is used if no other tag is set
	void OnLowLevelFree(ELLMTracker Tracker, const void* Ptr, ELLMAllocType AllocType = ELLMAllocType::None, bool bTrackInMemPro = true);

	// call if an allocation is moved in memory, such as in a defragger
	void OnLowLevelAllocMoved(ELLMTracker Tracker, const void* Dest, const void* Source, ELLMAllocType AllocType = ELLMAllocType::None);

	// expected to be called once a frame, from game thread or similar - updates memory stats 
	void UpdateStatsPerFrame(const TCHAR* LogName=nullptr);
	/** A tick function that can be called as frequently as necessary rather than once per frame; this is sometimes necessary when tracking large amounts of tags that have a superlinear update cost */
	void Tick();

	// Optionally set the amount of memory taken up before the game starts for executable and data segments
	void SetProgramSize(uint64 InProgramSize);

	// console command handler
	bool Exec(const TCHAR* Cmd, FOutputDevice& Ar);

	// are we in the more intensive asset tracking mode, and is it active
	bool IsTagSetActive(ELLMTagSet Set);

	// for some tag sets, it's really useful to reduce threads, to attribute allocations to assets, for instance
	bool ShouldReduceThreads();

	// get the top active tag for the given tracker
	UE_DEPRECATED(4.27, "Tags have been changed to FNames and the old ELLMTag is now only the top-level coarse tag. Use GetActivateTagData instead to get the current Tag instead of its toplevel parent.")
	int64 GetActiveTag(ELLMTracker Tracker);

	/** Get an opaque identifier for the top active tag for the given tracker */
	const UE::LLMPrivate::FTagData* GetActiveTagData(ELLMTracker Tracker);

	// register custom ELLMTags
	void RegisterPlatformTag(int32 Tag, const TCHAR* Name, FName StatName, FName SummaryStatName, int32 ParentTag = -1);
	void RegisterProjectTag(int32 Tag, const TCHAR* Name, FName StatName, FName SummaryStatName, int32 ParentTag = -1);
    
	// look up the ELLMTag associated with the given display name
	bool FindTagByName( const TCHAR* Name, uint64& OutTag ) const;

	UE_DEPRECATED(4.27, "Use FindTagDisplayName instead")
	const TCHAR* FindTagName(uint64 Tag) const;
	// get the display name for the given ELLMTag
	FName FindTagDisplayName(uint64 Tag) const;

	// Get the amount of memory for an ELLMTag from the given tracker
	int64 GetTagAmountForTracker(ELLMTracker Tracker, ELLMTag Tag);

	// Set the amount of memory for an ELLMTag for a given tracker, optionally updating the total tracked memory too
	void SetTagAmountForTracker(ELLMTracker Tracker, ELLMTag Tag, int64 Amount, bool bAddToTotal );

	// Dump the display name of the current TagData for the given tracker to the output
	uint64 DumpTag( ELLMTracker Tracker, const char* FileName, int LineNumber );

	// Publishes the active LLM stats in the active frame, useful for single targeted LLM snapshots
	void PublishDataSingleFrame();

private:
	FLowLevelMemTracker();

	~FLowLevelMemTracker();

	/** Allocation and Setup of the data required for tracking allocations is done as late as possible to prevent exercising allocation code too early
	 * Note that as late as possible for tracking allocations is still earlier than ParseCommandLine, so we do not complete all initialisation in this function
	 * (e.g. features required only for Update are omitted), and whatever we do initialise here will be torn down later in ParseCommandLine if LLM is disabled
	 */
	void BootstrapInitialise();

	/** Free all memory. This will put the tracker into a permanently disabled state */
	void Clear();
	void InitialiseProgramSize();

	class UE::LLMPrivate::FLLMTracker* GetTracker(ELLMTracker Tracker);

	void TickInternal();
	void UpdateTags();
	void SortTags(UE::LLMPrivate::FTagDataArray*& OutOldTagDatas);
	void PublishDataPerFrame(const TCHAR* LogName);

	void RegisterCustomTagInternal(int32 Tag, const TCHAR* Name, FName StatName, FName SummaryStatName, int32 ParentTag = -1);
	/**
	* Called during C++ global static initialization when GMalloc is available (and hence FNames are unavailable)
	* Creates the subset of tags necessary to record allocations during GMalloc and FName construction
	*/
	void BootstrapTagDatas();
	/** Called when we have detected enabled in processcommandline, or as late as possible if callers access features that require full initialisation before then */
	void FinishInitialise();
	void InitialiseTagDatas_SetLLMTagNames();
	void InitialiseTagDatas_FinishRegister();
	void InitialiseTagDatas();
	void ClearTagDatas();
	void RegisterTagDeclaration(FLLMTagDeclaration& TagDeclaration);
	UE::LLMPrivate::FTagData& RegisterTagData(FName Name, FName DisplayName, FName ParentName, FName StatName, FName SummaryStatName, bool bHasEnumTag, ELLMTag EnumTag, bool bIsStatTag, UE::LLMPrivate::ETagReferenceSource ReferenceSource);
	/** Construct if not yet done the data on the given FTagData that relies on the presence of other TagDatas */
	void FinishConstruct(UE::LLMPrivate::FTagData* TagData, UE::LLMPrivate::ETagReferenceSource ReferenceSource);
	void ReportDuplicateTagName(UE::LLMPrivate::FTagData* TagData, UE::LLMPrivate::ETagReferenceSource ReferenceSource);

	const UE::LLMPrivate::FTagData* FindOrAddTagData(ELLMTag EnumTag, UE::LLMPrivate::ETagReferenceSource ReferenceSource = UE::LLMPrivate::ETagReferenceSource::FunctionAPI);
	const UE::LLMPrivate::FTagData* FindOrAddTagData(FName Name, bool bIsStatData=false, UE::LLMPrivate::ETagReferenceSource ReferenceSource = UE::LLMPrivate::ETagReferenceSource::FunctionAPI);
	const UE::LLMPrivate::FTagData* FindTagData(ELLMTag EnumTag, UE::LLMPrivate::ETagReferenceSource ReferenceSource = UE::LLMPrivate::ETagReferenceSource::FunctionAPI);
	const UE::LLMPrivate::FTagData* FindTagData(FName Name, UE::LLMPrivate::ETagReferenceSource ReferenceSource = UE::LLMPrivate::ETagReferenceSource::FunctionAPI);

	friend class FLLMPauseScope;
	friend class FLLMScope;
	friend class FLLMScopeFromPtr;
	friend class UE::LLMPrivate::FLLMCsvWriter;
	friend class UE::LLMPrivate::FLLMTracker;
	friend class UE::LLMPrivate::FLLMThreadState;
	friend class UE::LLMPrivate::FLLMTraceWriter;
	friend class UE::LLMPrivate::FLLMCsvProfilerWriter;
	friend void GlobalRegisterTagDeclaration(FLLMTagDeclaration& TagDeclaration);

	UE::LLMPrivate::FLLMAllocator Allocator;
	/** All TagDatas that have been constructed, in an array sorted by TagData->GetIndex() */
	UE::LLMPrivate::FTagDataArray* TagDatas;
	/** Map from TagData->GetName() to TagData for all names, used to handle LLM_SCOPE with FName */
	UE::LLMPrivate::FTagDataNameMap* TagDataNameMap;
	/** Array to Map from ELLMTag to the TagData for that tag, used to handle LLM_SCOPE with ELLMTag */
	UE::LLMPrivate::FTagData** TagDataEnumMap;

	UE::LLMPrivate::FLLMTracker* Trackers[static_cast<int32>(ELLMTracker::Max)];

	mutable FRWLock TagDataLock;
	FCriticalSection UpdateLock;

	uint64 ProgramSize;
	int64 MemoryUsageCurrentOverhead;
	int64 MemoryUsagePlatformTotalUntracked;

	bool ActiveSets[(int32)ELLMTagSet::Max];

	bool bFirstTimeUpdating;
	bool bCanEnable;
	bool bCsvWriterEnabled;
	bool bTraceWriterEnabled;
	bool bInitialisedTracking;
	bool bIsBootstrapping;
	bool bFullyInitialised;
	bool bConfigurationComplete;
	bool bTagAdded;
	bool bAutoPublish;
	bool bPublishSingleFrame;

	static FLowLevelMemTracker* TrackerInstance;
public: // really internal but needs to be visible for LLM_IF_ENABLED macro
	static bool bIsDisabled;
};

/*
 * LLM scope for tracking memory
 */
class CORE_API FLLMScope
{
public:
	FLLMScope(FName TagName, bool bIsStatTag, ELLMTagSet InTagSet, ELLMTracker InTracker)
	{
		if (!FLowLevelMemTracker::bIsDisabled)
		{
			Init(TagName, bIsStatTag, InTagSet, InTracker);
		}
	}
	FLLMScope(ELLMTag TagEnum, bool bIsStatTag, ELLMTagSet InTagSet, ELLMTracker InTracker)
	{
		if (!FLowLevelMemTracker::bIsDisabled)
		{
			Init(TagEnum, bIsStatTag, InTagSet, InTracker);
		}

	}

	FLLMScope(const UE::LLMPrivate::FTagData* TagData, bool bIsStatTag, ELLMTagSet Set, ELLMTracker Tracker)
	{
		if (!FLowLevelMemTracker::bIsDisabled)
		{
			Init(TagData, bIsStatTag, Set, Tracker);
		}
	}

	~FLLMScope()
	{
		if (bEnabled)
		{
			Destruct();
		}
	}

protected:
	void Init(FName TagName, bool bIsStatTag, ELLMTagSet InTagSet, ELLMTracker InTracker);
	void Init(ELLMTag TagEnum, bool bIsStatTag, ELLMTagSet InTagSet, ELLMTracker InTracker);
	void Init(const UE::LLMPrivate::FTagData* TagData, bool bIsStatTag, ELLMTagSet InTagSet, ELLMTracker InTracker);
	void Destruct();

	ELLMTracker Tracker{};
	bool bEnabled = false;
#if LLM_ALLOW_ASSETS_TAGS
	bool bIsAssetTag = false;
#endif
};

/*
* LLM scope for pausing LLM (disables the allocation hooks)
*/
class CORE_API FLLMPauseScope
{
public:
	FLLMPauseScope(FName TagName, bool bIsStatTag, uint64 Amount, ELLMTracker TrackerToPause, ELLMAllocType InAllocType);
	FLLMPauseScope(ELLMTag TagEnum, bool bIsStatTag, uint64 Amount, ELLMTracker TrackerToPause, ELLMAllocType InAllocType);
	~FLLMPauseScope();
protected:
	void Init(FName TagName, ELLMTag EnumTag, bool bIsEnumTag, bool bIsStatTag, uint64 Amount, ELLMTracker TrackerToPause, ELLMAllocType InAllocType);
	ELLMTracker PausedTracker;
	ELLMAllocType AllocType;
	bool bEnabled;
};

/*
* LLM scope for inheriting tag from the given address
*/
class CORE_API FLLMScopeFromPtr
{
public:
	FLLMScopeFromPtr(void* Ptr, ELLMTracker Tracker);
	~FLLMScopeFromPtr();
protected:
	ELLMTracker Tracker;
	bool bEnabled;
};

/**
 * Global instances to provide information about a tag to LLM
 */
class CORE_API FLLMTagDeclaration
{
public:
	FLLMTagDeclaration(const TCHAR* InCPPName, const FName InDisplayName=NAME_None, FName InParentTagName = NAME_None, FName InStatName = NAME_None, FName InSummaryStatName = NAME_None);
	FName GetUniqueName() const { return UniqueName; }

	typedef void (*FCreationCallback)(FLLMTagDeclaration&);
protected:

	static void AddCreationCallback(FCreationCallback InCallback);
	static void ClearCreationCallbacks();
	static TArrayView<FCreationCallback> GetCreationCallbacks();
	static FLLMTagDeclaration* GetList();

	void Register();

protected:
	void ConstructUniqueName();

	const TCHAR* CPPName;
	FName UniqueName;
	FName DisplayName;
	FName ParentTagName;
	FName StatName;
	FName SummaryStatName;
	FLLMTagDeclaration* Next = nullptr;

	friend class FLowLevelMemTracker;
	friend class FTagTrace;
};


#else

	#define LLM(...)
	#define LLM_IF_ENABLED(...)
	#define LLM_SCOPE(...)
	#define LLM_SCOPE_BYNAME(...)
	#define LLM_SCOPE_BYTAG(...)
	#define LLM_PLATFORM_SCOPE(...)
	#define LLM_PLATFORM_SCOPE_BYNAME(...)
	#define LLM_PLATFORM_SCOPE_BYTAG(...)
	#define LLM_REALLOC_SCOPE(...)
	#define LLM_REALLOC_PLATFORM_SCOPE(...)
	#define LLM_SCOPED_PAUSE_TRACKING(...)
	#define LLM_SCOPED_PAUSE_TRACKING_FOR_TRACKER(...)
	#define LLM_SCOPED_PAUSE_TRACKING_WITH_ENUM_AND_AMOUNT(...)
	#define LLM_DUMP_TAG()
	#define LLM_DUMP_PLATFORM_TAG()
	#define LLM_DEFINE_TAG(...)
	#define LLM_DECLARE_TAG(...)
	#define LLM_DECLARE_TAG_API(...)
	#define LLM_DEFINE_BOOTSTRAP_TAG(...)
	#define LLM_SCOPE_BY_BOOTSTRAP_TAG(...)
	#define LLM_DECLARE_BOOTSTRAP_TAG(...) 
	#define LLM_DECLARE_BOOTSTRAP_TAG_API(...)

#endif		// #if ENABLE_LOW_LEVEL_MEM_TRACKER
