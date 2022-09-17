// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/*--------------------------------------------------------------------------------
	Build configuration coming from UBT, do not modify
--------------------------------------------------------------------------------*/

// Set any configuration not defined by UBT to zero
#ifndef UE_BUILD_DEBUG
	#define UE_BUILD_DEBUG				0
#endif
#ifndef UE_BUILD_DEVELOPMENT
	#define UE_BUILD_DEVELOPMENT		0
#endif
#ifndef UE_BUILD_TEST
	#define UE_BUILD_TEST				0
#endif
#ifndef UE_BUILD_SHIPPING
	#define UE_BUILD_SHIPPING			0
#endif
#ifndef UE_GAME
	#define UE_GAME						0
#endif
#ifndef UE_EDITOR
	#define UE_EDITOR					0
#endif
#ifndef UE_BUILD_SHIPPING_WITH_EDITOR
	#define UE_BUILD_SHIPPING_WITH_EDITOR 0
#endif
#ifndef UE_BUILD_DOCS
	#define UE_BUILD_DOCS				0
#endif

/** 
 *   Whether compiling for dedicated server or not.
 */
#ifndef UE_SERVER
	#define UE_SERVER					0
#endif

// Ensure that we have one, and only one build config coming from UBT
#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING != 1
	#error Exactly one of [UE_BUILD_DEBUG UE_BUILD_DEVELOPMENT UE_BUILD_TEST UE_BUILD_SHIPPING] should be defined to be 1
#endif

/*--------------------------------------------------------------------------------
	Legacy defined we want to make sure don't compile if they came in a merge.
--------------------------------------------------------------------------------*/

#define FINAL_RELEASE_DEBUGCONSOLE	(#)
#define FINAL_RELEASE				(#)
#define SHIPPING_PC_GAME			(#)
#define UE_BUILD_FINAL_RELEASE (#)

/*----------------------------------------------------------------------------
	Mandatory bridge options coming from UBT, do not modify directly!
----------------------------------------------------------------------------*/

/**
 * Whether we are compiling with the editor; must be defined by UBT
 */
#ifndef WITH_EDITOR
	#define WITH_EDITOR	0 // for auto-complete
	#error UBT should always define WITH_EDITOR to be 0 or 1
#endif

/**
 * Whether we are compiling with the engine; must be defined by UBT
 */
#ifndef WITH_ENGINE
	#define WITH_ENGINE	0 // for auto-complete
	#error UBT should always define WITH_ENGINE to be 0 or 1
#endif

/**
 *	Whether we are compiling with developer tools; must be defined by UBT
 */
#ifndef WITH_UNREAL_DEVELOPER_TOOLS
	#define WITH_UNREAL_DEVELOPER_TOOLS		0	// for auto-complete
	#error UBT should always define WITH_UNREAL_DEVELOPER_TOOLS to be 0 or 1
#endif

 /**
  *	Whether we are compiling with developer tools that may use other platforms or external connected devices, etc
  */
#ifndef WITH_UNREAL_TARGET_DEVELOPER_TOOLS
	#define WITH_UNREAL_TARGET_DEVELOPER_TOOLS		WITH_UNREAL_DEVELOPER_TOOLS // a subset of WITH_UNREAL_DEVELOPER_TOOLS, but can be disabled separately
#endif

/**
 *	Whether we are compiling with plugin support; must be defined by UBT
 */
#ifndef WITH_PLUGIN_SUPPORT
	#define WITH_PLUGIN_SUPPORT		0	// for auto-complete
	#error UBT should always define WITH_PLUGIN_SUPPORT to be 0 or 1
#endif

/**
 * Whether we are compiling with Slate accessibility and automation support
 */
#ifndef WITH_ACCESSIBILITY
	#define WITH_ACCESSIBILITY		1
#endif

 /** Enable perf counters */
#ifndef WITH_PERFCOUNTERS
	#define WITH_PERFCOUNTERS		0
#endif

/** 
 * Whether we are compiling a PGO instrumented build.
 */
#ifndef ENABLE_PGO_PROFILE
	#define ENABLE_PGO_PROFILE 0
#endif

