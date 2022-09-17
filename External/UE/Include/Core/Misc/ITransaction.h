// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/Guid.h"
#include "UObject/UObjectHierarchyFwd.h"
#include "Change.h"

// Class for handling undo/redo transactions among objects.
typedef void(*STRUCT_DC)( void* TPtr );						// default construct
typedef void(*STRUCT_AR)( class FArchive& Ar, void* TPtr );	// serialize
typedef void(*STRUCT_DTOR)( void* TPtr );					// destruct


/** Different kinds of actions that can trigger a transaction state change */
enum class ETransactionStateEventType : uint8
{
	/** A transaction has been started. This will be followed by a TransactionCanceled or TransactionFinalized event. */
	TransactionStarted,
	/** A transaction was canceled. */
	TransactionCanceled,
	/** A transaction was finalized. */
	TransactionFinalized,

	/** A transaction will be used used in an undo/redo operation. This will be followed by a UndoRedoFinalized event. */
	UndoRedoStarted,
	/** A transaction has been used in an undo/redo operation. */
	UndoRedoFinalized,
};


/**
 * Convenience struct for passing around transaction context.
 */
struct FTransactionContext 
{
	FTransactionContext()
		: TransactionId()
		, OperationId()
		, Title()
		, Context()
		, PrimaryObject(nullptr)
	{
	}

	FTransactionContext(const FGuid& InTransactionId, const FGuid& InOperationId, const FText& InSessionTitle, const TCHAR* InContext, UObject* InPrimaryObject) 
		: TransactionId(InTransactionId)
		, OperationId(InOperationId)
		, Title(InSessionTitle)
		, Context(InContext)
		, PrimaryObject(InPrimaryObject)
	{
	}

	bool IsValid() const
	{
		return TransactionId.IsValid() && OperationId.IsValid();
	}

	/** Unique identifier for the transaction, used to track it during its lifetime */
	FGuid TransactionId;
	/** Unique identifier for the active operation on the transaction (if any) */
	FGuid OperationId;
	/** Descriptive title of the transaction */
	FText Title;
	/** The context that generated the transaction */
	FString Context;
	/** The primary UObject for the transaction (if any). */
	UObject* PrimaryObject;
};


/**
 * Interface for transaction object annotations.
 *
 * Transaction object annotations are used for attaching additional user defined data to a transaction.
 * This is sometimes useful, because the transaction system only remembers changes that are serializable
 * on the UObject that a modification was performed on, but it does not see other changes that may have
 * to be remembered in order to properly restore the object internals.
 */
class ITransactionObjectAnnotation
{
public:
	virtual ~ITransactionObjectAnnotation() = default;
	virtual void AddReferencedObjects(class FReferenceCollector& Collector) = 0;
	virtual void Serialize(class FArchive& Ar) = 0;
};


/** Delta-change information for an object that was transacted */
struct FTransactionObjectDeltaChange
{
	FTransactionObjectDeltaChange()
		: bHasNameChange(false)
		, bHasOuterChange(false)
		, bHasExternalPackageChange(false)
		, bHasPendingKillChange(false)
		, bHasNonPropertyChanges(false)
	{
	}

	bool HasChanged() const
	{
		return bHasNameChange || bHasOuterChange || bHasExternalPackageChange || bHasPendingKillChange || bHasNonPropertyChanges || ChangedProperties.Num() > 0;
	}

	void Merge(const FTransactionObjectDeltaChange& InOther)
	{
		bHasNameChange |= InOther.bHasNameChange;
		bHasOuterChange |= InOther.bHasOuterChange;
		bHasExternalPackageChange |= InOther.bHasExternalPackageChange;
		bHasPendingKillChange |= InOther.bHasPendingKillChange;
		bHasNonPropertyChanges |= InOther.bHasNonPropertyChanges;

		for (const FName& OtherChangedPropName : InOther.ChangedProperties)
		{
			ChangedProperties.AddUnique(OtherChangedPropName);
		}
	}

	/** True if the object name has changed */
	bool bHasNameChange : 1;
	/** True of the object outer has changed */
	bool bHasOuterChange : 1;
	/** True of the object assigned package has changed */
	bool bHasExternalPackageChange : 1;
	/** True if the object "pending kill" state has changed */
	bool bHasPendingKillChange : 1;
	/** True if the object has changes other than property changes (may be caused by custom serialization) */
	bool bHasNonPropertyChanges : 1;
	/** Array of properties that have changed on the object */
	TArray<FName> ChangedProperties;
};


