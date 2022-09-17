// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../HAL/PreprocessorHelpers.h"


// This file is included in some resource files, which issue a warning:
//
// warning RC4011: identifier truncated to 'PLATFORM_CAN_SUPPORT_EDITORONLY'
//
// due to limitations of resource compiler. The only thing needed from this file
// for resource compilation is PREPROCESSOR_TO_STRING macro at the end, so we take
// rest of code out for resource compilation.
#ifndef RC_INVOKED

#define PLAYWORLD_PACKAGE_PREFIX TEXT("UEDPIE")

#ifndef WITH_EDITORONLY_DATA
	#if !PLATFORM_CAN_SUPPORT_EDITORONLY_DATA || UE_SERVER || PLATFORM_IOS
		#define WITH_EDITORONLY_DATA	0
	#else
		#define WITH_EDITORONLY_DATA	1
	#endif
#endif

/** This controls if metadata for compiled in classes is unpacked and setup at boot time. Meta data is not normally used except by the editor. **/
#define WITH_METADATA (WITH_EDITORONLY_DATA && WITH_EDITOR)

// Set up optimization control macros, now that we have both the build settings and the platform macros
#define PRAGMA_DISABLE_OPTIMIZATION		PRAGMA_DISABLE_OPTIMIZATION_ACTUAL
#if UE_BUILD_DEBUG
	#define PRAGMA_ENABLE_OPTIMIZATION  PRAGMA_DISABLE_OPTIMIZATION_ACTUAL
#else
	#define PRAGMA_ENABLE_OPTIMIZATION  PRAGMA_ENABLE_OPTIMIZATION_ACTUAL
#endif

#if UE_BUILD_DEBUG
	#define FORCEINLINE_DEBUGGABLE FORCEINLINE_DEBUGGABLE_ACTUAL
#else
	#define FORCEINLINE_DEBUGGABLE FORCEINLINE
#endif


#if STATS
	#define CLOCK_CYCLES(Timer)   {Timer -= FPlatformTime::Cycles();}
	#define UNCLOCK_CYCLES(Timer) {Timer += FPlatformTime::Cycles();}
#else
	#define CLOCK_CYCLES(Timer)
	#define UNCLOCK_CYCLES(Timer)
#endif

#define SHUTDOWN_IF_EXIT_REQUESTED
#define RETURN_IF_EXIT_REQUESTED
#define RETURN_VAL_IF_EXIT_REQUESTED(x)

#if CHECK_PUREVIRTUALS
#define PURE_VIRTUAL(func,...) =0;
#else
#define PURE_VIRTUAL(func,...) { LowLevelFatalError(TEXT("Pure virtual not implemented (%s)"), TEXT(#func)); __VA_ARGS__ }
#endif


// Code analysis features
#ifndef USING_CODE_ANALYSIS
	#define USING_CODE_ANALYSIS 0
#endif

#if USING_CODE_ANALYSIS
	#if !defined( CA_IN ) || !defined( CA_OUT ) || !defined( CA_READ_ONLY ) || !defined( CA_WRITE_ONLY ) || !defined( CA_VALID_POINTER ) || !defined( CA_CHECK_RETVAL ) || !defined( CA_NO_RETURN ) || !defined( CA_SUPPRESS ) || !defined( CA_ASSUME )
		#error Code analysis macros are not configured correctly for this platform
	#endif
#else
	// Just to be safe, define all of the code analysis macros to empty macros
	#define CA_IN 
	#define CA_OUT
	#define CA_READ_ONLY
	#define CA_WRITE_ONLY
	#define CA_VALID_POINTER
	#define CA_CHECK_RETVAL
	#define CA_NO_RETURN
	#define CA_SUPPRESS( WarningNumber )
	#define CA_ASSUME( Expr )
	#define CA_CONSTANT_IF(Condition) if (Condition)
#endif

#ifndef USING_THREAD_SANITISER
	#define USING_THREAD_SANITISER 0
#endif

#if USING_THREAD_SANITISER
	#if !defined( TSAN_SAFE ) || !defined( TSAN_BEFORE ) || !defined( TSAN_AFTER ) || !defined( TSAN_ATOMIC )
		#error Thread Sanitiser macros are not configured correctly for this platform
	#endif
#else
	// Define TSAN macros to empty when not enabled
	#define TSAN_SAFE
	#define TSAN_BEFORE(Addr)
	#define TSAN_AFTER(Addr)
	#define TSAN_ATOMIC(Type) Type
#endif

enum {INDEX_NONE	= -1				};
enum {UNICODE_BOM   = 0xfeff			};

enum EForceInit 
{
	ForceInit,
	ForceInitToZero
};
enum ENoInit {NoInit};
enum EInPlace {InPlace};

#endif // RC_INVOKED

// When passed to pragma message will result in clickable warning in VS
#define WARNING_LOCATION(Line) __FILE__ "(" PREPROCESSOR_TO_STRING(Line) ")"