/**
 * Unreal Header Tool requires extra data stored in the structure of a few core files. This enables some ifdef hacks to make this work. 
 * Set via UBT, do not modify directly
 */
#ifndef HACK_HEADER_GENERATOR
	#define HACK_HEADER_GENERATOR 0
#endif

/** Whether we are compiling with automation worker functionality.  Note that automation worker defaults to enabled in
    UE_BUILD_TEST configuration, so that it can be used for performance testing on devices */
#ifndef WITH_AUTOMATION_WORKER
	#define WITH_AUTOMATION_WORKER !(UE_BUILD_SHIPPING || HACK_HEADER_GENERATOR)
#endif

/**
* Whether we want a monolithic build (no DLLs); must be defined by UBT
*/
#ifndef IS_MONOLITHIC
	#define IS_MONOLITHIC 0 // for auto-complete
	#error UBT should always define IS_MONOLITHIC to be 0 or 1
#endif

/**
* Whether we want a program (shadercompilerworker, fileserver) or a game; must be defined by UBT
*/
#ifndef IS_PROGRAM
	#define IS_PROGRAM 0 // for autocomplete
	#error UBT should always define IS_PROGRAM to be 0 or 1
#endif

/**
* Whether we support hot-reload. Currently requires a non-monolithic build and non-shipping configuration.
*/
#ifndef WITH_HOT_RELOAD
	#define WITH_HOT_RELOAD (!IS_MONOLITHIC && !UE_BUILD_SHIPPING && !UE_BUILD_TEST && !UE_GAME && !UE_SERVER)
#endif

/**
* Make sure that live coding define is available.  Normally this is supplied by UBT
*/
#ifndef WITH_LIVE_CODING
	#define WITH_LIVE_CODING 0
#endif

/**
* Whether we support any type of live reloading
*/
#define WITH_RELOAD (WITH_HOT_RELOAD || WITH_LIVE_CODING)

/**
* Whether we include support for text archive formats. Disabling support allows de-virtualizing archive calls
* and eliminating string constants for field names.
*/
#ifndef WITH_TEXT_ARCHIVE_SUPPORT
	#define WITH_TEXT_ARCHIVE_SUPPORT WITH_EDITORONLY_DATA
#endif

/*----------------------------------------------------------------------------
	Optional bridge options coming from UBT, do not modify directly!
	If UBT doesn't set the value, it is assumed to be 0, and we set that here.
----------------------------------------------------------------------------*/

/**
 * Checks to see if pure virtual has actually been implemented, this is normally run as a CIS process and is set (indirectly) by UBT
 *
 * @see Core.h
 * @see ObjectMacros.h
 **/
#ifndef CHECK_PUREVIRTUALS
	#define CHECK_PUREVIRTUALS 0
#endif

/** Whether to use the null RHI. */
#ifndef USE_NULL_RHI
	#define USE_NULL_RHI 0
#endif

/** If not specified, disable logging in shipping */
#ifndef USE_LOGGING_IN_SHIPPING
	#define USE_LOGGING_IN_SHIPPING 0
#endif

#ifndef USE_CHECKS_IN_SHIPPING
	#define USE_CHECKS_IN_SHIPPING 0
#endif

/** If not defined follow the CHECK behavior since previously ensures were compiled in with checks */
#ifndef USE_ENSURES_IN_SHIPPING
	#define USE_ENSURES_IN_SHIPPING USE_CHECKS_IN_SHIPPING
#endif

#ifndef ALLOW_CONSOLE_IN_SHIPPING
	#define ALLOW_CONSOLE_IN_SHIPPING 0
#endif

/** Compile flag to force stats to be compiled */
#ifndef FORCE_USE_STATS
	#define FORCE_USE_STATS 0
#endif

/**
 *	Optionally enable support for named events from the stat macros without the stat system overhead
 *	This will attempt to disable regular stats system and use named events instead
 */
#ifndef ENABLE_STATNAMEDEVENTS
	#define ENABLE_STATNAMEDEVENTS	0
#endif

#ifndef ENABLE_STATNAMEDEVENTS_UOBJECT
	#define ENABLE_STATNAMEDEVENTS_UOBJECT 0
#endif