/** Different kinds of actions that can trigger a transaction object event */
enum class ETransactionObjectEventType : uint8
{
	/** This event was caused by an undo/redo operation */
	UndoRedo,
	/** This event was caused by a transaction being finalized within the transaction system */
	Finalized,
	/** This event was caused by a transaction snapshot. Several of these may be generated in the case of an interactive change */
	Snapshot,
};


/**
 * Transaction object events.
 *
 * Transaction object events are used to notify objects when they are transacted in some way.
 * This mostly just means that an object has had an undo/redo applied to it, however an event is also triggered
 * when the object has been finalized as part of a transaction (allowing you to detect object changes).
 */
class FTransactionObjectEvent
{
public:
	FTransactionObjectEvent() = default;

	FTransactionObjectEvent(const FGuid& InTransactionId, const FGuid& InOperationId, const ETransactionObjectEventType InEventType, const FTransactionObjectDeltaChange& InDeltaChange, const TSharedPtr<ITransactionObjectAnnotation>& InAnnotation
		, const FName InOriginalObjectPackageName, const FName InOriginalObjectName, const FName InOriginalObjectPathName, const FName InOriginalObjectOuterPathName, const FName InOriginalObjectExternalPackageName, const FName InOriginalObjectClassPathName)
		: TransactionId(InTransactionId)
		, OperationId(InOperationId)
		, EventType(InEventType)
		, DeltaChange(InDeltaChange)
		, Annotation(InAnnotation)
		, OriginalObjectPackageName(InOriginalObjectPackageName)
		, OriginalObjectName(InOriginalObjectName)
		, OriginalObjectPathName(InOriginalObjectPathName)
		, OriginalObjectOuterPathName(InOriginalObjectOuterPathName)
		, OriginalObjectExternalPackageName(InOriginalObjectExternalPackageName)
		, OriginalObjectClassPathName(InOriginalObjectClassPathName)
	{
		check(TransactionId.IsValid());
		check(OperationId.IsValid());
	}

	/** The unique identifier of the transaction this event belongs to */
	const FGuid& GetTransactionId() const
	{
		return TransactionId;
	}

	/** The unique identifier for the active operation on the transaction this event belongs to */
	const FGuid& GetOperationId() const
	{
		return OperationId;
	}

	/** What kind of action caused this event? */
	ETransactionObjectEventType GetEventType() const
	{
		return EventType;
	}

	/** Was the pending kill state of this object changed? (implies non-property changes) */
	bool HasPendingKillChange() const
	{
		return DeltaChange.bHasPendingKillChange;
	}

	/** Was the name of this object changed? (implies non-property changes) */
	bool HasNameChange() const
	{
		return DeltaChange.bHasNameChange;
	}

	/** Get the original package name of this object */
	FName GetOriginalObjectPackageName() const
	{
		return OriginalObjectPackageName;
	}

	/** Get the original name of this object */
	FName GetOriginalObjectName() const
	{
		return OriginalObjectName;
	}

	/** Get the original path name of this object */
	FName GetOriginalObjectPathName() const
	{
		return OriginalObjectPathName;
	}

	FName GetOriginalObjectClassPathName() const
	{
		return OriginalObjectClassPathName;
	}

	/** Was the outer of this object changed? (implies non-property changes) */
	bool HasOuterChange() const
	{
		return DeltaChange.bHasOuterChange;
	}

	/** Has the package assigned to this object changed? (implies non-property changes) */
	bool HasExternalPackageChange() const
	{
		return DeltaChange.bHasExternalPackageChange;
	}

	/** Get the original outer path name of this object */
	FName GetOriginalObjectOuterPathName() const
	{
		return OriginalObjectOuterPathName;
	}

	/** Get the original package name of this object */
	FName GetOriginalObjectExternalPackageName() const
	{
		return OriginalObjectExternalPackageName;
	}


	/** Were any non-property changes made to the object? */
	bool HasNonPropertyChanges(const bool InSerializationOnly = false) const
	{
		return (!InSerializationOnly && (DeltaChange.bHasNameChange || DeltaChange.bHasOuterChange || DeltaChange.bHasExternalPackageChange || DeltaChange.bHasPendingKillChange)) || DeltaChange.bHasNonPropertyChanges;
	}

	/** Were any property changes made to the object? */
	bool HasPropertyChanges() const
	{
		return DeltaChange.ChangedProperties.Num() > 0;
	}

