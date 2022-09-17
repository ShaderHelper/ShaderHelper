// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Containers/Map.h"
#include "HAL/IConsoleManager.h"

class CORE_API FConsoleManager :public IConsoleManager
{
public:
	/** constructor */
	FConsoleManager()
		: bHistoryWasLoaded(false)
		, ThreadPropagationCallback(0)
		, bCallAllConsoleVariableSinks(true)
	{
	}

	/** destructor */
	~FConsoleManager()
	{
		for(TMap<FString, IConsoleObject*>::TConstIterator PairIt(ConsoleObjects); PairIt; ++PairIt)
		{
			IConsoleObject* Var = PairIt.Value();

			delete Var;
		}
	}
	
	// internally needed or ECVF_RenderThreadSafe
	IConsoleThreadPropagation* GetThreadPropagationCallback();
	// internally needed or ECVF_RenderThreadSafe
	bool IsThreadPropagationThread();

	/** @param InVar must not be 0 */
	virtual FString FindConsoleObjectName(const IConsoleObject* Obj) const override;

	/** Can be moved out into some automated testing system */
	void Test();

	void OnCVarChanged();

	// interface IConsoleManager -----------------------------------

	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, bool DefaultValue, const TCHAR* Help, uint32 Flags) override;
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, int32 DefaultValue, const TCHAR* Help, uint32 Flags) override;
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, float DefaultValue, const TCHAR* Help, uint32 Flags) override;
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, const TCHAR* DefaultValue, const TCHAR* Help, uint32 Flags) override;
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, const FString& DefaultValue, const TCHAR* Help, uint32 Flags) override;

	virtual IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* Name, bool& RefValue, const TCHAR* Help, uint32 Flags) override;
	virtual IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* Name, int32& RefValue, const TCHAR* Help, uint32 Flags) override;
	virtual IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* Name, float& RefValue, const TCHAR* Help, uint32 Flags) override;
	virtual IConsoleVariable* RegisterConsoleVariableRef(const TCHAR* Name, FString& RefValue, const TCHAR* Help, uint32 Flags) override;
	virtual IConsoleVariable* RegisterConsoleVariableBitRef(const TCHAR* CVarName, const TCHAR* FlagName, uint32 BitNumber, uint8* Force0MaskPtr, uint8* Force1MaskPtr, const TCHAR* Help, uint32 Flags) override;
	
	virtual void CallAllConsoleVariableSinks() override;

	virtual FConsoleVariableSinkHandle RegisterConsoleVariableSink_Handle(const FConsoleCommandDelegate& Command) override;
	virtual void UnregisterConsoleVariableSink_Handle(FConsoleVariableSinkHandle Handle) override;

	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandDelegate& Command, uint32 Flags) override;
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithArgsDelegate& Command, uint32 Flags) override;
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldDelegate& Command, uint32 Flags) override;
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldAndArgsDelegate& Command, uint32 Flags) override;
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldArgsAndOutputDeviceDelegate& Command, uint32 Flags) override;
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithOutputDeviceDelegate& Command, uint32 Flags) override;
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, uint32 Flags) override;
	virtual IConsoleObject* FindConsoleObject(const TCHAR* Name, bool bTrackFrequentCalls = true) const override;
	virtual IConsoleVariable* FindConsoleVariable(const TCHAR* Name, bool bTrackFrequentCalls = true) const override;
	virtual void ForEachConsoleObjectThatStartsWith(const FConsoleObjectVisitor& Visitor, const TCHAR* ThatStartsWith) const override;
	virtual void ForEachConsoleObjectThatContains(const FConsoleObjectVisitor& Visitor, const TCHAR* ThatContains) const override;
	virtual bool ProcessUserConsoleInput(const TCHAR* InInput, FOutputDevice& Ar, UWorld* InWorld) override;
	virtual void AddConsoleHistoryEntry(const TCHAR* Key, const TCHAR* Input) override;
	virtual void GetConsoleHistory(const TCHAR* Key, TArray<FString>& Out) override;
	virtual bool IsNameRegistered(const TCHAR* Name) const override;	
	virtual void RegisterThreadPropagation(uint32 ThreadId, IConsoleThreadPropagation* InCallback) override;
	virtual void UnregisterConsoleObject( IConsoleObject* Object, bool bKeepState) override;

private: // ----------------------------------------------------

	/** Map of console variables and commands, indexed by the name of that command or variable */
	// [name] = pointer (pointer must not be 0)
	TMap<FString, IConsoleObject*> ConsoleObjects;

	bool bHistoryWasLoaded;
	TMap<FString, TArray<FString>>	HistoryEntriesMap;
	TArray<FConsoleCommandDelegate>	ConsoleVariableChangeSinks;

	IConsoleThreadPropagation* ThreadPropagationCallback;

	// if true the next call to CallAllConsoleVariableSinks() we will call all registered sinks
	bool bCallAllConsoleVariableSinks;

	/** 
		* Used to prevent concurrent access to ConsoleObjects.
		* We don't aim to solve all concurrency problems (for example registering and unregistering a cvar on different threads, or reading a cvar from one thread while writing it from a different thread).
		* Rather we just ensure that operations on a cvar from one thread will not conflict with operations on another cvar from another thread.
	**/
	mutable FCriticalSection ConsoleObjectsSynchronizationObject;

	/** 
	 * @param Name must not be 0, must not be empty
	 * @param Obj must not be 0
	 * @return 0 if the name was already in use
	 */
	IConsoleObject* AddConsoleObject(const TCHAR* Name, IConsoleObject* Obj);

	/**
	 * @param Stream must not be 0
	 * @param Pattern must not be 0
	 */
	static bool MatchPartialName(const TCHAR* Stream, const TCHAR* Pattern);

	/** Returns true if Pattern is found in Stream, case insensitive. */
	static bool MatchSubstring(const TCHAR* Stream, const TCHAR* Pattern);

	/**
	 * Get string till whitespace, jump over whitespace
	 * inefficient but this code is not performance critical
	 */
	static FString GetTextSection(const TCHAR* &It);

	/** same as FindConsoleObject() but ECVF_CreatedFromIni are not filtered out (for internal use) */
	IConsoleObject* FindConsoleObjectUnfiltered(const TCHAR* Name) const;

	/**
	 * Unregisters a console variable or command, if that object was registered.  For console variables, this will
	 * actually only "deactivate" the variable so if it becomes registered again the state may persist
	 * (unless bKeepState is false).
	 *
	 * @param	Name	Name of the console object to remove (not case sensitive)
	 * @param	bKeepState	if the current state is kept in memory until a cvar with the same name is registered
	 */
	void UnregisterConsoleObject(const TCHAR* Name, bool bKeepState);

	// reads HistoryEntriesMap from the .ini file (if not already loaded)
	void LoadHistoryIfNeeded();

	// writes HistoryEntriesMap to the .ini file
	void SaveHistory();
};