/*--------------------------------------------------------------------------------
	Basic options that by default depend on the build configuration and platform

	DO_GUARD_SLOW									If true, then checkSlow, checkfSlow and verifySlow are compiled into the executable.
	DO_CHECK										If true, then checkCode, checkf, verify, check, checkNoEntry, checkNoReentry, checkNoRecursion, verifyf, checkf are compiled into the executables
	DO_ENSURE										If true, then ensure, ensureAlways, ensureMsgf and ensureAlwaysMsgf are compiled into the executables
	STATS											If true, then the stats system is compiled into the executable.
	ALLOW_DEBUG_FILES								If true, then debug files like screen shots and profiles can be saved from the executable.
	NO_LOGGING										If true, then no logs or text output will be produced

--------------------------------------------------------------------------------*/

#if UE_BUILD_DEBUG
	#ifndef DO_GUARD_SLOW
		#define DO_GUARD_SLOW									1
	#endif
	#ifndef DO_CHECK
		#define DO_CHECK										1
	#endif
	#ifndef DO_ENSURE
		#define DO_ENSURE										1
	#endif
	#ifndef STATS
		#define STATS											((WITH_UNREAL_DEVELOPER_TOOLS || !WITH_EDITORONLY_DATA || USE_STATS_WITHOUT_ENGINE || USE_MALLOC_PROFILER || FORCE_USE_STATS) && !ENABLE_STATNAMEDEVENTS)
	#endif
	#ifndef ALLOW_DEBUG_FILES
		#define ALLOW_DEBUG_FILES								1
	#endif
	#ifndef ALLOW_CONSOLE
		#define ALLOW_CONSOLE									1
	#endif
	#ifndef NO_LOGGING
		#define NO_LOGGING										0
	#endif
#elif UE_BUILD_DEVELOPMENT
	#ifndef DO_GUARD_SLOW
		#define DO_GUARD_SLOW									0
	#endif
	#ifndef DO_CHECK
		#define DO_CHECK										1
	#endif
	#ifndef DO_ENSURE
		#define DO_ENSURE										1
	#endif
	#ifndef STATS
		#define STATS											((WITH_UNREAL_DEVELOPER_TOOLS || !WITH_EDITORONLY_DATA || USE_STATS_WITHOUT_ENGINE || USE_MALLOC_PROFILER || FORCE_USE_STATS) && !ENABLE_STATNAMEDEVENTS)
	#endif
	#ifndef ALLOW_DEBUG_FILES
		#define ALLOW_DEBUG_FILES								1
	#endif
	#ifndef ALLOW_CONSOLE
		#define ALLOW_CONSOLE									1
	#endif
	#ifndef NO_LOGGING
		#define NO_LOGGING										0
	#endif
#elif UE_BUILD_TEST
	#ifndef DO_GUARD_SLOW
		#define DO_GUARD_SLOW									0
	#endif
	#ifndef DO_CHECK
		#define DO_CHECK										USE_CHECKS_IN_SHIPPING
	#endif
	#ifndef DO_ENSURE
		#define DO_ENSURE										USE_ENSURES_IN_SHIPPING
	#endif
	#ifndef STATS
		#define STATS											((USE_MALLOC_PROFILER || FORCE_USE_STATS) && !ENABLE_STATNAMEDEVENTS)
	#endif
	#ifndef ALLOW_DEBUG_FILES
		#define ALLOW_DEBUG_FILES								1
	#endif
	#ifndef ALLOW_CONSOLE
		#define ALLOW_CONSOLE									1
	#endif
	#ifndef NO_LOGGING
		#define NO_LOGGING										!USE_LOGGING_IN_SHIPPING
	#endif
#elif UE_BUILD_SHIPPING
	#ifndef DO_GUARD_SLOW
		#define DO_GUARD_SLOW								0
	#endif
	#ifndef DO_CHECK
		#define DO_CHECK									USE_CHECKS_IN_SHIPPING
	#endif
	#ifndef DO_ENSURE
		#define DO_ENSURE									USE_ENSURES_IN_SHIPPING
	#endif
	#ifndef STATS
		#define STATS										(FORCE_USE_STATS && !ENABLE_STATNAMEDEVENTS)
	#endif
	#ifndef ALLOW_DEBUG_FILES
		#define ALLOW_DEBUG_FILES							WITH_EDITOR
	#endif
	#ifndef ALLOW_CONSOLE
		#define ALLOW_CONSOLE								ALLOW_CONSOLE_IN_SHIPPING
	#endif
	#ifndef NO_LOGGING
		#define NO_LOGGING									!USE_LOGGING_IN_SHIPPING
	#endif