	/** Get the list of changed properties. Each entry is actually a chain of property names (root -> leaf) separated by a dot, eg) "ObjProp.StructProp". */
	const TArray<FName>& GetChangedProperties() const
	{
		return DeltaChange.ChangedProperties;
	}

	/** Get the annotation object associated with the object being transacted (if any). */
	TSharedPtr<ITransactionObjectAnnotation> GetAnnotation() const
	{
		return Annotation;
	}

	/** Merge this transaction event with another */
	void Merge(const FTransactionObjectEvent& InOther)
	{
		if (EventType == ETransactionObjectEventType::Snapshot)
		{
			EventType = InOther.EventType;
		}

		DeltaChange.Merge(InOther.DeltaChange);
	}

private:
	FGuid TransactionId;
	FGuid OperationId;
	ETransactionObjectEventType EventType;
	FTransactionObjectDeltaChange DeltaChange;
	TSharedPtr<ITransactionObjectAnnotation> Annotation;
	FName OriginalObjectPackageName;
	FName OriginalObjectName;
	FName OriginalObjectPathName;
	FName OriginalObjectOuterPathName;
	FName OriginalObjectExternalPackageName;
	FName OriginalObjectClassPathName;
};

/**
 * Diff for a given transaction.
 */
struct FTransactionDiff
{
	FGuid TransactionId;
	FString TransactionTitle;
	TMap<FName, TSharedPtr<FTransactionObjectEvent>> DiffMap;
};

/**
 * Interface for transactions.
 *
 * Transactions are created each time an UObject is modified, for example in the Editor.
 * The data stored inside a transaction object can then be used to provide undo/redo functionality.
 */
class ITransaction
{
public:

	/** BeginOperation should be called when a transaction or undo/redo starts */
	virtual void BeginOperation() = 0;

	/** EndOperation should be called when a transaction is finalized or canceled or undo/redo ends */
	virtual void EndOperation() = 0;

	/** Called when this transaction is completed to finalize the transaction */
	virtual void Finalize() = 0;

	/** Applies the transaction. */
	virtual void Apply() = 0;

	/** Gets the full context for the transaction */
	virtual FTransactionContext GetContext() const = 0;

	/**
	 * Report if a transaction should be put in the undo buffer.
	 * A transaction will be transient if it contains PIE objects or result in a no-op.
	 * If this returns true the transaction won't be put in the transaction buffer.
	 * @returns true if the transaction is transient.
	 */
	virtual bool IsTransient() const = 0;

	/** @returns if this transaction tracks PIE objects */
	virtual bool ContainsPieObjects() const = 0;

	/**
	 * Saves an array to the transaction.
	 *
	 * @param Object The object that owns the array.
	 * @param Array The array to save.
	 * @param Index 
	 * @param Count 
	 * @param Oper
	 * @param ElementSize
	 * @param ElementAlignment
	 * @param Serializer
	 * @param Destructor
	 * @see SaveObject
	 */
	virtual void SaveArray( UObject* Object, class FScriptArray* Array, int32 Index, int32 Count, int32 Oper, int32 ElementSize, uint32 ElementAlignment, STRUCT_DC DefaultConstructor, STRUCT_AR Serializer, STRUCT_DTOR Destructor ) = 0;

	/**
	 * Saves an UObject to the transaction.
	 *
	 * @param Object The object to save.
	 *
	 * @see SaveArray
	 */
	virtual void SaveObject( UObject* Object ) = 0;

	/**
	 * Stores a command that can be used to undo a change to the specified object.  This may be called multiple times in the
	 * same transaction to stack up changes that will be rolled back in reverse order.  No copy of the object itself is stored.
	 *
	 * @param Object The object the undo change will apply to
	 * @param CustomChange The change that can be used to undo the changes to this object.
	 */
	virtual void StoreUndo( UObject* Object, TUniquePtr<FChange> CustomChange ) = 0;

	/**
	 * Sets the transaction's primary object.
	 *
	 * @param Object The primary object to set.
	 */
	virtual void SetPrimaryObject( UObject* Object ) = 0;

	/**
	 * Snapshots a UObject within the transaction.
	 *
	 * @param Object	The object to snapshot.
	 * @param Property	The optional list of properties that have potentially changed on the object (to avoid snapshotting the entire object).
	 */
	virtual void SnapshotObject( UObject* Object, TArrayView<const FProperty*> Properties ) = 0;
};