// Push and pop macro definitions
#ifdef __clang__
	#define UE_PUSH_MACRO(name) _Pragma(PREPROCESSOR_TO_STRING(push_macro(name)))
	#define UE_POP_MACRO(name) _Pragma(PREPROCESSOR_TO_STRING(pop_macro(name)))
#else
	#define UE_PUSH_MACRO(name) __pragma(push_macro(name))
	#define UE_POP_MACRO(name) __pragma(pop_macro(name))
#endif
#define PUSH_MACRO(name) DEPRECATED_MACRO(5.0, "PUSH_MACRO is deprecated. Use UE_PUSH_MACRO and pass the macro name as a string.") UE_PUSH_MACRO(PREPROCESSOR_TO_STRING(name))
#define POP_MACRO(name) DEPRECATED_MACRO(5.0, "POP_MACRO is deprecated. Use UE_POP_MACRO and pass the macro name as a string.") UE_POP_MACRO(PREPROCESSOR_TO_STRING(name))

#ifdef __COUNTER__
	// Created a variable with a unique name
	#define ANONYMOUS_VARIABLE( Name ) PREPROCESSOR_JOIN(Name, __COUNTER__)
#else
	// Created a variable with a unique name.
	// Less reliable than the __COUNTER__ version.
	#define ANONYMOUS_VARIABLE( Name ) PREPROCESSOR_JOIN(Name, __LINE__)
#endif

/** Thread-safe call once helper, similar to std::call_once without the std::once_flag */
#define UE_CALL_ONCE(Func) static int32 ANONYMOUS_VARIABLE(ThreadSafeOnce) = ((Func)(), 1)

/**
 * Macro for marking up deprecated code, functions and types.
 *
 * This should be used as syntactic replacement for the [[deprecated]] attribute
 * which provides a UE version number like the old DEPRECATED macro.
 *
 * Features that are marked as deprecated are scheduled to be removed from the code base
 * in a future release. If you are using a deprecated feature in your code, you should
 * replace it before upgrading to the next release. See the Upgrade Notes in the release
 * notes for the release in which the feature was marked deprecated.
 *
 * Sample usage (note the slightly different syntax for classes and structures):
 *
 *		UE_DEPRECATED(4.xx, "Message")
 *		void MyFunction();
 *
 *		UE_DEPRECATED(4.xx, "Message")
 *		typedef FThing MyType;
 *
 *		using MyAlias UE_DEPRECATED(4.xx, "Message") = FThing;
 *
 *		UE_DEPRECATED(4.xx, "Message")
 *		int32 MyVariable;
 *
 *		namespace UE_DEPRECATED(4.xx, "Message") MyNamespace
 *		{
 *		}
 *
 *		Unfortunately, clang will complain that [the] "declaration of [an] anonymous class must
 *		be a definition" for API types.  To work around this, first forward declare the type as
 *		deprecated, then declare the type with the visibility macro.  Note that macros like
 *		USTRUCT must immediately precede the the type declaration, not the forward declaration.
 *
 *		struct UE_DEPRECATED(4.xx, "Message") FMyStruct;
 *		USTRUCT()
 *		struct MODULE_API FMyStruct
 *		{
 *		};
 *
 *		class UE_DEPRECATED(4.xx, "Message") FMyClass;
 *		class MODULE_API FMyClass
 *		{
 *		};
 *
 *		enum class UE_DEPRECATED(4.xx, "Message") EMyEnumeration
 *		{
 *			Zero = 0,
 *			One UE_DEPRECATED(4.xx, "Message") = 1,
 *			Two = 2
 *		};
 *
 *		Unfortunately, VC++ will complain about using member functions and fields from deprecated
 *		class/structs even for class/struct implementation e.g.:
 *		class UE_DEPRECATED(4.xx, "") DeprecatedClass
 *		{
 *		public:
 *			DeprecatedClass() {}
 *
 *			float GetMyFloat()
 *			{
 *				return MyFloat; // This line will cause warning that deprecated field is used.
 *			}
 *		private:
 *			float MyFloat;
 *		};
 *
 *		To get rid of this warning, place all code not called in class implementation in non-deprecated
 *		base class and deprecate only derived one. This may force you to change some access specifiers
 *		from private to protected, e.g.:
 *
 *		class DeprecatedClass_Base_DEPRECATED
 *		{
 *		protected: // MyFloat is protected now, so DeprecatedClass has access to it.
 *			float MyFloat;
 *		};
 *
 *		class UE_DEPRECATED(4.xx, "") DeprecatedClass : DeprecatedClass_Base_DEPRECATED
 *		{
 *		public:
 *			DeprecatedClass() {}
 *
 *			float GetMyFloat()
 *			{
 *				return MyFloat;
 *			}
 *		};
 *
 * @param VERSION The release number in which the feature was marked deprecated.
 * @param MESSAGE A message containing upgrade notes.
 */