#else
	#error Exactly one of [UE_BUILD_DEBUG UE_BUILD_DEVELOPMENT UE_BUILD_TEST UE_BUILD_SHIPPING] should be defined to be 1
#endif


/**
 * This is a global setting which will turn on logging / checks for things which are
 * considered especially bad for consoles.  Some of the checks are probably useful for PCs also.
 *
 * Throughout the code base there are specific things which dramatically affect performance and/or
 * are good indicators that something is wrong with the content.  These have PERF_ISSUE_FINDER in the
 * comment near the define to turn the individual checks on. 
 *
 * e.g. #if defined(PERF_LOG_DYNAMIC_LOAD_OBJECT) || LOOKING_FOR_PERF_ISSUES
 *
 * If one only cares about DLO, then one can enable the PERF_LOG_DYNAMIC_LOAD_OBJECT define.  Or one can
 * globally turn on all PERF_ISSUE_FINDERS :-)
 *
 **/
#ifndef LOOKING_FOR_PERF_ISSUES
	#define LOOKING_FOR_PERF_ISSUES (0 && !(UE_BUILD_SHIPPING))
#endif

/** Enable the use of the network profiler as long as we are not a Shipping or Test build */
#ifndef USE_NETWORK_PROFILER
#define USE_NETWORK_PROFILER !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#endif

/** Enable UberGraphPersistentFrame feature. It can speed up BP compilation (re-instancing) in editor, but introduce an unnecessary overhead in runtime. */
#define USE_UBER_GRAPH_PERSISTENT_FRAME 1

/** Enable validation of the Uber Graph's persistent frame's layout, this is useful to detect uber graph frame related corruption */
#define VALIDATE_UBER_GRAPH_PERSISTENT_FRAME (!(UE_BUILD_SHIPPING || UE_BUILD_TEST)) && USE_UBER_GRAPH_PERSISTENT_FRAME

/** Enable fast calls for event thunks into an event graph that have no parameters  */
#define UE_BLUEPRINT_EVENTGRAPH_FASTCALLS 1

/** Enable perf counters on dedicated servers */
#define USE_SERVER_PERF_COUNTERS ((UE_SERVER || UE_EDITOR) && WITH_PERFCOUNTERS)

#define USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING 1
#define USE_DEFERRED_DEPENDENCY_CHECK_VERIFICATION_TESTS (USE_CIRCULAR_DEPENDENCY_LOAD_DEFERRING && 0)

// 0 (default), set this to 1 to get draw events with "TOGGLEDRAWEVENTS" "r.ShowMaterialDrawEvents" and the "ProfileGPU" command working in test
#ifndef ALLOW_PROFILEGPU_IN_TEST
	#define ALLOW_PROFILEGPU_IN_TEST 0
#endif

#ifndef ALLOW_PROFILEGPU_IN_SHIPPING
	#define ALLOW_PROFILEGPU_IN_SHIPPING 0
#endif

// draw events with "TOGGLEDRAWEVENTS" "r.ShowMaterialDrawEvents" (for ProfileGPU, Pix, Razor, RenderDoc, ...) and the "ProfileGPU" command are normally compiled out for TEST and SHIPPING
#define WITH_PROFILEGPU (!(UE_BUILD_SHIPPING || UE_BUILD_TEST) || (UE_BUILD_TEST && ALLOW_PROFILEGPU_IN_TEST) || (UE_BUILD_SHIPPING && ALLOW_PROFILEGPU_IN_SHIPPING))

#ifndef ALLOW_DUMPGPU_IN_TEST
	#define ALLOW_DUMPGPU_IN_TEST 1
#endif

#ifndef ALLOW_DUMPGPU_IN_SHIPPING
	#define ALLOW_DUMPGPU_IN_SHIPPING 0
