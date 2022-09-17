// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AccessDetection.h"
#include "Misc/AssertionMacros.h"
#include "Templates/UnrealTemplate.h"
#include "Containers/UnrealString.h"
#include "Logging/LogMacros.h"
#include "Delegates/IDelegateInstance.h"
#include "Delegates/Delegate.h"
#include "Features/IModularFeature.h"
#include "Templates/EnableIf.h"

#define TRACK_CONSOLE_FIND_COUNT !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#if DO_CHECK && (!UE_BUILD_SHIPPING) // Disable even if checks in shipping are enabled.
	#define cvarCheckCode( Code )		checkCode( Code )
#else
	#define cvarCheckCode(...)
#endif

template <class T> class TConsoleVariableData;

/**
 * Console variable usage guide:
 *
 * The variable should be creates early in the initialization but not before (not in global variable construction).
 * Choose the right variable type, consider using a console command if more functionality is needed (see Exec()).
 * Available types: bool, int, float, bool&, int&, float&, string
 * Always provide a good help text, other should be able to understand the function of the console variable by reading this help.
 * The help length should be limited to a reasonable width in order to work well for low res screen resolutions.
 *
 * Usage in the game console:
 *   <COMMAND> ?				print the HELP
 *   <COMMAND>	 				print the current state of the console variable
 *   <COMMAND> x 				set and print the new state of the console variable
 *
 * All variables support auto completion. The single line help that can show up there is currently not connected to the help as the help text
 * is expected to be multi line.
 * The former Exec() system can be used to access the console variables.
 * Use console variables only in main thread.
 * The state of console variables is not network synchronized or serialized (load/save). The plan is to allow to set the state in external files (game/platform/engine/local).
 */

/**
 * Bitmask 0x1, 0x2, 0x4, ..
 */
enum EConsoleVariableFlags
{
	/* Mask for flags. Use this instead of ~ECVF_SetByMask */
	ECVF_FlagMask = 0x0000ffff,

	/**
	 * Default, no flags are set, the value is set by the constructor 
	 */
	ECVF_Default = 0x0,
	/**
	 * Console variables marked with this flag behave differently in a final release build.
	 * Then they are are hidden in the console and cannot be changed by the user.
	 */
	ECVF_Cheat = 0x1,
	/**
	 * Console variables cannot be changed by the user (from console).
	 * Changing from C++ or ini is still possible.
	 */
	ECVF_ReadOnly = 0x4,
	/**
	 * UnregisterConsoleObject() was called on this one.
	 * If the variable is registered again with the same type this object is reactivated. This is good for DLL unloading.
	 */
	ECVF_Unregistered = 0x8,
	/**
	 * This flag is set by the ini loading code when the variable wasn't registered yet.
	 * Once the variable is registered later the value is copied over and the variable is destructed.
	 */
	ECVF_CreatedFromIni = 0x10,
	/**
	 * Maintains another shadow copy and updates the copy with render thread commands to maintain proper ordering.
	 * Could be extended for more/other thread.
 	 * Note: On console variable references it assumes the reference is accessed on the render thread only
	 * (Don't use in any other thread or better don't use references to avoid the potential pitfall).
	 */
	ECVF_RenderThreadSafe = 0x20,

	/* ApplyCVarSettingsGroupFromIni will complain if this wasn't set, should not be combined with ECVF_Cheat */
	ECVF_Scalability = 0x40,

	/* those cvars control other cvars with the flag ECVF_Scalability, names should start with "sg." */
	ECVF_ScalabilityGroup = 0x80,

	/* CVars with this flag will be included in the device profile previews. */
	ECVF_Preview = 0x100,

	// ------------------------------------------------

	/* Set flags */
	ECVF_SetFlagMask =				0x00ff0000,

	// Use to set a cvar without calling all cvar sinks. Much faster, but potentially unsafe. Use only if you know the particular cvar/setting does not require a sink call
	ECVF_Set_NoSinkCall_Unsafe =	0x00010000,

	// ------------------------------------------------

	/* to get some history of where the last value was set by ( useful for track down why a cvar is in a specific state */
	ECVF_SetByMask =				0xff000000,

	// The ECVF_SetBy flags are sorted in override order (weak to strong), the value is not serialized. It only affects the override behavior when calling Set()

	// Lowest priority (default after console variable creation)
	ECVF_SetByConstructor =			0x00000000,
	// Set by scalability groups from Scalability.ini (lower priority than game settings so it's easier to override partially)
	ECVF_SetByScalability =			0x01000000,
	// Default priority for engine-level game user settings, platform-specific settings will override this
	ECVF_SetByGameSetting =			0x02000000,
	// Set by project settings UI or specific sections in ini file (higher priority than game setting to allow enforcing some settings for this project)
	ECVF_SetByProjectSetting =		0x03000000,
	// Used by the [ConsoleVariables] section of Engine.ini as well as FSystemSettings
	ECVF_SetBySystemSettingsIni =	0x04000000,
	// Per device settings using the DeviceProfiles.ini hierarchy (e.g. specific iOS device, higher priority than per project to do device specific settings)
	ECVF_SetByDeviceProfile =		0x05000000,
	// User settable game overrides, used for GameUserSettings fields that need to override device specific settings
	ECVF_SetByGameOverride =		0x06000000,
	// Set by local consolevariables.ini, mostly used for testing multiple projects
	ECVF_SetByConsoleVariablesIni = 0x07000000,
	// Used by some command line parameters, others use the Console priority instead
	ECVF_SetByCommandline =			0x08000000,
	// Used for high priority temporary debugging or operation modes 
	ECVF_SetByCode =				0x09000000,
	// Highest priority used via editor UI or or game/editor interactive console
	ECVF_SetByConsole =				0x0A000000,

	// ------------------------------------------------
};

class IConsoleVariable;

#if !NO_CVARS

/** Console variable delegate type  This is a void callback function. */
DECLARE_DELEGATE_OneParam( FConsoleVariableDelegate, IConsoleVariable* );

/** Console variable multicast delegate type. */
DECLARE_MULTICAST_DELEGATE_OneParam(FConsoleVariableMulticastDelegate, IConsoleVariable*);

/** Console command delegate type (takes no arguments.)  This is a void callback function. */
DECLARE_DELEGATE( FConsoleCommandDelegate );

/** Console command delegate type (with arguments.)  This is a void callback function that always takes a list of arguments. */
DECLARE_DELEGATE_OneParam( FConsoleCommandWithArgsDelegate, const TArray< FString >& );

/** Console command delegate type with a world argument. This is a void callback function that always takes a world. */
DECLARE_DELEGATE_OneParam( FConsoleCommandWithWorldDelegate, UWorld* );

/** Console command delegate type (with a world and arguments.)  This is a void callback function that always takes a list of arguments and a world. */
DECLARE_DELEGATE_TwoParams(FConsoleCommandWithWorldAndArgsDelegate, const TArray< FString >&, UWorld*);

/** Console command delegate type (with a world arguments and output device.)  This is a void callback function that always takes a list of arguments, a world and output device. */
DECLARE_DELEGATE_ThreeParams(FConsoleCommandWithWorldArgsAndOutputDeviceDelegate, const TArray< FString >&, UWorld*, FOutputDevice&);