#define UE_DEPRECATED(Version, Message) [[deprecated(Message " Please update your code to the new API before upgrading to the next release, otherwise your project will no longer compile.")]]

#ifndef UE_DEPRECATED_FORGAME
	#define UE_DEPRECATED_FORGAME(...)
#endif

/*
 * Macro that can be defined in the target file to strip deprecated properties in objects across the engine that check against this define.
 * Can be used by project that have migrated away from using deprecated functions and data members to potentially gain back some memory and perf.
 * @note This is a define that engine developer may use when deprecating properties to allow additional memory savings when a project is compliant with deprecation notice.
 * It doesn't indicate that all deprecated properties will be stripped.
 */
#ifndef UE_STRIP_DEPRECATED_PROPERTIES
	#define UE_STRIP_DEPRECATED_PROPERTIES 0
#endif

template <bool bIsDeprecated>
struct TStaticDeprecateExpression
{
};

/**
 * Can be used in the same contexts as static_assert but gives a warning rather than an error, and 'fails' if the expression is true rather than false.
 */
#define UE_STATIC_DEPRECATE(Version, bExpression, Message) \
	struct PREPROCESSOR_JOIN(FDeprecationMsg_, __LINE__) { \
		[[deprecated(Message " Please update your code to the new API before upgrading to the next release, otherwise your project will no longer compile.")]] \
		static constexpr int condition(TStaticDeprecateExpression<true>) { return 1; } \
		static constexpr int condition(TStaticDeprecateExpression<false>) { return 1; } \
	}; \
	enum class PREPROCESSOR_JOIN(EDeprecationMsg_, __LINE__) { Value = PREPROCESSOR_JOIN(FDeprecationMsg_, __LINE__)::condition(TStaticDeprecateExpression<!!(bExpression)>()) }

// These defines are used to mark a difference between two pointers as expected to fit into the specified range
// while still leaving something searchable if the surrounding code is updated to work with a 64 bit count/range
// in the future
#define UE_PTRDIFF_TO_INT32(argument) static_cast<int32>(argument)
#define UE_PTRDIFF_TO_UINT32(argument) static_cast<uint32>(argument)

/**
* Makes a type non-copyable and non-movable by deleting copy/move constructors and assignment/move operators.
* The macro should be placed in the public section of the type for better compiler diagnostic messages.
* Example usage:
*
*	class FMyClassName
*	{
*	public:
*		UE_NONCOPYABLE(FMyClassName)
*		FMyClassName() = default;
*	};
*/
#define UE_NONCOPYABLE(TypeName) \
	TypeName(TypeName&&) = delete; \
	TypeName(const TypeName&) = delete; \
	TypeName& operator=(const TypeName&) = delete; \
	TypeName& operator=(TypeName&&) = delete;


/** 
 * Handle that defines a local user on this platform.
 * This used to be just a typedef int32 that was used interchangeably as ControllerId and LocalUserIndex.
 * Moving forward these will be allocated by the platform application layer.
 */
struct FPlatformUserId
{
	/** Create a default invalid Id */
	FORCEINLINE FPlatformUserId() : InternalId(INDEX_NONE) {}

	FPlatformUserId(const FPlatformUserId&) = default;
	FPlatformUserId& operator=(const FPlatformUserId&) = default;

	/** Sees if this is a valid user */
	FORCEINLINE bool IsValid() const
	{
		return InternalId != INDEX_NONE;
	}

	/** Returns the internal id for debugging/etc */
	FORCEINLINE int32 GetInternalId() const
	{
		return InternalId;
	}

	/** Explicit function to create from an internal id */
	FORCEINLINE static FPlatformUserId CreateFromInternalId(int32 InInternalId)
	{
		FPlatformUserId IdToReturn;
		IdToReturn.InternalId = InInternalId;
		return IdToReturn;
	}

	FORCEINLINE bool operator==(const FPlatformUserId& Other) const
	{
		return InternalId == Other.InternalId;
	}

	FORCEINLINE bool operator!=(const FPlatformUserId& Other) const
	{
		return InternalId != Other.InternalId;
	}

	FORCEINLINE friend uint32 GetTypeHash(const FPlatformUserId& UserId)
	{
		return UserId.InternalId;
	}

	// This is needed until input and online code handles FPlatformUserId properly
	UE_DEPRECATED(5.0, "Implicit conversion from user index is deprecated, use FPlatformMisc::GetPlatformUserForUserIndex")
	FORCEINLINE constexpr FPlatformUserId(int32 InIndex) : InternalId(InIndex) {}

	// This should be deprecated when the online code uniformly handles FPlatformUserId */
	// UE_DEPRECATED(5.x, "Implicit conversion to user index is deprecated, use FPlatformMisc::GetUserIndexForPlatformUser")
	FORCEINLINE constexpr operator int32() const { return InternalId; }

private:
	/** Raw id, will be allocated by application layer */
	int32 InternalId;
};

/** Static invalid platform user */
const FPlatformUserId PLATFORMUSERID_NONE;
