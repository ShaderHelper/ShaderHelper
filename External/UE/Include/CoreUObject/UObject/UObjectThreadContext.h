// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnAsyncLoading.cpp: Unreal async loading code.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "HAL/ThreadSingleton.h"
#include "HAL/ThreadSafeCounter.h"
#include "Templates/RefCounting.h"

class FObjectInitializer;
struct FUObjectSerializeContext;
class FLinkerLoad;
class IAsyncPackageLoader;

COREUOBJECT_API DECLARE_LOG_CATEGORY_EXTERN(LogUObjectThreadContext, Log, All);

class COREUOBJECT_API FUObjectThreadContext : public TThreadSingleton<FUObjectThreadContext>
{
	friend TThreadSingleton<FUObjectThreadContext>;

	FUObjectThreadContext();
	virtual ~FUObjectThreadContext();

	/** Stack of currently used FObjectInitializers for this thread */
	TArray<FObjectInitializer*> InitializerStack;

public:
	
	/**
	* Remove top element from the stack.
	*/
	void PopInitializer()
	{
		InitializerStack.Pop(/*bAllowShrinking=*/ false);
	}

	/**
	* Push new FObjectInitializer on stack.
	* @param	Initializer			Object initializer to push.
	*/
	void PushInitializer(FObjectInitializer* Initializer)
	{
		InitializerStack.Push(Initializer);
	}

	/**
	* Retrieve current FObjectInitializer for current thread.
	* @return Current FObjectInitializer.
	*/
	FObjectInitializer* TopInitializer()
	{
		return InitializerStack.Num() ? InitializerStack.Last() : nullptr;
	}

	/**
	* Retrieves current FObjectInitializer for current thread. Will assert of no ObjectInitializer is currently set.
	* @return Current FObjectInitializer reference.
	*/
	FObjectInitializer& TopInitializerChecked()
	{
		FObjectInitializer* ObjectInitializerPtr = TopInitializer();
		UE_CLOG(!ObjectInitializerPtr, LogUObjectThreadContext, Fatal, TEXT("Tried to get the current ObjectInitializer, but none is set. Please use NewObject to construct new UObject-derived classes."));
		return *ObjectInitializerPtr;
	}

	/** true when we are routing ConditionalPostLoad/PostLoad to objects										*/
	bool IsRoutingPostLoad;
	/** The object we are routing PostLoad from the Async Loading code for */
	UObject* CurrentlyPostLoadedObjectByALT;
	/** true when FLinkerManager deletes linkers */
	bool IsDeletingLinkers;
	/* Global flag so that FObjectFinders know if they are called from inside the UObject constructors or not. */
	int32 IsInConstructor;
	/* Object that is currently being constructed with ObjectInitializer */
	UObject* ConstructedObject;
	/** Async Package currently processing objects */
	void* AsyncPackage;
#if WITH_IOSTORE_IN_EDITOR
	/** Async package loader currently processing objects */
	IAsyncPackageLoader* AsyncPackageLoader;
#endif

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	/** Stack to ensure that PostInitProperties is routed through Super:: calls. **/
	TArray<UObject*, TInlineAllocator<16>> PostInitPropertiesCheck;
	/** Used to verify that the Super::PostLoad chain is intact.			*/
	TArray<UObject*, TInlineAllocator<16> > DebugPostLoad;
#endif
#if WITH_EDITORONLY_DATA
	/** Maps a package name to all packages marked as editor-only due to the fact it was marked as editor-only */
	TMap<FName, TSet<FName>> PackagesMarkedEditorOnlyByOtherPackage;
#endif

	/** Gets the current serialization context */
	FUObjectSerializeContext* GetSerializeContext()
	{
		return SerializeContext;
	}

private:
	/** Current serialization context */
	TRefCountPtr<FUObjectSerializeContext> SerializeContext;
};

/** Structure that holds the current serialization state of UObjects */
struct COREUOBJECT_API FUObjectSerializeContext
{
	friend class FUObjectThreadContext;

private:

	/** Constructor */
	FUObjectSerializeContext();

	/** Destructor */
	~FUObjectSerializeContext();

	/** Reference count of this context */
	int32 RefCount;