#endif

// DumpGPU command
#define WITH_DUMPGPU (!(UE_BUILD_SHIPPING || UE_BUILD_TEST) || (UE_BUILD_TEST && ALLOW_DUMPGPU_IN_TEST) || (UE_BUILD_SHIPPING && ALLOW_DUMPGPU_IN_SHIPPING))

#ifndef ALLOW_CHEAT_CVARS_IN_TEST
	#define ALLOW_CHEAT_CVARS_IN_TEST 1
#endif

#define DISABLE_CHEAT_CVARS (UE_BUILD_SHIPPING || (UE_BUILD_TEST && !ALLOW_CHEAT_CVARS_IN_TEST))

// Controls the creation of a thread for detecting hangs (FThreadHeartBeat). This is subject to other criteria, USE_HANG_DETECTION
#ifndef ALLOW_HANG_DETECTION
	#define ALLOW_HANG_DETECTION 1
#endif
#define USE_HANG_DETECTION (ALLOW_HANG_DETECTION && !WITH_EDITORONLY_DATA && !IS_PROGRAM && !UE_BUILD_DEBUG && !ENABLE_PGO_PROFILE)

// Controls the creation of a thread for detecting hitches (FGameThreadHitchHeartBeat). This is subject to other criteria, USE_HITCH_DETECTION
#ifndef ALLOW_HITCH_DETECTION
	#define ALLOW_HITCH_DETECTION 0
#endif

// Adjust a few things with the slack policy and MallocBinned2 to minimize memory usage (at some performance cost)
#ifndef AGGRESSIVE_MEMORY_SAVING
	#define AGGRESSIVE_MEMORY_SAVING 0
#endif

// Controls if UObjects are initialized as soon as they are available or only after the module is "loaded". This only applies to monolithic builds; if there are DLLs, this is how it works anyway and this should not be turned on
#ifndef USE_PER_MODULE_UOBJECT_BOOTSTRAP
	#define USE_PER_MODULE_UOBJECT_BOOTSTRAP 0
#endif

#define USE_HITCH_DETECTION (ALLOW_HITCH_DETECTION && !WITH_EDITORONLY_DATA && !IS_PROGRAM && !UE_BUILD_DEBUG)

// Controls whether shipping builds create backups of the most recent log file.
// All other configurations always create backups.
#ifndef PRESERVE_LOG_BACKUPS_IN_SHIPPING
	#define PRESERVE_LOG_BACKUPS_IN_SHIPPING 1
#endif

#ifndef ENABLE_RHI_VALIDATION
	#define ENABLE_RHI_VALIDATION (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)
#endif

// Controls whether FPlatformMisc::GetDeviceId() is available to be called.
// When set to 1, calls to this API will be hardcoded to return an empty string
// to avoid running afoul of calling device APIs that platform owners may restrict
// access to without waivers or special steps. Code that uses GetDeviceId() should
// expect to receive empty strings in these cases and response appropriately with
// fallback logic.
#ifndef GET_DEVICE_ID_UNAVAILABLE
	#define GET_DEVICE_ID_UNAVAILABLE 0
#endif

// Controls whether the executable is compiled with cooked editor functionality
#ifndef UE_IS_COOKED_EDITOR
#define UE_IS_COOKED_EDITOR 0
#endif
// Controls whether to enable loading cooked packages from I/O store in editor builds. Defaults to UE_IS_COOKED_EDITOR
// but can be defined separately if needed
#ifndef WITH_IOSTORE_IN_EDITOR
#define WITH_IOSTORE_IN_EDITOR UE_IS_COOKED_EDITOR
#endif

// Controls whether or not to make a global object to load COnfig.bin as soon as possible
#ifndef PRELOAD_BINARY_CONFIG
	#define PRELOAD_BINARY_CONFIG 1
#endif

#ifndef WITH_COTF
	#define WITH_COTF ((WITH_ENGINE) && !(IS_PROGRAM || UE_BUILD_SHIPPING))
#endif

// Controls if the config system can stores configs for other platforms than the running one
#define ALLOW_OTHER_PLATFORM_CONFIG		WITH_UNREAL_DEVELOPER_TOOLS
