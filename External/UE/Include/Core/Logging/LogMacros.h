// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/VarArgs.h"
#include "Misc/AssertionMacros.h"
#include "Containers/UnrealString.h"
#include "Logging/LogCategory.h"
#include "Logging/LogScopedCategoryAndVerbosityOverride.h"
#include "Logging/LogTrace.h"
#include "Templates/IsValidVariadicFunctionArg.h"
#include "Templates/AndOrNot.h"
#include "Templates/IsArrayOrRefOfType.h"


/*----------------------------------------------------------------------------
	Logging
----------------------------------------------------------------------------*/

// Define NO_LOGGING to strip out all writing to log files, OutputDebugString(), etc.
// This is needed for consoles that require no logging (Xbox, Xenon)

/**
 * FMsg 
 * This struct contains functions for messaging with tools or debug logs.
 **/
struct CORE_API FMsg
{
	/** Sends a message to a remote tool. */
	static void SendNotificationString( const TCHAR* Message );

	/** Sends a formatted message to a remote tool. */
	template <typename FmtType, typename... Types>
	static void SendNotificationStringf(const FmtType& Fmt, Types... Args)
	{
		static_assert(TIsArrayOrRefOfType<FmtType, TCHAR>::Value, "Formatting string must be a TCHAR array.");
		static_assert(TAnd<TIsValidVariadicFunctionArg<Types>...>::Value, "Invalid argument(s) passed to FMsg::SendNotificationStringf");

		SendNotificationStringfImpl(Fmt, Args...);
	}

	/** Log function */
	template <typename FmtType, typename... Types>
	static void Logf(const ANSICHAR* File, int32 Line, const FLogCategoryName& Category, ELogVerbosity::Type Verbosity, const FmtType& Fmt, Types... Args)
	{
		static_assert(TIsArrayOrRefOfType<FmtType, TCHAR>::Value, "Formatting string must be a TCHAR array.");
		static_assert(TAnd<TIsValidVariadicFunctionArg<Types>...>::Value, "Invalid argument(s) passed to FMsg::Logf");

		LogfImpl(File, Line, Category, Verbosity, Fmt, Args...);
	}

	/** Internal version of log function. Should be used only in logging macros, as it relies on caller to call assert on fatal error */
	template <typename FmtType, typename... Types>
	static void Logf_Internal(const ANSICHAR* File, int32 Line, const FLogCategoryName& Category, ELogVerbosity::Type Verbosity, const FmtType& Fmt, Types... Args)
	{
		static_assert(TIsArrayOrRefOfType<FmtType, TCHAR>::Value, "Formatting string must be a TCHAR array.");
		static_assert(TAnd<TIsValidVariadicFunctionArg<Types>...>::Value, "Invalid argument(s) passed to FMsg::Logf_Internal");

		Logf_InternalImpl(File, Line, Category, Verbosity, Fmt, Args...);
	}

private:
	static void VARARGS LogfImpl(const ANSICHAR* File, int32 Line, const FLogCategoryName& Category, ELogVerbosity::Type Verbosity, const TCHAR* Fmt, ...);
	static void VARARGS Logf_InternalImpl(const ANSICHAR* File, int32 Line, const FLogCategoryName& Category, ELogVerbosity::Type Verbosity, const TCHAR* Fmt, ...);
	static void VARARGS SendNotificationStringfImpl(const TCHAR* Fmt, ...);
};

/*----------------------------------------------------------------------------
	Logging suppression
----------------------------------------------------------------------------*/

#ifndef COMPILED_IN_MINIMUM_VERBOSITY
	#define COMPILED_IN_MINIMUM_VERBOSITY VeryVerbose
#else
	#if !IS_MONOLITHIC
		#error COMPILED_IN_MINIMUM_VERBOSITY can only be defined in monolithic builds.
	#endif
#endif

#define UE_LOG_EXPAND_IS_FATAL(Verbosity, ActiveBlock, InactiveBlock) PREPROCESSOR_JOIN(UE_LOG_EXPAND_IS_FATAL_, Verbosity)(ActiveBlock, InactiveBlock)