	/** Imports for EndLoad optimization.	*/
	int32 ImportCount;
	/** Forced exports for EndLoad optimization. */
	int32 ForcedExportCount;
	/** Count for BeginLoad multiple loads.	*/
	int32 ObjBeginLoadCount;
	/** Objects that might need preloading. */
	TArray<UObject*> ObjectsLoaded;
	/** List of linkers that we want to close the loaders for (to free file handles) - needs to be delayed until EndLoad is called with GObjBeginLoadCount of 0 */
	TArray<FLinkerLoad*> DelayedLinkerClosePackages;
	/** List of linkers associated with this context */
	TSet<FLinkerLoad*> AttachedLinkers;

public:

	/** Points to the main UObject currently being serialized */
	UObject* SerializedObject;
	/** Points to the main PackageLinker currently being serialized (Defined in Linker.cpp) */
	FLinkerLoad* SerializedPackageLinker;
	/** The main Import Index currently being used for serialization by CreateImports() (Defined in Linker.cpp) */
	int32 SerializedImportIndex;
	/** Points to the main Linker currently being used for serialization by CreateImports() (Defined in Linker.cpp) */
	FLinkerLoad* SerializedImportLinker;
	/** The most recently used export Index for serialization by CreateExport() */
	int32 SerializedExportIndex;
	/** Points to the most recently used Linker for serialization by CreateExport() */
	FLinkerLoad* SerializedExportLinker;

	/** Adds a new loaded object */
	void AddLoadedObject(UObject* InObject);
	void AddUniqueLoadedObjects(const TArray<UObject*>& InObjects);

	/** Checks if object loading has started */
	bool HasStartedLoading() const
	{
		return ObjBeginLoadCount > 0;
	}
	int32 GetBeginLoadCount() const
	{
		return ObjBeginLoadCount;
	}

	int32 IncrementBeginLoadCount();
	int32 DecrementBeginLoadCount();

	int32 IncrementImportCount()
	{
		return ++ImportCount;
	}
	void ResetImportCount()
	{
		ImportCount = 0;
	}

	int32 IncrementForcedExportCount()
	{
		return ++ForcedExportCount;
	}
	void ResetForcedExports()
	{
		ForcedExportCount = 0;
	}

	bool HasPendingImportsOrForcedExports() const
	{
		return ImportCount || ForcedExportCount;
	}

	bool HasLoadedObjects() const
	{
		return !!ObjectsLoaded.Num();
	}

	bool PRIVATE_PatchNewObjectIntoExport(UObject* OldObject, UObject* NewObject);

	/** This is only meant to be used by FAsyncPackage for performance reasons. The ObjectsLoaded array should not be manipulated directly! */
	TArray<UObject*>& PRIVATE_GetObjectsLoadedInternalUseOnly()
	{
		return ObjectsLoaded;
	}

	void AppendLoadedObjectsAndEmpty(TArray<UObject*>& InLoadedObject)
	{
		InLoadedObject.Append(ObjectsLoaded);
		ObjectsLoaded.Reset();
	}

	void ReserveObjectsLoaded(int32 InReserveSize)
	{
		ObjectsLoaded.Reserve(InReserveSize);
	}

	int32 GetNumObjectsLoaded() const
	{
		return ObjectsLoaded.Num();
	}

	void AddDelayedLinkerClosePackage(class FLinkerLoad* InLinker)
	{
		DelayedLinkerClosePackages.AddUnique(InLinker);
	}

	void RemoveDelayedLinkerClosePackage(class FLinkerLoad* InLinker)
	{
		DelayedLinkerClosePackages.Remove(InLinker);
	}

	void MoveDelayedLinkerClosePackages(TArray<class FLinkerLoad*>& OutDelayedLinkerClosePackages)
	{
		OutDelayedLinkerClosePackages = MoveTemp(DelayedLinkerClosePackages);
	}

	/** Attaches a linker to this context */
	void AttachLinker(FLinkerLoad* InLinker);
	
	/** Detaches a linker from this context */
	void DetachLinker(FLinkerLoad* InLinker);

	/** Detaches all linkers from this context */
	void DetachFromLinkers();

	//~ TRefCountPtr interface
	int32 AddRef()
	{
		return ++RefCount;
	}
	int32 Release()
	{
		int32 CurrentRefCount = --RefCount;
		check(CurrentRefCount >= 0);
		if (CurrentRefCount == 0)
		{
			delete this;
		}
		return CurrentRefCount;
	}
	int32 GetRefCount() const
	{
		return RefCount;
	}
	//~ TRefCountPtr interface
};