/** Console command delegate type with the output device passed through. */
DECLARE_DELEGATE_OneParam( FConsoleCommandWithOutputDeviceDelegate, FOutputDevice& );

#else

template <typename DerivedType, typename... ParamTypes>
struct FNullConsoleVariableDelegate
{
	/**
	 * Static: Creates a raw C++ pointer global function delegate
	 */
	template <typename... VarTypes>
	inline static DerivedType CreateStatic(typename TIdentity<void (*)(ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}

	template<typename FunctorType, typename... VarTypes>
	inline static DerivedType CreateLambda(FunctorType&&, VarTypes...)
	{
		return {};
	}

	template<typename UserClass, typename FunctorType, typename... VarTypes>
	inline static DerivedType CreateWeakLambda(UserClass*, FunctorType&&, VarTypes...)
	{
		return {};
	}

	template <typename UserClass, typename... VarTypes>
	inline static DerivedType CreateRaw(UserClass*, typename TMemFunPtrType<false, UserClass, void (ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}
	template <typename UserClass, typename... VarTypes>
	inline static DerivedType CreateRaw(UserClass*, typename TMemFunPtrType<true, UserClass, void (ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}

	template <typename UserClass, typename... VarTypes>
	inline static DerivedType CreateSP(const TSharedRef<UserClass>&, typename TMemFunPtrType<false, UserClass, void (ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}
	template <typename UserClass, typename... VarTypes>
	inline static DerivedType CreateSP(const TSharedRef<UserClass>&, typename TMemFunPtrType<true, UserClass, void (ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}

	template <typename UserClass, typename... VarTypes>
	inline static DerivedType CreateSP(UserClass*, typename TMemFunPtrType<false, UserClass, void (ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}
	template <typename UserClass, typename... VarTypes>
	inline static DerivedType CreateSP(UserClass*, typename TMemFunPtrType<true, UserClass, void (ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}
	
	template <typename UserClass, typename... VarTypes>
	inline static DerivedType CreateThreadSafeSP(const TSharedRef<UserClass, ESPMode::ThreadSafe>&, typename TMemFunPtrType<false, UserClass, void (ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}
	template <typename UserClass, typename... VarTypes>
	inline static DerivedType CreateThreadSafeSP(const TSharedRef<UserClass, ESPMode::ThreadSafe>&, typename TMemFunPtrType<true, UserClass, void (ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}

	template <typename UserClass, typename... VarTypes>
	inline static DerivedType CreateThreadSafeSP(UserClass*, typename TMemFunPtrType<false, UserClass, void (ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}
	template <typename UserClass, typename... VarTypes>
	inline static DerivedType CreateThreadSafeSP(UserClass*, typename TMemFunPtrType<true, UserClass, void (ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}

	template <typename UObjectTemplate, typename... VarTypes>
	inline static DerivedType CreateUFunction(UObjectTemplate*, const FName&, VarTypes...)
	{
		return {};
	}

	template <typename UserClass, typename... VarTypes>
	inline static DerivedType CreateUObject(UserClass*, typename TMemFunPtrType<false, UserClass, void (ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}
	template <typename UserClass, typename... VarTypes>
	inline static DerivedType CreateUObject(UserClass*, typename TMemFunPtrType<true, UserClass, void (ParamTypes..., VarTypes...)>::Type, VarTypes...)
	{
		return {};
	}

	FDelegateHandle GetHandle() const
	{
		return {};
	}

	bool ExecuteIfBound(ParamTypes...)
	{
		return false;
	}
};

struct FConsoleVariableDelegate                            : FNullConsoleVariableDelegate<FConsoleVariableDelegate, IConsoleVariable*> {};
struct FConsoleCommandDelegate                             : FNullConsoleVariableDelegate<FConsoleCommandDelegate> {};
struct FConsoleCommandWithArgsDelegate                     : FNullConsoleVariableDelegate<FConsoleCommandWithArgsDelegate, const TArray<FString>&> {};
struct FConsoleCommandWithWorldDelegate                    : FNullConsoleVariableDelegate<FConsoleCommandWithWorldDelegate, UWorld*> {};
struct FConsoleCommandWithWorldAndArgsDelegate             : FNullConsoleVariableDelegate<FConsoleCommandWithWorldAndArgsDelegate, const TArray<FString>&, UWorld*> {};
struct FConsoleCommandWithWorldArgsAndOutputDeviceDelegate : FNullConsoleVariableDelegate<FConsoleCommandWithWorldArgsAndOutputDeviceDelegate, const TArray<FString>&, UWorld*, FOutputDevice&> {};
struct FConsoleCommandWithOutputDeviceDelegate             : FNullConsoleVariableDelegate<FConsoleCommandWithOutputDeviceDelegate, FOutputDevice&> {};

#endif

template <class T> class TConsoleVariableData;

/**
 * Interface for console objects (variables and commands)
 */
class IConsoleObject
{

public:

	IConsoleObject()
#if TRACK_CONSOLE_FIND_COUNT
		: FindCallCount(0)
#endif
	{}

	virtual ~IConsoleObject() {}

	/**
	 *  @return never 0, can be multi line ('\n')
	 */
	virtual const TCHAR* GetHelp() const = 0;
	/**
	 *  @return never 0, can be multi line ('\n')
	 */
	virtual void SetHelp(const TCHAR* Value) = 0;
	/**
	 * Get the internal state of the flags.
	 */
	virtual EConsoleVariableFlags GetFlags() const = 0;
	/**
	 * Sets the internal flag state to the specified value.
	 */
	virtual void SetFlags(const EConsoleVariableFlags Value) = 0;

	// Convenience methods -------------------------------------

	/**
	 * Removes the specified flags in the internal state.
	 */
	void ClearFlags(const EConsoleVariableFlags Value)
	{
		uint32 New = (uint32)GetFlags() & ~(uint32)Value;
	
		SetFlags((EConsoleVariableFlags)New);
	}
	/**
	 * Test is any of the specified flags is set in the internal state.
	 */
	bool TestFlags(const EConsoleVariableFlags Value) const
	{
		return ((uint32)GetFlags() & (uint32)Value) != 0;
	}

	/**
	 * Casts this object to an IConsoleVariable, returns 0 if it's not
	 */
	virtual class IConsoleVariable* AsVariable()
	{
		return 0; 
	}

	virtual bool IsVariableBool() const { return false; }
	virtual bool IsVariableInt() const { return false; }
	virtual bool IsVariableFloat() const { return false; }
	virtual bool IsVariableString() const { return false; }

	virtual class TConsoleVariableData<bool>* AsVariableBool()
	{
		ensureMsgf(false, TEXT("Attempted to access variable data of a console variable type that doesn't support it.  For example FindTConsoleVariableData* on a FAutoConsoleVariableRef."));
		return 0;
	}

	virtual class TConsoleVariableData<int32>* AsVariableInt()
	{
		ensureMsgf(false, TEXT("Attempted to access variable data of a console variable type that doesn't support it.  For example FindTConsoleVariableData* on a FAutoConsoleVariableRef."));
		return 0; 
	}

	virtual class TConsoleVariableData<float>* AsVariableFloat()
	{
		ensureMsgf(false, TEXT("Attempted to access variable data of a console variable type that doesn't support it.  For example FindTConsoleVariableData* on a FAutoConsoleVariableRef."));
		return 0; 
	}

	virtual class TConsoleVariableData<FString>* AsVariableString()
	{
		ensureMsgf(false, TEXT("Attempted to access variable data of a console variable type that doesn't support it.  For example FindTConsoleVariableData* on a FAutoConsoleVariableRef."));
		return 0;
	}

	/**
	 * Casts this object to an IConsoleCommand, verifying first that it is safe to do so
	 */
	virtual struct IConsoleCommand* AsCommand()
	{
		return 0; 
	}

private: // -----------------------------------------

#if TRACK_CONSOLE_FIND_COUNT
	// no longer pure visual, if that causes problems we can change the interface
	// to track down FindConsoleObject/FindConsoleVariable calls without static
	uint32 FindCallCount;
#endif

	/**
	 *  should only be called by the manager, needs to be implemented for each instance
	 */
	virtual void Release() = 0;

	friend class FConsoleManager;
};

/**
 * Interface for console variables
 */
class IConsoleVariable : public IConsoleObject
{
public:

	/**
	 * Set the internal value from the specified string. 
	 * @param SetBy anything in ECVF_LastSetMask e.g. ECVF_SetByScalability
	 **/
	virtual void Set(const TCHAR* InValue, EConsoleVariableFlags SetBy = ECVF_SetByCode) = 0;

	/**
	 * Get the internal value as a bool, works on bools, ints and floats.
	 */
	virtual bool GetBool() const = 0;
	/**
	 * Get the internal value as int (should not be used on strings).
	 * @return value is not rounded (simple cast)
	 */
	virtual int32 GetInt() const = 0;
	/** Get the internal value as float (works on all types). */
	virtual float GetFloat() const = 0;
	/** Get the internal value as string (works on all types). */
	virtual FString GetString() const = 0;

	/** Generic versions for templated code */
	void GetValue(int32& OutIntValue)
	{
		OutIntValue = GetInt();
	}
	void GetValue(bool& OutBoolValue)
	{
		OutBoolValue = GetBool();
	}
	void GetValue(float& OutFloatValue)
	{
		OutFloatValue = GetFloat();
	}
	void GetValue(FString& OutStringValue)
	{
		OutStringValue = GetString();
	}

	/**
	 * Allows to specify a callback function that is called when the console variable value changes.
 	 * Is even called if the value is the same as the value before. Will always be on the game thread.
	 * This can be dangerous (instead try to use RegisterConsoleVariableSink())
	 * - Setting other console variables in the delegate can cause infinite loops
	 * - Setting many console variables could result in wasteful cycles (e.g. if multiple console variables require to reattach all objects it would happen for each one)
	 * - The call can be at any time during initialization.
	 * As this cannot be specified during constructions you are not called on creation.
	 * We also don't call for the SetOnChangedCallback() call as this is up to the caller.
	 **/
	virtual void SetOnChangedCallback(const FConsoleVariableDelegate& Callback) = 0;

	virtual FConsoleVariableMulticastDelegate& OnChangedDelegate() = 0;

	/**
	 * Get the saved off default value, in a cvar variable, if one was created
	 */
	virtual IConsoleVariable* GetDefaultValueVariable()
	{
		return nullptr;
	}

#if ALLOW_OTHER_PLATFORM_CONFIG

	/**
	 * Get a CVar opject that matches this cvar, but contains the value of the platform given.
	 */
	virtual TSharedPtr<IConsoleVariable> GetPlatformValueVariable(FName PlatformName)
	{
		// this could be called for some special subclass like FConsoleVariableBitRef that don't implement this (yet)
		unimplemented();
		return TSharedPtr<IConsoleVariable>();
	}

	/**
	 * Used only for debugging/iterating, this will clear all of the other platform's cvar objects, which will 
	 * force a fresh lookup (with fresh ini files, likely) on next call to GetPlatformValueVariable
	 */
	virtual void ClearPlatformVariables(FName PlatformName=NAME_None)
	{
	}
#endif

	// convenience methods

	/** Set the internal value from the specified bool. */
	void Set(bool InValue, EConsoleVariableFlags SetBy = ECVF_SetByCode)
	{
		// NOTE: Bool needs to use 1 and 0 here rather than true/false, as this may be a int32 or something
		// and eventually this code calls, TTypeFromString<T>::FromString which won't handle the true/false,
		// but 1 and 0 will work for whatever.
		// inefficient but no common code path
		Set(InValue ? TEXT("1") : TEXT("0"), SetBy);
	}
	/** Set the internal value from the specified int. */
	void Set(int32 InValue, EConsoleVariableFlags SetBy = ECVF_SetByCode)
	{
		// inefficient but no common code path
		Set(*FString::Printf(TEXT("%d"), InValue), SetBy);
	}
	/** Set the internal value from the specified float. */
	void Set(float InValue, EConsoleVariableFlags SetBy = ECVF_SetByCode)
	{
		// inefficient but no common code path
		Set(*FString::Printf(TEXT("%g"), InValue), SetBy);
	}

	void SetWithCurrentPriority(bool InValue)
	{
		EConsoleVariableFlags CurFlags = (EConsoleVariableFlags)(GetFlags() & ECVF_SetByMask);
		Set(InValue, CurFlags);
	}
	void SetWithCurrentPriority(int32 InValue)
	{
		EConsoleVariableFlags CurFlags = (EConsoleVariableFlags)(GetFlags() & ECVF_SetByMask);
		Set(InValue, CurFlags);
	}
	void SetWithCurrentPriority(float InValue)
	{
		EConsoleVariableFlags CurFlags = (EConsoleVariableFlags)(GetFlags() & ECVF_SetByMask);
		Set(InValue, CurFlags);
	}
	void SetWithCurrentPriority(const TCHAR* InValue)
	{
		EConsoleVariableFlags CurFlags = (EConsoleVariableFlags)(GetFlags() & ECVF_SetByMask);
		Set(InValue, CurFlags);
	}


};

/**
 * Interface for console commands
 */
struct IConsoleCommand : public IConsoleObject
{
	/**
	 * Executes this command (optionally, with arguments)
	 *
	 * @param	Args		Argument list for this command
	 * @param	InWorld		World context for this command
	 * @return	True if the delegate for this command was executed successfully
	 */
	virtual bool Execute( const TArray< FString >& Args, UWorld* InWorld, class FOutputDevice& OutputDevice ) = 0;
};

/**
 * Interface to propagate changes of console variables to another thread
 */
struct IConsoleThreadPropagation
{
	virtual void OnCVarChange(int32& Dest, int32 NewValue) = 0;
	virtual void OnCVarChange(float& Dest, float NewValue) = 0;
	virtual void OnCVarChange(bool& Dest, bool NewValue) = 0;
	virtual void OnCVarChange(FString& Dest, const FString& NewValue) = 0;
};

/**
 * Declares a delegate type that's used by the console manager to call back into a user function for each
 * known console object.
 *
 * First parameter is the Name string for the current console object
 * Second parameter is the current console object
 */
DECLARE_DELEGATE_TwoParams( FConsoleObjectVisitor, const TCHAR*, IConsoleObject* );


/**
 * Class representing an handle to an online delegate.
 */
class FConsoleVariableSinkHandle
{
public:
	FConsoleVariableSinkHandle()
	{
	}

	explicit FConsoleVariableSinkHandle(FDelegateHandle InHandle)
		: Handle(InHandle)
	{
	}

	template <typename MulticastDelegateType>
	void RemoveFromDelegate(MulticastDelegateType& MulticastDelegate)
	{
		MulticastDelegate.Remove(Handle);
	}

	template <typename DelegateType>
	bool HasSameHandle(const DelegateType& Delegate) const
	{
		return Delegate.GetHandle() == Handle;
	}

private:
	FDelegateHandle Handle;
};


/**
 * Handles executing console commands
 */
class IConsoleCommandExecutor : public IModularFeature
{
public:
	virtual ~IConsoleCommandExecutor() = default;

	/**
	 * Get the name identifying this modular feature set.
	 */
	static FName ModularFeatureName()
	{
		static const FName Name = TEXT("ConsoleCommandExecutor");
		return Name;
	}

	/**
	 * Get the name of this executor.
	 */
	virtual FName GetName() const = 0;

	/**
	 * Get the display name of this executor.
	 */
	virtual FText GetDisplayName() const = 0;

	/**
	 * Get the description of this executor.
	 */
	virtual FText GetDescription() const = 0;

	/**
	 * Get the hint text of this executor.
	 */
	virtual FText GetHintText() const = 0;

	/**
	 * Get the list of auto-complete suggestions for the given command.
	 */
	virtual void GetAutoCompleteSuggestions(const TCHAR* Input, TArray<FString>& Out) = 0;

	/**
	 * Get the list of commands that this executor has recently processed.
	 */
	virtual void GetExecHistory(TArray<FString>& Out) = 0;

	/**
	 * Execute the given command using this executor.
	 * @return true if the command was recognized.
	 */
	virtual bool Exec(const TCHAR* Input) = 0;

	/**
	 * True if we allow the console to be closed using the "open console" hot-key.
	 * @note Some scripting languages use the default "open console" hot-key (~) in their code, so these should return false.
	 */
	virtual bool AllowHotKeyClose() const = 0;

	/**
	 * True if we allow the console to create multi-line commands.
	 */
	virtual bool AllowMultiLine() const = 0;

	/**
	* Returns the hotkey for this executor
	*/
	virtual struct FInputChord GetHotKey() const = 0;
};


/**
 * handles console commands and variables, registered console variables are released on destruction
 */
struct CORE_API IConsoleManager
{
	/**
	 * Create a bool console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, bool DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	/**
	 * Create a int console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, int32 DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	/**
	 * Create a float console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, float DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	/**
	 * Create a string console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, const TCHAR* DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	/**
	 * Create a string console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, const FString& DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	/**
	 * Create a reference to a bool console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	virtual IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* Name, bool& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	/**
	 * Create a reference to a int console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	virtual IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* Name, int32& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	/**
	 * Create a reference to a float console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	virtual IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* Name, float& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	/**
	* Create a reference to a string console variable
	* @param Name must not be 0
	* @param Help must not be 0
	* @param Flags bitmask combined from EConsoleVariableFlags
	*/
	virtual IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* Name, FString& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	/**
	 * Create a reference to a show flag variable
	 * @param CVarName must not be 0, e.g. "Show.PostProcessing"
	 * @param FlagName must not be 0, e.g. "PostProcessing"
	 * @param BitNumber in the memory defined by Force0MaskPtr and Force1MaskPtr
	 * @param Force0MaskPtr memory that contains the bits that should be forced to 0
	 * @param Force1MaskPtr memory that contains the bits that should be forced to 1
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	virtual IConsoleVariable* RegisterConsoleVariableBitRef(const TCHAR* CVarName, const TCHAR* FlagName, uint32 BitNumber, uint8* Force0MaskPtr, uint8* Force1MaskPtr, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;

	// ----------

	/**
	 * The sinks are only called if a change has been done since the last time
	 * Should be called in very few points:
	 *  - after ini file loading
	 *  - after user console input
	 *  - user initiated a console variable change (it needs to be clear to user that a cvar can change e.g. game options menu)
	 *  - beginning of Tick (to catch stray Set() calls, which are usually bad)
	 */
	virtual void CallAllConsoleVariableSinks() = 0;

	/**
	 * The registered command is executed at few defined points (see CallAllConsoleVariableSinks)
	 * @param Command
	 */
	virtual FConsoleVariableSinkHandle RegisterConsoleVariableSink_Handle(const FConsoleCommandDelegate& Command) = 0;

	/**
	 * The registered command is executed at few defined points (see CallAllConsoleVariableSinks)
	 * @param Command
	 */
	virtual void UnregisterConsoleVariableSink_Handle(FConsoleVariableSinkHandle Handle) = 0;

	// ----------

	/**
	 * Register a console command that takes no arguments
	 *
	 * @param	Name		The name of this command (must not be nullptr)
	 * @param	Help		Help text for this command
	 * @param	Command		The user function to call when this command is executed
	 * @param	Flags		Optional flags bitmask
	 */
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandDelegate& Command, uint32 Flags = ECVF_Default) = 0;

	/**
	 * Register a console command that takes arguments
	 *
	 * @param	Name		The name of this command (must not be nullptr)
	 * @param	Help		Help text for this command
	 * @param	Command		The user function to call when this command is executed
	 * @param	Flags		Optional flags bitmask
	 */
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithArgsDelegate& Command, uint32 Flags = ECVF_Default) = 0;

	/**
	 * Register a console command that takes arguments
	 *
	 * @param	Name		The name of this command (must not be nullptr)
	 * @param	Help		Help text for this command
	 * @param	Command		The user function to call when this command is executed
	 * @param	Flags		Optional flags bitmask
	 */
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldDelegate& Command, uint32 Flags = ECVF_Default) = 0;

	/**
	* Register a console command that takes arguments
	*
	* @param	Name		The name of this command (must not be nullptr)
	* @param	Help		Help text for this command
	* @param	Command		The user function to call when this command is executed
	* @param	Flags		Optional flags bitmask
	*/
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldAndArgsDelegate& Command, uint32 Flags = ECVF_Default) = 0;

	/**
	* Register a console command that takes arguments
	*
	* @param	Name		The name of this command (must not be nullptr)
	* @param	Help		Help text for this command
	* @param	Command		The user function to call when this command is executed
	* @param	Flags		Optional flags bitmask
	*/
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldArgsAndOutputDeviceDelegate& Command, uint32 Flags = ECVF_Default) = 0;

	/**
	 * Register a console command that takes arguments
	 *
	 * @param	Name		The name of this command (must not be nullptr)
	 * @param	Help		Help text for this command
	 * @param	Command		The user function to call when this command is executed
	 * @param	Flags		Optional flags bitmask
	 */
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithOutputDeviceDelegate& Command, uint32 Flags = ECVF_Default) = 0;

	/**
	 * Register a console command that is handles by an Exec functions (for auto completion)
	 *
	 * @param	Name		The name of this command (must not be nullptr)
	 * @param	Help		Help text for this command
	 * @param	Flags		Optional flags bitmask
	 */
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, uint32 Flags = (uint32)ECVF_Default) = 0;

	/**
	 * Unregisters a console object, if that object was registered. O(n), n is the console object count
	 *
	 * @param ConsoleObject - object to remove
	 * @param bKeepState if the current state is kept in memory until a cvar with the same name is registered
	 */
	virtual void UnregisterConsoleObject( IConsoleObject* ConsoleObject, bool bKeepState = true) = 0;

	/**
	 * Unregisters a console variable or command by name, if an object of that name was registered.
	 *
	 * @param Name - name of object to remove
	 * @param bKeepState if the current state is kept in memory until a cvar with the same name is registered
	 */
	virtual void UnregisterConsoleObject(const TCHAR* Name, bool bKeepState = true) = 0;

	/**
	 * Find a console variable
	 * @param Name must not be 0
	 * @return 0 if the object wasn't found
	 */
	virtual IConsoleVariable* FindConsoleVariable(const TCHAR* Name, bool bTrackFrequentCalls = true) const = 0;

	/**
	* Find a console variable or command
	* @param Name must not be 0
	* @return 0 if the object wasn't found
	*/
	virtual IConsoleObject* FindConsoleObject(const TCHAR* Name, bool bTrackFrequentCalls = true) const = 0;

	/**
	 * Lookup the name of a console object by its pointer 
	 * @param Object to lookup
	 * @return Name of the object, or an empty string if the object can't be found
	 */
	virtual FString FindConsoleObjectName(const IConsoleObject* Obj) const = 0;

	/**
	 * Find a typed console variable (faster access to the value, no virtual function call)
	 * @param Name must not be 0
	 * @return 0 if the object wasn't found
	 */
	TConsoleVariableData<int32>* FindTConsoleVariableDataInt(const TCHAR* Name) const 
	{ 
		IConsoleVariable* P = FindConsoleVariable(Name); 
		
		return P ? P->AsVariableInt() : 0; 
	}

	/**
	 * Find a typed console variable (faster access to the value, no virtual function call)
	 * @param Name must not be 0
	 * @return 0 if the object wasn't found
	 */
	TConsoleVariableData<float>* FindTConsoleVariableDataFloat(const TCHAR* Name) const 
	{ 
		IConsoleVariable* P = FindConsoleVariable(Name); 

		return P ? P->AsVariableFloat() : 0; 
	}


	/**
	 *  Iterate in O(n), not case sensitive, does not guarantee that UnregisterConsoleObject() will work in the loop
	 *  @param Visitor must not be 0
	 *  @param ThatStartsWith must not be 0 
	 */
	virtual void ForEachConsoleObjectThatStartsWith( const FConsoleObjectVisitor& Visitor, const TCHAR* ThatStartsWith = TEXT("")) const = 0;

	/**
	 *  Not case sensitive, does not guarantee that UnregisterConsoleObject() will work in the loop
	 *  @param Visitor must not be 0
	 *  @param ThatContains must not be 0 
	 */
	virtual void ForEachConsoleObjectThatContains(const FConsoleObjectVisitor& Visitor, const TCHAR* ThatContains) const = 0;

	/**
	 * Process user input
	 *  e.g.
	 *  "MyCVar" to get the current value of the console variable
	 *  "MyCVar -5.2" to set the value to -5.2
	 *  "MyCVar ?" to get the help text
	 *  @param	Input		must not be 0
	 *  @param	Ar			archive
	 *  @param	InWorld		world context
	 *  @return true if the command was recognized
	 */
	virtual bool ProcessUserConsoleInput(const TCHAR* Input, FOutputDevice& Ar, UWorld* InWorld) = 0;

	/**
	 * @param Input - must not be 0
	 */
	virtual void AddConsoleHistoryEntry(const TCHAR* Key, const TCHAR* Input) = 0;
	
	/**
	 */
	virtual void GetConsoleHistory(const TCHAR* Key, TArray<FString>& Out) = 0; 

	/**
	 * Check if a name (command or variable) has been registered with the console manager
	 * @param Name - Name to check. Must not be 0
	 */
	virtual bool IsNameRegistered(const TCHAR* Name) const = 0;

	// currently only for render thread
	// @param InCallback 0 to disable the callbacks
	virtual void RegisterThreadPropagation(uint32 ThreadId = 0, IConsoleThreadPropagation* InCallback = 0) = 0;

	/** Returns the singleton for the console manager **/
	FORCEINLINE static IConsoleManager& Get()
	{
		if (!Singleton)
		{
			SetupSingleton();
			check(Singleton != nullptr);
		}
		return *Singleton;
	}

#if ALLOW_OTHER_PLATFORM_CONFIG
	/**
	 * Walks over the best set of ini sections for another platform that can be used to emulate cvars as the platform
	 * will have set - WITH THE NOTABLE EXCEPTION OF specific DeviceProfiles. Use the DeviceProfileManager code to get a DP
	 * for a platform, as this code cannot easily access DeviceProfile logic.
	 * It also won't include any UserSettings or ConsoleVariables.ini settings.
	 * 
	 * @param PlatformName The platform name (the ini name, so Windows, not Win64)
	 * @param bVisitPlatformDeviceProfile Set to true if you want Visit called with CVars found in the base DeviceProfile named PlatformName 
	 * @param Visit the callback to run for each CVar found
	 */
	static bool VisitPlatformCVarsForEmulation(FName PlatformName, bool bVisitPlatformDeviceProfile, TFunctionRef<void(const FString& CVarName, const FString& CVarValue, EConsoleVariableFlags SetBy)> Visit);
#endif

protected:
	virtual ~IConsoleManager() { }

private:
	/** Singleton for the console manager **/
	static IConsoleManager* Singleton;

	/** Function to create the singleton **/
	static void SetupSingleton();
};


/**
 * auto registering console variable sinks (register a callback function that is called when ever a cvar is changes by the user, changes are grouped and happen in specific engine spots during the frame/main loop)
 */
class CORE_API FAutoConsoleVariableSink
{
public:
	/** Constructor, saves the argument for future removal from the console variable system **/
	FAutoConsoleVariableSink(const FConsoleCommandDelegate& InCommand)
		: Command(InCommand)
	{
		Handle = IConsoleManager::Get().RegisterConsoleVariableSink_Handle(Command);
	}
	/** Destructor, removes the console variable sink **/
	virtual ~FAutoConsoleVariableSink()
	{
//disabled for now, destruction order makes this not always working		IConsoleManager::Get().UnregisterConsoleVariableSink_Handle(Handle);
	}

	const FConsoleCommandDelegate& Command;
	FConsoleVariableSinkHandle Handle;
};


/**
 * Base class for autoregistering console commands.
 */
class CORE_API FAutoConsoleObject
{
protected:
	/** Constructor, saves the argument for future removal from the console variable system **/
	FAutoConsoleObject(IConsoleObject* InTarget)
		: Target(InTarget)
	{
		check(Target);
	}
	/** Destructor, removes the console object **/
	virtual ~FAutoConsoleObject()
	{
		IConsoleManager::Get().UnregisterConsoleObject(Target);
	}

public:
	/** returns the contained console object as an IConsoleVariable **/
	FORCEINLINE IConsoleVariable* AsVariable()
	{
		checkSlow(Target->AsVariable());
		return static_cast<IConsoleVariable*>(Target);
	}
	/** returns the contained console object as an IConsoleVariable **/
	FORCEINLINE const IConsoleVariable* AsVariable() const
	{
		checkSlow(Target->AsVariable());
		return static_cast<const IConsoleVariable*>(Target);
	}

private:
	/** Contained console object, cannot be 0 **/
	IConsoleObject* Target;
};

#if !NO_CVARS
/**
 * Autoregistering float, int or string console variable
 */
class CORE_API FAutoConsoleVariable : private FAutoConsoleObject
{
public:
	/**
	 * Create a bool console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariable(const TCHAR* Name, bool DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariable(Name, DefaultValue, Help, Flags))
	{
	}
	/**
	 * Create a int console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariable(const TCHAR* Name, int32 DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariable(Name, DefaultValue, Help, Flags))
	{
	}
	/**
	 * Create a float console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariable(const TCHAR* Name, float DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariable(Name, DefaultValue, Help, Flags))
	{
	}
	/**
	 * Create a string console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariable(const TCHAR* Name, const TCHAR* DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariable(Name, DefaultValue, Help, Flags))
	{
	}

	/**
	 * Create a bool console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Callback Delegate called when the variable changes. @see IConsoleVariable::SetOnChangedCallback
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariable(const TCHAR* Name, bool DefaultValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariable(Name, DefaultValue, Help, Flags))
	{
		AsVariable()->SetOnChangedCallback(Callback);
	}
	
	/**
	 * Create a int console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Callback Delegate called when the variable changes. @see IConsoleVariable::SetOnChangedCallback
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariable(const TCHAR* Name, int32 DefaultValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariable(Name, DefaultValue, Help, Flags))
	{
		AsVariable()->SetOnChangedCallback(Callback);
	}

	/**
	 * Create a float console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Callback Delegate called when the variable changes. @see IConsoleVariable::SetOnChangedCallback
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariable(const TCHAR* Name, float DefaultValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariable(Name, DefaultValue, Help, Flags))
	{
		AsVariable()->SetOnChangedCallback(Callback);
	}

	/**
	 * Create a string console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Callback Delegate called when the variable changes. @see IConsoleVariable::SetOnChangedCallback
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariable(const TCHAR* Name, const TCHAR* DefaultValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariable(Name, DefaultValue, Help, Flags))
	{
		AsVariable()->SetOnChangedCallback(Callback);
	}

	/** Dereference back to a console variable**/
	FORCEINLINE IConsoleVariable& operator*()
	{
		return *AsVariable();
	}
	FORCEINLINE const IConsoleVariable& operator*() const
	{
		return *AsVariable();
	}
	/** Dereference back to a console variable**/
	FORCEINLINE IConsoleVariable* operator->()
	{
		return AsVariable();
	}
	FORCEINLINE const IConsoleVariable* operator->() const
	{
		return AsVariable();
	}
};
#else
class CORE_API FAutoConsoleVariable
{
public:
	FAutoConsoleVariable(const TCHAR* Name, int32 DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
	{
	}

	FAutoConsoleVariable(const TCHAR* Name, float DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
	{
	}

	FAutoConsoleVariable(const TCHAR* Name, const TCHAR* DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
	{
	}
};
#endif

#if !NO_CVARS
/**
 * Autoregistering float, int, bool, FString REF variable class...this changes that value when the console variable is changed. 
 */
class CORE_API FAutoConsoleVariableRef : private FAutoConsoleObject
{
public:
	/**
	 * Create a reference to a int console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariableRef(const TCHAR* Name, int32& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariableRef(Name, RefValue, Help, Flags))
	{
	}
	/**
	 * Create a reference to a float console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariableRef(const TCHAR* Name, float& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariableRef(Name, RefValue, Help, Flags))
	{
	}
	/**
	 * Create a reference to a bool console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariableRef(const TCHAR* Name, bool& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariableRef(Name, RefValue, Help, Flags))
	{
	}
	/**
	 * Create a reference to a FString console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariableRef(const TCHAR* Name, FString& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariableRef(Name, RefValue, Help, Flags))
	{
	}

	/**
	 * Create a reference to a int console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Callback Delegate called when the variable changes. @see IConsoleVariable::SetOnChangedCallback
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariableRef(const TCHAR* Name, int32& RefValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariableRef(Name, RefValue, Help, Flags))
	{
		AsVariable()->SetOnChangedCallback(Callback);
	}

	/**
	 * Create a reference to a float console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Callback Delegate called when the variable changes. @see IConsoleVariable::SetOnChangedCallback
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariableRef(const TCHAR* Name, float& RefValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariableRef(Name, RefValue, Help, Flags))
	{
		AsVariable()->SetOnChangedCallback(Callback);
	}

	/**
	 * Create a reference to a bool console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Callback Delegate called when the variable changes. @see IConsoleVariable::SetOnChangedCallback
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariableRef(const TCHAR* Name, bool& RefValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariableRef(Name, RefValue, Help, Flags))
	{
		AsVariable()->SetOnChangedCallback(Callback);
	}

	/**
	 * Create a reference to a FString console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Callback Delegate called when the variable changes. @see IConsoleVariable::SetOnChangedCallback
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	FAutoConsoleVariableRef(const TCHAR* Name, FString& RefValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariableRef(Name, RefValue, Help, Flags))
	{
		AsVariable()->SetOnChangedCallback(Callback);
	}

	virtual ~FAutoConsoleVariableRef()
	{
	}
	/** Dereference back to a variable**/
	FORCEINLINE IConsoleVariable& operator*()
	{
		return *AsVariable();
	}
	FORCEINLINE const IConsoleVariable& operator*() const
	{
		return *AsVariable();
	}
	/** Dereference back to a variable**/
	FORCEINLINE IConsoleVariable* operator->()
	{
		return AsVariable();
	}
	FORCEINLINE const IConsoleVariable* operator->() const
	{
		return AsVariable();
	}
};
#else
class CORE_API FAutoConsoleVariableRef
{
public:
	FAutoConsoleVariableRef(const TCHAR* Name, int32& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
	{
	}

	FAutoConsoleVariableRef(const TCHAR* Name, float& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
	{
	}

	FAutoConsoleVariableRef(const TCHAR* Name, bool& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
	{
	}

	FAutoConsoleVariableRef(const TCHAR* Name, FString& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default)
	{
	}

	FAutoConsoleVariableRef(const TCHAR* Name, int32& RefValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
	{
	}

	FAutoConsoleVariableRef(const TCHAR* Name, float& RefValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
	{
	}
	
	FAutoConsoleVariableRef(const TCHAR* Name, bool& RefValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
	{
	}

	FAutoConsoleVariableRef(const TCHAR* Name, FString& RefValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
	{
	}
};
#endif // NO_CVARS


// currently only supports main and render thread
// optimized for read access speed (no virtual function call and no thread handling if using the right functions)
// T: int32, float
template <class T>
class TConsoleVariableData
{
public:
	// constructor
	TConsoleVariableData(const T DefaultValue)
	{
		for(uint32 i = 0; i < UE_ARRAY_COUNT(ShadowedValue); ++i)
		{
			ShadowedValue[i] = DefaultValue;
		}
	}

	// faster than GetValueOnAnyThread()
	T GetValueOnGameThread() const
	{
		UE::AccessDetection::ReportAccess(UE::AccessDetection::EType::CVar);
		// compiled out in shipping for performance (we can change in development later), if this get triggered you need to call GetValueOnRenderThread() or GetValueOnAnyThread(), the last one is a bit slower
		cvarCheckCode(ensure(GetShadowIndex() == 0));	// ensure to not block content creators, #if to optimize in shipping
		return ShadowedValue[0];
	}

	// faster than GetValueOnAnyThread()
	T GetValueOnRenderThread() const
	{
		UE::AccessDetection::ReportAccess(UE::AccessDetection::EType::CVar);
#if !defined(__clang__) // @todo Mac: figure out how to make this compile
		// compiled out in shipping for performance (we can change in development later), if this get triggered you need to call GetValueOnGameThread() or GetValueOnAnyThread(), the last one is a bit slower
		cvarCheckCode(ensure(IsInParallelRenderingThread()));	// ensure to not block content creators, #if to optimize in shipping
#endif
		return ShadowedValue[1];
	}

	// convenient, for better performance consider using GetValueOnGameThread() or GetValueOnRenderThread()
	T GetValueOnAnyThread(bool bForceGameThread = false) const
	{
		UE::AccessDetection::ReportAccess(UE::AccessDetection::EType::CVar);
		return ShadowedValue[GetShadowIndex(bForceGameThread)];
	}

private: // ----------------------------------------------------

	// [0]:main thread, [1]: render thread, having them both in the same cache line should only hurt on write which happens rarely for cvars
	T ShadowedValue[2];

	// @return 0:main thread, 1: render thread, later more
	static uint32 GetShadowIndex(bool bForceGameThread = false)
	{	
		if (bForceGameThread)
		{
			cvarCheckCode(ensure(!IsInActualRenderingThread()));
			return 0;
		}
		return IsInParallelGameThread() || IsInGameThread() ? 0 : 1;
	}

	// needed for FConsoleVariable and FConsoleVariableRef2, intentionally not public
	T& GetReferenceOnAnyThread(bool bForceGameThread = false)
	{
		UE::AccessDetection::ReportAccess(UE::AccessDetection::EType::CVar);
		return ShadowedValue[GetShadowIndex(bForceGameThread)];
	}

	template<class T2> friend class FConsoleVariable;
	template<class T2> friend class TAutoConsoleVariable;
};

#if !NO_CVARS
/**
 * Autoregistering float, int variable class...this changes that value when the console variable is changed. 
 */
template <class T>
class TAutoConsoleVariable : public FAutoConsoleObject
{
public:
	/**
	 * Create a float, int or string console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	TAutoConsoleVariable(const TCHAR* Name, const T& DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default);

	/**
	 * Create a float, int or string console variable
	 * @param Name must not be 0
	 * @param Help must not be 0
	 * @param Callback Delegate called when the variable changes. @see IConsoleVariable::SetOnChangedCallback
	 * @param Flags bitmask combined from EConsoleVariableFlags
	 */
	TAutoConsoleVariable(const TCHAR* Name, const T& DefaultValue, const TCHAR* Help, const FConsoleVariableDelegate& Callback, uint32 Flags = ECVF_Default)
		: TAutoConsoleVariable(Name, DefaultValue, Help, Flags)
	{
		AsVariable()->SetOnChangedCallback(Callback);
	}

	T GetValueOnGameThread() const
	{
		return Ref->GetValueOnGameThread();
	}

	T GetValueOnRenderThread() const
	{
		return Ref->GetValueOnRenderThread();
	}

	T GetValueOnAnyThread(bool bForceGameThread = false) const
	{
		return Ref->GetValueOnAnyThread(bForceGameThread);
	}
	
	/** Dereference back to a variable**/
	FORCEINLINE IConsoleVariable& operator*()
	{
		return *AsVariable();
	}
	FORCEINLINE const IConsoleVariable& operator*() const
	{
		return *AsVariable();
	}
	/** Dereference back to a variable**/
	FORCEINLINE IConsoleVariable* operator->()
	{
		return AsVariable();
	}
	FORCEINLINE const IConsoleVariable* operator->() const
	{
		return AsVariable();
	}
private:
	TConsoleVariableData<T>* Ref;
};

template <>
inline TAutoConsoleVariable<bool>::TAutoConsoleVariable(const TCHAR* Name, const bool& DefaultValue, const TCHAR* Help, uint32 Flags)
	: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariable(Name, DefaultValue, Help, Flags))
{
	Ref = AsVariable()->AsVariableBool();
}

template <>
inline TAutoConsoleVariable<int32>::TAutoConsoleVariable(const TCHAR* Name, const int32& DefaultValue, const TCHAR* Help, uint32 Flags)
	: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariable(Name, DefaultValue, Help, Flags))
{
	Ref = AsVariable()->AsVariableInt();
}

template <>
inline TAutoConsoleVariable<float>::TAutoConsoleVariable(const TCHAR* Name, const float& DefaultValue, const TCHAR* Help, uint32 Flags)
	: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariable(Name, DefaultValue, Help, Flags))
{
	Ref = AsVariable()->AsVariableFloat();
}

template <>
inline TAutoConsoleVariable<FString>::TAutoConsoleVariable(const TCHAR* Name, const FString& DefaultValue, const TCHAR* Help, uint32 Flags)
	: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleVariable(Name, DefaultValue, Help, Flags))
{
	Ref = AsVariable()->AsVariableString();
}
#else
template <class T>
class TAutoConsoleVariable : public IConsoleVariable
{
public:
	TAutoConsoleVariable(const TCHAR* Name, const T& DefaultValue, const TCHAR* InHelp, uint32 InFlags = ECVF_Default)
		: Value(DefaultValue), Flags((EConsoleVariableFlags)InFlags)
	{
	}

	T GetValueOnGameThread() const
	{
		return Value.GetValueOnGameThread();
	}

	T GetValueOnRenderThread() const
	{
		return Value.GetValueOnRenderThread();
	}

	T GetValueOnAnyThread(bool bForceGameThread = false) const
	{
		return Value.GetValueOnAnyThread(bForceGameThread);
	}
	
	IConsoleVariable& operator*()
	{
		return *AsVariable();
	}

	const IConsoleVariable& operator*() const
	{
		return *AsVariable();
	}

	IConsoleVariable* operator->()
	{
		return AsVariable();
	}

	const IConsoleVariable* operator->() const
	{
		return AsVariable();
	}

	IConsoleVariable*		AsVariable()		{ return this; }
	const IConsoleVariable* AsVariable() const	{ return this; }

	virtual class TConsoleVariableData<int32>*		AsVariableInt()		override { return AsImpl<int32>(); }
	virtual class TConsoleVariableData<float>*		AsVariableFloat()	override { return AsImpl<float>(); }
	virtual class TConsoleVariableData<FString>*	AsVariableString()	override { return AsImpl<FString>(); }

	virtual bool		IsVariableInt() const override	{ return TIsSame<int32, T>::Value; }
	virtual int32		GetInt()		const override	{ return GetImpl<int32>(); }
	virtual float		GetFloat()		const override	{ return GetImpl<float>(); }
	virtual FString		GetString()		const override	{ return GetImpl<FString>(); }
	virtual bool		GetBool()		const override { return GetImpl<bool>(); }

	virtual const TCHAR* GetHelp() const override
	{
		return TEXT("NO_CVARS, no help");
	}

	virtual void SetHelp(const TCHAR* InHelp) override
	{
		check(false);
	}

	virtual void Release() override
	{
		check(false);
	}

	virtual void SetOnChangedCallback(const FConsoleVariableDelegate &) override
	{
		check(false);
	}

	virtual FConsoleVariableMulticastDelegate& OnChangedDelegate()override
	{
		static FConsoleVariableMulticastDelegate Dummy;
		check(false);
		return Dummy;
	}

	virtual EConsoleVariableFlags GetFlags() const override
	{
		return Flags;
	}

	virtual void SetFlags(const EConsoleVariableFlags InFlags) override
	{
		Flags = InFlags;
	}

	virtual void Set(const TCHAR* InValue, EConsoleVariableFlags SetBy) override
	{
		LexFromString(Value.ShadowedValue[0], InValue);
	}

private:
	TConsoleVariableData<T> Value;
	FString Help;
	EConsoleVariableFlags Flags = EConsoleVariableFlags::ECVF_Default;

	template<class Y>
	typename TEnableIf<!TIsSame<T, Y>::Value, Y>::Type GetImpl() const
	{
		check(false);
		return Y();
	}

	template<class Y>
	typename TEnableIf<TIsSame<T, Y>::Value, Y>::Type GetImpl() const
	{
		return GetValueOnAnyThread();
	}

	template<class Y>
	typename TEnableIf<!TIsSame<T, Y>::Value, TConsoleVariableData<Y>*>::Type AsImpl()
	{
		check(false);
		return nullptr;
	}

	template<class Y>
	typename TEnableIf<TIsSame<T, Y>::Value, TConsoleVariableData<T>*>::Type AsImpl()
	{
		return &Value;
	}
};
#endif // NO_CVARS

#if !NO_CVARS
/**
 * Autoregistering console command
 */
class CORE_API FAutoConsoleCommand : private FAutoConsoleObject
{
public:
	/**
	 * Register a console command that takes no arguments
	 *
	 * @param	Name		The name of this command (must not be nullptr)
	 * @param	Help		Help text for this command
	 * @param	Command		The user function to call when this command is executed
	 * @param	Flags		Optional flags bitmask
	 */
	FAutoConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandDelegate& Command, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleCommand(Name, Help, Command, Flags))
	{
	}

	/**
	 * Register a console command that takes arguments
	 *
	 * @param	Name		The name of this command (must not be nullptr)
	 * @param	Help		Help text for this command
	 * @param	Command		The user function to call when this command is executed
	 * @param	Flags		Optional flags bitmask
	 */
	FAutoConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithArgsDelegate& Command, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleCommand(Name, Help, Command, Flags))
	{
	}

	/**
	* Register a console command that takes arguments, a world argument and an output device
	*
	* @param	Name		The name of this command (must not be nullptr)
	* @param	Help		Help text for this command
	* @param	Command		The user function to call when this command is executed
	* @param	Flags		Optional flags bitmask
	*/
	FAutoConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldArgsAndOutputDeviceDelegate& Command, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleCommand(Name, Help, Command, Flags))
	{
	}
};
#else
class CORE_API FAutoConsoleCommand
{
public:
	FAutoConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandDelegate& Command, uint32 Flags = ECVF_Default)
	{
	}

	FAutoConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithArgsDelegate& Command, uint32 Flags = ECVF_Default)
	{
	}

	FAutoConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldArgsAndOutputDeviceDelegate& Command, uint32 Flags = ECVF_Default)
	{
	}
};
#endif


#if !NO_CVARS
/**
 * Autoregistering console command with a world
 */
class CORE_API FAutoConsoleCommandWithWorld : private FAutoConsoleObject
{
public:
	/**
	 * Register a console command that takes a world argument
	 *
	 * @param	Name		The name of this command (must not be nullptr)
	 * @param	Help		Help text for this command
	 * @param	Command		The user function to call when this command is executed
	 * @param	Flags		Optional flags bitmask
	 */
	FAutoConsoleCommandWithWorld(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldDelegate& Command, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleCommand(Name, Help, Command, Flags))
	{
	}

	
};

/**
 * Autoregistering console command with a world and arguments
 */
class CORE_API FAutoConsoleCommandWithWorldAndArgs : private FAutoConsoleObject
{
public:	
	/**
	 * Register a console command that takes arguments and a world argument
	 *
	 * @param	Name		The name of this command (must not be nullptr)
	 * @param	Help		Help text for this command
	 * @param	Command		The user function to call when this command is executed
	 * @param	Flags		Optional flags bitmask
	 */
	FAutoConsoleCommandWithWorldAndArgs(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldAndArgsDelegate& Command, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleCommand(Name, Help, Command, Flags))
	{
	}
};

/**
 * Autoregistering console command with an output device
 */
class CORE_API FAutoConsoleCommandWithOutputDevice : private FAutoConsoleObject
{
public:
	/**
	 * Register a console command that takes an output device
	 *
	 * @param	Name		The name of this command (must not be nullptr)
	 * @param	Help		Help text for this command
	 * @param	Command		The user function to call when this command is executed
	 * @param	Flags		Optional flags bitmask
	 */
	FAutoConsoleCommandWithOutputDevice(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithOutputDeviceDelegate& Command, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleCommand(Name, Help, Command, Flags))
	{
	}
};

/**
 * Autoregistering console command with world, args, an output device
 */
class CORE_API FAutoConsoleCommandWithWorldArgsAndOutputDevice : private FAutoConsoleObject
{
public:
	/**
	 * Register a console command that takes an output device
	 *
	 * @param	Name		The name of this command (must not be nullptr)
	 * @param	Help		Help text for this command
	 * @param	Command		The user function to call when this command is executed
	 * @param	Flags		Optional flags bitmask
	 */
	FAutoConsoleCommandWithWorldArgsAndOutputDevice(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldArgsAndOutputDeviceDelegate& Command, uint32 Flags = ECVF_Default)
		: FAutoConsoleObject(IConsoleManager::Get().RegisterConsoleCommand(Name, Help, Command, Flags))
	{
	}
};

#else

class FAutoConsoleCommandWithWorld
{
public:
	template<class... Args> FAutoConsoleCommandWithWorld(const Args&...) {}
};

class FAutoConsoleCommandWithWorldAndArgs
{
public:
	template<class... Args> FAutoConsoleCommandWithWorldAndArgs(const Args&...) {}
};

class FAutoConsoleCommandWithOutputDevice
{
public:
	template<class... Args> FAutoConsoleCommandWithOutputDevice(const Args&...) {}
};

class FAutoConsoleCommandWithWorldArgsAndOutputDevice
{
public:
	template<class... Args> FAutoConsoleCommandWithWorldArgsAndOutputDevice(const Args&...) {}
};

#endif

CORE_API DECLARE_LOG_CATEGORY_EXTERN(LogConsoleResponse, Log, All);