#define UE_LOG_EXPAND_IS_FATAL_Fatal(      ActiveBlock, InactiveBlock) ActiveBlock
#define UE_LOG_EXPAND_IS_FATAL_Error(      ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_Warning(    ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_Display(    ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_Log(        ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_Verbose(    ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_VeryVerbose(ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_All(        ActiveBlock, InactiveBlock) InactiveBlock
#define UE_LOG_EXPAND_IS_FATAL_SetColor(   ActiveBlock, InactiveBlock) InactiveBlock

#if DO_CHECK
	#define UE_LOG_SOURCE_FILE(File) File
#else
	#define UE_LOG_SOURCE_FILE(File) "Unknown"
#endif

#if NO_LOGGING

	struct FNoLoggingCategory {};
	
	// This will only log Fatal errors
	#define UE_LOG(CategoryName, Verbosity, Format, ...) \
	{ \
		if (ELogVerbosity::Verbosity == ELogVerbosity::Fatal) \
		{ \
			LowLevelFatalError(Format, ##__VA_ARGS__); \
			UE_LOG_EXPAND_IS_FATAL(Verbosity, CA_ASSUME(false);, PREPROCESSOR_NOTHING) \
		} \
	}
	#define UE_LOG_REF(ConstLogCategoryRef, Verbosity, Format, ...) UE_LOG(ConstLogCategoryRef, Verbosity, Format, __VA_ARGS__ )

	#define UE_LOG_CLINKAGE(CategoryName, Verbosity, Format, ...) UE_LOG(CategoryName, Verbosity, Format, __VA_ARGS__ )

	// Conditional logging (fatal errors only).
	#define UE_CLOG(Condition, CategoryName, Verbosity, Format, ...) \
	{ \
		if (ELogVerbosity::Verbosity == ELogVerbosity::Fatal) \
		{ \
			if (Condition) \
			{ \
				LowLevelFatalError(Format, ##__VA_ARGS__); \
				UE_LOG_EXPAND_IS_FATAL(Verbosity, CA_ASSUME(false);, PREPROCESSOR_NOTHING) \
			} \
		} \
	}

	#define UE_LOG_ACTIVE(...)				(0)
	#define UE_LOG_ANY_ACTIVE(...)			(0)
	#define UE_SUPPRESS(...) {}
	#define UE_GET_LOG_VERBOSITY(...)		(ELogVerbosity::NoLogging)
	#define UE_SET_LOG_VERBOSITY(...)
	#define DECLARE_LOG_CATEGORY_EXTERN(CategoryName, DefaultVerbosity, CompileTimeVerbosity) extern FNoLoggingCategory CategoryName;
	#define DEFINE_LOG_CATEGORY(CategoryName, ...) FNoLoggingCategory CategoryName;
	#define DEFINE_LOG_CATEGORY_STATIC(...)
	#define DECLARE_LOG_CATEGORY_CLASS(...)
	#define DEFINE_LOG_CATEGORY_CLASS(...)
	#define UE_SECURITY_LOG(...)

#else

	namespace UEAsserts_Private
	{
		template <int32 VerbosityToCheck, typename CategoryType>
		FORCEINLINE
			typename TEnableIf<
				((VerbosityToCheck & ELogVerbosity::VerbosityMask) <= CategoryType::CompileTimeVerbosity &&
				(VerbosityToCheck & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY),
				bool>::Type
			IsLogActive(const CategoryType& Category)
		{
			return !Category.IsSuppressed((ELogVerbosity::Type)VerbosityToCheck);
		}

		template <int32 VerbosityToCheck, typename CategoryType>
		FORCEINLINE
			typename TEnableIf<
				!((VerbosityToCheck & ELogVerbosity::VerbosityMask) <= CategoryType::CompileTimeVerbosity &&
				(VerbosityToCheck & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY),
				bool>::Type
			IsLogActive(const CategoryType& Category)
		{
			return false;
		}
	}

	/** 
	 * A predicate that returns true if the given logging category is active (logging) at a given verbosity level 
	 * @param CategoryName name of the logging category
	 * @param Verbosity, verbosity level to test against
	**/
	#define UE_LOG_ACTIVE(CategoryName, Verbosity) (::UEAsserts_Private::IsLogActive<(int32)ELogVerbosity::Verbosity>(CategoryName))

	#define UE_GET_LOG_VERBOSITY(CategoryName) \
		CategoryName.GetVerbosity()

	#define UE_SET_LOG_VERBOSITY(CategoryName, Verbosity) \
		CategoryName.SetVerbosity(ELogVerbosity::Verbosity);

	/** 
	 * INTERNAL IMPLEMENTATION. DO NOT CALL DIRECTLY!
	**/
	#define UE_INTERNAL_LOG_IMPL(CategoryName, Verbosity, Format, ...) \
		static_assert(TIsArrayOrRefOfType<decltype(Format), TCHAR>::Value, "Formatting string must be a TCHAR array."); \
		static_assert((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, "Verbosity must be constant and in range."); \
		UE_LOG_EXPAND_IS_FATAL(Verbosity, PREPROCESSOR_NOTHING, if (!CategoryName.IsSuppressed(ELogVerbosity::Verbosity))) \
			{ \
				DispatchCheckVerify([] (const auto& LCategoryName, const auto& LFormat, const auto&... UE_LOG_Args) FORCENOINLINE \
				{ \
					TRACE_LOG_MESSAGE(LCategoryName, Verbosity, LFormat, UE_LOG_Args...) \
					UE_LOG_EXPAND_IS_FATAL(Verbosity, \
						{ \
							FMsg::Logf_Internal(UE_LOG_SOURCE_FILE(__FILE__), __LINE__, LCategoryName.GetCategoryName(), ELogVerbosity::Verbosity, LFormat, UE_LOG_Args...); \
							_DebugBreakAndPromptForRemote(); \
							FDebug::ProcessFatalError(PLATFORM_RETURN_ADDRESS()); \
						}, \
						{ \
							FMsg::Logf_Internal(nullptr, 0, LCategoryName.GetCategoryName(), ELogVerbosity::Verbosity, LFormat, UE_LOG_Args...); \
						} \
					) \
				}, CategoryName, Format, ##__VA_ARGS__); \
				UE_LOG_EXPAND_IS_FATAL(Verbosity, CA_ASSUME(false);, PREPROCESSOR_NOTHING) \
			} \

	/**
	 * A  macro that outputs a formatted message to log if a given logging category is active at a given verbosity level
	 * @param Category  A reference to a FLogCategoryBase instance
	 * @param Verbosity Verbosity level to test against
	 * @param Format    Format text
	 ***/
	#define UE_LOG_REF(ConstLogCategoryRef, Verbosity, Format, ...) \
	{ \
		CA_CONSTANT_IF((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY && (ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= ConstLogCategoryRef.GetCompileTimeVerbosity()) \
		{ \
			UE_INTERNAL_LOG_IMPL(ConstLogCategoryRef, Verbosity, Format, ##__VA_ARGS__); \
		} \
	}

	/** 
	 * A  macro that outputs a formatted message to log if a given logging category is active at a given verbosity level
	 * @param CategoryName name of the logging category
	 * @param Verbosity, verbosity level to test against
	 * @param Format, format text
	 ***/
	#define UE_LOG(CategoryName, Verbosity, Format, ...) \
	{ \
		CA_CONSTANT_IF((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY && (ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= FLogCategory##CategoryName::CompileTimeVerbosity) \
		{ \
			UE_INTERNAL_LOG_IMPL(CategoryName, Verbosity, Format, ##__VA_ARGS__); \
		} \
	}

	/** 
	 * A  macro that outputs a formatted message to log if a given logging category is active at a given verbosity level
	 * @param CategoryName name of the logging category
	 * @param Verbosity, verbosity level to test against
	 * @param Format, format text
	 ***/
	#define UE_LOG_CLINKAGE(CategoryName, Verbosity, Format, ...) \
	{ \
		static_assert(TIsArrayOrRefOfType<decltype(Format), TCHAR>::Value, "Formatting string must be a TCHAR array."); \
		static_assert((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, "Verbosity must be constant and in range."); \
		CA_CONSTANT_IF((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY && (ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= FLogCategory##CategoryName::CompileTimeVerbosity) \
		{ \
			UE_LOG_EXPAND_IS_FATAL(Verbosity, PREPROCESSOR_NOTHING, if (!CategoryName.IsSuppressed(ELogVerbosity::Verbosity))) \
			{ \
				TRACE_LOG_MESSAGE(CategoryName, Verbosity, Format, ##__VA_ARGS__) \
				UE_LOG_EXPAND_IS_FATAL(Verbosity, \
					{ \
						FMsg::Logf_Internal(UE_LOG_SOURCE_FILE(__FILE__), __LINE__, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, Format,  ##__VA_ARGS__); \
						_DebugBreakAndPromptForRemote(); \
						FDebug::ProcessFatalError(PLATFORM_RETURN_ADDRESS()); \
						CA_ASSUME(false); \
					}, \
					{ \
						FMsg::Logf_Internal(nullptr, 0, CategoryName.GetCategoryName(), ELogVerbosity::Verbosity, Format,  ##__VA_ARGS__); \
					} \
				) \
			} \
		} \
	}
	/**
	* A  macro that outputs a formatted message to the log specifically used for security events
	* @param NetConnection, a valid UNetConnection
	* @param SecurityEventType, a security event type (ESecurityEvent::Type)
	* @param Format, format text
	***/
	#define UE_SECURITY_LOG(NetConnection, SecurityEventType, Format, ...) \
	{ \
		static_assert(TIsArrayOrRefOfType<decltype(Format), TCHAR>::Value, "Formatting string must be a TCHAR array."); \
		check(NetConnection != nullptr); \
		CA_CONSTANT_IF((ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY && (ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= FLogCategoryLogSecurity::CompileTimeVerbosity) \
		{ \
			if (!LogSecurity.IsSuppressed(ELogVerbosity::Warning)) \
			{ \
				FMsg::Logf_Internal(UE_LOG_SOURCE_FILE(__FILE__), __LINE__, LogSecurity.GetCategoryName(), ELogVerbosity::Warning, TEXT("%s: %s: ") Format, *(NetConnection->RemoteAddressToString()), ToString(SecurityEventType), ##__VA_ARGS__); \
			} \
		} \
	}

	// Conditional logging. Will only log if Condition is met.
	#define UE_CLOG(Condition, CategoryName, Verbosity, Format, ...) \
	{ \
		static_assert(TIsArrayOrRefOfType<decltype(Format), TCHAR>::Value, "Formatting string must be a TCHAR array."); \
		static_assert((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, "Verbosity must be constant and in range."); \
		CA_CONSTANT_IF((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY && (ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= FLogCategory##CategoryName::CompileTimeVerbosity) \
		{ \
			UE_LOG_EXPAND_IS_FATAL(Verbosity, PREPROCESSOR_NOTHING, if (!CategoryName.IsSuppressed(ELogVerbosity::Verbosity))) \
			{ \
				if (Condition) \
				{ \
					DispatchCheckVerify([] (const auto& LCategoryName, const auto& LFormat, const auto&... UE_LOG_Args) FORCENOINLINE \
					{ \
						TRACE_LOG_MESSAGE(LCategoryName, Verbosity, LFormat, UE_LOG_Args...) \
						UE_LOG_EXPAND_IS_FATAL(Verbosity, \
							{ \
								FMsg::Logf_Internal(UE_LOG_SOURCE_FILE(__FILE__), __LINE__, LCategoryName.GetCategoryName(), ELogVerbosity::Verbosity, LFormat, UE_LOG_Args...); \
								_DebugBreakAndPromptForRemote(); \
								FDebug::ProcessFatalError(PLATFORM_RETURN_ADDRESS()); \
							}, \
							{ \
								FMsg::Logf_Internal(nullptr, 0, LCategoryName.GetCategoryName(), ELogVerbosity::Verbosity, LFormat, UE_LOG_Args...); \
							} \
						) \
						CA_ASSUME(true); \
					}, CategoryName, Format, ##__VA_ARGS__); \
					UE_LOG_EXPAND_IS_FATAL(Verbosity, CA_ASSUME(false);, PREPROCESSOR_NOTHING) \
				} \
			} \
		} \
	}

	/** 
	 * A macro that executes some code within a scope if a given logging category is active at a given verbosity level
	 * Also, withing the scope of the execution, the default category and verbosity is set up for the low level logging 
	 * functions.
	 * @param CategoryName name of the logging category
	 * @param Verbosity, verbosity level to test against
	 * @param ExecuteIfUnsuppressed, code to execute if the verbosity level for this category is being displayed
	 ***/
	#define UE_SUPPRESS(CategoryName, Verbosity, ExecuteIfUnsuppressed) \
	{ \
		static_assert((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && ELogVerbosity::Verbosity > 0, "Verbosity must be constant and in range."); \
		CA_CONSTANT_IF((ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY && (ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= FLogCategory##CategoryName::CompileTimeVerbosity) \
		{ \
			if (!CategoryName.IsSuppressed(ELogVerbosity::Verbosity)) \
			{ \
				FScopedCategoryAndVerbosityOverride TEMP__##CategoryName(CategoryName.GetCategoryName(), ELogVerbosity::Type(ELogVerbosity::Verbosity & ELogVerbosity::VerbosityMask)); \
				ExecuteIfUnsuppressed; \
				CategoryName.PostTrigger(ELogVerbosity::Verbosity); \
			} \
		} \
	}

	/** 
	 * A macro to declare a logging category as a C++ "extern", usually declared in the header and paired with DEFINE_LOG_CATEGORY in the source. Accessible by all files that include the header.
	 * @param CategoryName, category to declare
	 * @param DefaultVerbosity, default run time verbosity
	 * @param CompileTimeVerbosity, maximum verbosity to compile into the code
	 **/
	#define DECLARE_LOG_CATEGORY_EXTERN(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
		extern struct FLogCategory##CategoryName : public FLogCategory<ELogVerbosity::DefaultVerbosity, ELogVerbosity::CompileTimeVerbosity> \
		{ \
			FORCEINLINE FLogCategory##CategoryName() : FLogCategory(TEXT(#CategoryName)) {} \
		} CategoryName;

	/** 
	 * A macro to define a logging category, usually paired with DECLARE_LOG_CATEGORY_EXTERN from the header.
	 * @param CategoryName, category to define
	**/
	#define DEFINE_LOG_CATEGORY(CategoryName) FLogCategory##CategoryName CategoryName;

	/** 
	 * A macro to define a logging category as a C++ "static". This should ONLY be declared in a source file. Only accessible in that single file.
	 * @param CategoryName, category to declare
	 * @param DefaultVerbosity, default run time verbosity
	 * @param CompileTimeVerbosity, maximum verbosity to compile into the code
	**/
	#define DEFINE_LOG_CATEGORY_STATIC(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
		static struct FLogCategory##CategoryName : public FLogCategory<ELogVerbosity::DefaultVerbosity, ELogVerbosity::CompileTimeVerbosity> \
		{ \
			FORCEINLINE FLogCategory##CategoryName() : FLogCategory(TEXT(#CategoryName)) {} \
		} CategoryName;

	/** 
	 * A macro to declare a logging category as a C++ "class static" 
	 * @param CategoryName, category to declare
	 * @param DefaultVerbosity, default run time verbosity
	 * @param CompileTimeVerbosity, maximum verbosity to compile into the code
	**/
	#define DECLARE_LOG_CATEGORY_CLASS(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
		DEFINE_LOG_CATEGORY_STATIC(CategoryName, DefaultVerbosity, CompileTimeVerbosity)

	/** 
	 * A macro to define a logging category, usually paired with DECLARE_LOG_CATEGORY_CLASS from the header.
	 * @param CategoryName, category to define
	**/
	#define DEFINE_LOG_CATEGORY_CLASS(Class, CategoryName) Class::FLogCategory##CategoryName Class::CategoryName;

#endif // NO_LOGGING

#if UE_BUILD_SHIPPING
#define NOTIFY_CLIENT_OF_SECURITY_EVENT_IF_NOT_SHIPPING(NetConnection, SecurityPrint) ;
#else
#define NOTIFY_CLIENT_OF_SECURITY_EVENT_IF_NOT_SHIPPING(NetConnection, SecurityPrint) \
	FNetControlMessage<NMT_SecurityViolation>::Send(NetConnection, SecurityPrint); \
	NetConnection->FlushNet(true)
#endif

/**
	* A  macro that closes the connection and logs the security event on the server and the client
	* @param NetConnection, a valid UNetConnection
	* @param SecurityEventType, a security event type (ESecurityEvent::Type)
	* @param Format, format text
***/
#define CLOSE_CONNECTION_DUE_TO_SECURITY_VIOLATION_INNER(NetConnection, SecurityEventType, Format, ...) \
{ \
	static_assert(TIsArrayOrRefOfType<decltype(Format), TCHAR>::Value, "Formatting string must be a TCHAR array."); \
	check(NetConnection != nullptr); \
	FString SecurityPrint = FString::Printf(Format, ##__VA_ARGS__); \
	UE_SECURITY_LOG(NetConnection, SecurityEventType, Format, ##__VA_ARGS__); \
	UE_SECURITY_LOG(NetConnection, ESecurityEvent::Closed, TEXT("Connection closed")); \
	NetConnection->Close({FromSecurityEvent(SecurityEventType), SecurityPrint}); \
}
#if USE_SERVER_PERF_COUNTERS
#define CLOSE_CONNECTION_DUE_TO_SECURITY_VIOLATION(NetConnection, SecurityEventType, Format, ...) \
	CLOSE_CONNECTION_DUE_TO_SECURITY_VIOLATION_INNER(NetConnection, SecurityEventType, Format, ##__VA_ARGS__) \
	PerfCountersIncrement(TEXT("ClosedConnectionsDueToSecurityViolations"));
#else
#define CLOSE_CONNECTION_DUE_TO_SECURITY_VIOLATION(NetConnection, SecurityEventType, Format, ...) \
	CLOSE_CONNECTION_DUE_TO_SECURITY_VIOLATION_INNER(NetConnection, SecurityEventType, Format, ##__VA_ARGS__)
#endif

extern CORE_API int32 GEnsureOnNANDiagnostic;

// Macro to either log an error or ensure on a NaN error.
#if DO_CHECK && !USING_CODE_ANALYSIS
namespace UEAsserts_Private
{
	CORE_API void VARARGS InternalLogNANDiagnosticMessage(const TCHAR* FormattedMsg, ...); // UE_LOG(LogCore, Error, _FormatString_, ##__VA_ARGS__);
}
#define logOrEnsureNanError(_FormatString_, ...) \
	if (!GEnsureOnNANDiagnostic)\
	{\
		static bool OnceOnly = false;\
		if (!OnceOnly)\
		{\
			UEAsserts_Private::InternalLogNANDiagnosticMessage(_FormatString_, ##__VA_ARGS__); \
			OnceOnly = true;\
		}\
	}\
	else\
	{\
		ensureMsgf(!GEnsureOnNANDiagnostic, _FormatString_, ##__VA_ARGS__); \
	}
#else
#define logOrEnsureNanError(_FormatString_, ...)
#endif // DO_CHECK
