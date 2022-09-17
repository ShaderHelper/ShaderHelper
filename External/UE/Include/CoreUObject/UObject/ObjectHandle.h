// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/ScriptArray.h"
#include "HAL/CriticalSection.h"
#include "HAL/Platform.h"
#include "Logging/LogMacros.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectPathId.h"

#define UE_WITH_OBJECT_HANDLE_LATE_RESOLVE WITH_EDITORONLY_DATA
#define UE_WITH_OBJECT_HANDLE_TRACKING WITH_EDITORONLY_DATA

DECLARE_LOG_CATEGORY_EXTERN(LogObjectHandle, Log, All);

/**
 * FObjectRef represents a heavyweight reference that contains the specific pieces of information needed to reference an object
 * (or null) that may or may not be loaded yet.  The expectation is that given the imports of a package we have loaded, we should
 * be able to create an FObjectRef to objects in a package we haven't yet loaded.  For this reason, FObjectRef has to be derivable
 * from the serialized (not transient) contents of an FObjectImport.
 */
struct FObjectRef
{
	FName PackageName;
	FName ClassPackageName;
	FName ClassName;
	FObjectPathId ObjectPath;

	bool operator==(const FObjectRef&& Other) const
	{
		return (PackageName == Other.PackageName) && (ObjectPath == Other.ObjectPath) && (ClassPackageName == Other.ClassPackageName) && (ClassName == Other.ClassName);
	}
};

inline bool IsObjectRefNull(const FObjectRef& ObjectRef) { return ObjectRef.PackageName.IsNone() && ObjectRef.ObjectPath.IsNone(); }

COREUOBJECT_API FObjectRef MakeObjectRef(const UObject* Object);
COREUOBJECT_API FObjectRef MakeObjectRef(struct FPackedObjectRef ObjectRef);
COREUOBJECT_API UObject* ResolveObjectRef(const FObjectRef& ObjectRef, uint32 LoadFlags = LOAD_None);
COREUOBJECT_API UClass* ResolveObjectRefClass(const FObjectRef& ObjectRef, uint32 LoadFlags = LOAD_None);


/**
 * FPackedObjectRef represents a lightweight reference that can fit in the space of a pointer and be able to refer to an object
 * (or null) that may or may not be loaded without pointing to its location in memory (even if it is currently loaded).
 */
struct FPackedObjectRef
{
	// Must be 0 for a reference to null.
	// The least significant bit must always be 1 in a non-null reference.
	UPTRINT EncodedRef;
};

inline bool IsPackedObjectRefNull(FPackedObjectRef ObjectRef) { return !ObjectRef.EncodedRef; }

COREUOBJECT_API FPackedObjectRef MakePackedObjectRef(const UObject* Object);
COREUOBJECT_API FPackedObjectRef MakePackedObjectRef(const FObjectRef& ObjectRef);
COREUOBJECT_API UObject* ResolvePackedObjectRef(FPackedObjectRef ObjectRef, uint32 LoadFlags = LOAD_None);
COREUOBJECT_API UClass* ResolvePackedObjectRefClass(FPackedObjectRef ObjectRef, uint32 LoadFlags = LOAD_None);

inline bool operator==(FPackedObjectRef LHS, FPackedObjectRef RHS) { return LHS.EncodedRef == RHS.EncodedRef; }
inline bool operator!=(FPackedObjectRef LHS, FPackedObjectRef RHS) { return LHS.EncodedRef != RHS.EncodedRef; }
inline uint32 GetTypeHash(FPackedObjectRef ObjectRef) { return GetTypeHash(ObjectRef.EncodedRef); }

/**
 * FObjectHandle is either a packed object ref or the resolved pointer to an object.  Depending on configuration
 * when you create a handle, it may immediately be resolved to a pointer.
 */
#if UE_WITH_OBJECT_HANDLE_LATE_RESOLVE

struct FObjectHandleInternal
{
	UPTRINT PointerOrRef;
};
using FObjectHandle = FObjectHandleInternal;

inline bool operator==(FObjectHandle LHS, FObjectHandle RHS);
inline bool operator!=(FObjectHandle LHS, FObjectHandle RHS);
inline uint32 GetTypeHash(FObjectHandle Handle);

#else

using FObjectHandle = UObject*;
//NOTE: operator==, operator!=, GetTypeHash fall back to the default on UObject* or void* through coercion.

#endif

inline FObjectHandle MakeObjectHandle(FPackedObjectRef ObjectRef);
inline FObjectHandle MakeObjectHandle(const FObjectRef& ObjectRef);
inline FObjectHandle MakeObjectHandle(UObject* Object);

inline bool IsObjectHandleNull(FObjectHandle Handle);
inline bool IsObjectHandleResolved(FObjectHandle Handle);

/** Read the handle as a pointer without checking if it is resolved. Invalid to call for unresolved handles. */
inline UObject* ReadObjectHandlePointerNoCheck(FObjectHandle Handle);
/** Read the handle as a packed object ref without checking if it is unresolved. Invalid to call for resolved handles. */
inline FPackedObjectRef ReadObjectHandlePackedObjectRefNoCheck(FObjectHandle Handle);

inline UObject* ResolveObjectHandle(FObjectHandle& Handle);
inline UObject* ResolveObjectHandleNoRead(FObjectHandle& Handle);
inline UClass* ResolveObjectHandleClass(FObjectHandle Handle);

/** Read the handle as a pointer if resolved, and otherwise return null. */
inline UObject* ReadObjectHandlePointer(FObjectHandle Handle);
/** Read the handle as a packed object ref if unresolved, and otherwise return the null packed object ref. */
inline FPackedObjectRef ReadObjectHandlePackedObjectRef(FObjectHandle Handle);

#if UE_WITH_OBJECT_HANDLE_LATE_RESOLVE
inline FObjectRef MakeObjectRef(FObjectHandle Handle);
inline FPackedObjectRef MakePackedObjectRef(FObjectHandle Handle);
#endif


/**
 * FObjectHandles can optionally support tracking.  Because of the low level nature of object handles, anything that
 * registers itself for these callbacks should ensure that it is:
 * 1) error free (ie: should not cause exceptions even in unusual circumstances)
 * 2) fault tolerant (ie: could be called at a time when an exception has happened)
 * 3) thread-safe (ie: could be called from any thread)
 * 4) high performance (ie: will be called many times)
 */
#if UE_WITH_OBJECT_HANDLE_TRACKING
 /**
  * Callback notifying when an object value is read from a handle.  Fired regardless of whether the handle
  * was resolved as part of the read operation or not and whether the object being read is null or not.
  *
  * @param ReadObject	The object that was read from a handle.
  */
using ObjectHandleReadFunction = void(UObject* ReadObject);

/**
 * Callback notifying when a class is resolved from an object handle or object reference.
 * Classes are resolved either independently for a given handle/reference or as part of each object resolve.
 *
 * @param SourceRef		The object reference (either user provided or internally computed from a handle) from which the class was resolved.
 * @param ClassPackage	The package containing the resolved class.
 * @param Class			The resolved class.
 */
using ObjectHandleClassResolvedFunction = void(const FObjectRef& SourceRef, UPackage* ClassPackage, UClass* Class);

/**
 * Callback notifying when an object is resolved from an object handle or object reference.
 *
 * @param SourceRef		The object reference (either user provided or internally computed from a handle) from which the object was resolved.
 * @param ObjectPackage	The package containing the resolved object.
 * @param Object		The resolved object.
 */
using ObjectHandleReferenceResolvedFunction = void(const FObjectRef& SourceRef, UPackage* ObjectPackage, UObject* Object);

/**
 * Installs a new callback for notifications that an object value has been read from a handle.
 *
 * @param Function		The new handle read callback to install.
 * @return				The previous handle read callback (or nullptr).  The caller is expected to store this and call in their own handle read callback.
 */
COREUOBJECT_API /*UE_NODISCARD*/ ObjectHandleReadFunction* SetObjectHandleReadCallback(ObjectHandleReadFunction* Function);

/**
 * Installs a new callback for notifications that a class has been resolved from an object handle or object reference.
 *
 * @param Function		The new class resolved callback to install.
 * @return				The previous object resolved callback (or nullptr).  The caller is expected to store this and call in their own class resolved callback.
 */
COREUOBJECT_API /*UE_NODISCARD*/ ObjectHandleClassResolvedFunction* SetObjectHandleClassResolvedCallback(ObjectHandleClassResolvedFunction* Function);

/**
 * Installs a new callback for notifications that an object has been resolved from an object handle or object reference.
 *
 * @param Function		The new object resolved callback to install.
 * @return				The previous object resolved callback (or nullptr).  The caller is expected to store this and call in their own object resolved callback.
 */
COREUOBJECT_API /*UE_NODISCARD*/ ObjectHandleReferenceResolvedFunction* SetObjectHandleReferenceResolvedCallback(ObjectHandleReferenceResolvedFunction* Function);
#endif



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct FObjectHandlePackageDebugData
{
	FMinimalName PackageName;
	FScriptArray ObjectPaths;
	FScriptArray DataClassDescriptors;
	uint8 _Padding[sizeof(FRWLock) + sizeof(FScriptMap)];
};

struct FObjectHandleDataClassDescriptor
{
	FMinimalName PackageName;
	FMinimalName ClassName;
};

namespace ObjectHandle_Private
{
	constexpr uint32 ObjectPathIdShift = 1;
	constexpr uint32 ObjectPathIdMask = 0x00FF'FFFF;

	constexpr uint32 DataClassDescriptorIdShift = 25;
	constexpr uint32 DataClassDescriptorIdMask = 0x0000'00FF;

	constexpr uint32 PackageIdShift = 33;
	constexpr uint32 PackageIdMask = 0x7FFF'FFFF;

#if UE_WITH_OBJECT_HANDLE_TRACKING
	extern COREUOBJECT_API ObjectHandleReadFunction* ObjectHandleReadCallback;
	extern COREUOBJECT_API ObjectHandleClassResolvedFunction* ObjectHandleClassResolvedCallback;
	extern COREUOBJECT_API ObjectHandleReferenceResolvedFunction* ObjectHandleReferenceResolvedCallback;

	inline void OnHandleRead(UObject* Object)
	{
		if (ObjectHandleReadFunction* Callback = ObjectHandleReadCallback)
		{
			Callback(Object);
		}
	}
	
	inline void OnClassReferenceResolved(const FObjectRef& ObjectRef, UPackage* Package, UClass* Class)
	{
		if (ObjectHandleClassResolvedFunction* Callback = ObjectHandleClassResolvedCallback)
		{
			Callback(ObjectRef, Package, Class);
		}
	}

	inline void OnReferenceResolved(const FObjectRef& ObjectRef, UPackage* Package, UObject* Object)
	{
		if (ObjectHandleReferenceResolvedFunction* Callback = ObjectHandleReferenceResolvedCallback)
		{
			Callback(ObjectRef, Package, Object);
		}
	}
#else
	inline void OnHandleRead(UObject* Object) {}
	inline void OnClassReferenceResolved(const FObjectRef& ObjectRef, UPackage* Package, UClass* Class) {}
	inline void OnReferenceResolved(const FObjectRef& ObjectRef, UPackage* Package, UObject* Object) {}
#endif
}

#if UE_WITH_OBJECT_HANDLE_LATE_RESOLVE

inline bool IsObjectHandleNull(FObjectHandle Handle) { return !Handle.PointerOrRef; }
inline bool IsObjectHandleResolved(FObjectHandle Handle) { return !(Handle.PointerOrRef & 1); }

inline UObject* ReadObjectHandlePointerNoCheck(FObjectHandle Handle)
{
	return reinterpret_cast<UObject*>(Handle.PointerOrRef);
}

inline FPackedObjectRef ReadObjectHandlePackedObjectRefNoCheck(FObjectHandle Handle)
{
	return {Handle.PointerOrRef};
}

inline FObjectHandle MakeObjectHandle(FPackedObjectRef ObjectRef) { return {ObjectRef.EncodedRef}; }
inline FObjectHandle MakeObjectHandle(const FObjectRef& ObjectRef) { return MakeObjectHandle(MakePackedObjectRef(ObjectRef)); }
inline FObjectHandle MakeObjectHandle(UObject* Object) { return {UPTRINT(Object)}; }

inline bool operator==(FObjectHandle LHS, FObjectHandle RHS)
{
	if (IsObjectHandleResolved(LHS) == IsObjectHandleResolved(RHS))
	{
		return LHS.PointerOrRef == RHS.PointerOrRef;
	}
	else
	{
		return MakePackedObjectRef(LHS) == MakePackedObjectRef(RHS);
	}
}
inline bool operator!=(FObjectHandle LHS, FObjectHandle RHS)
{
	return !(LHS == RHS);
}

inline uint32 GetTypeHash(FObjectHandle Handle)
{
	checkf(IsObjectHandleResolved(Handle), TEXT("Cannot hash an unresolved handle."));
	return GetTypeHash(ReadObjectHandlePointerNoCheck(Handle));
}

#else

inline bool IsObjectHandleNull(FObjectHandle Handle) { return !Handle; }
inline bool IsObjectHandleResolved(FObjectHandle Handle) { return true; }

inline UObject* ReadObjectHandlePointerNoCheck(FObjectHandle Handle) { return Handle; }
inline FPackedObjectRef ReadObjectHandlePackedObjectRefNoCheck(FObjectHandle Handle) { return FPackedObjectRef(); }

inline FObjectHandle MakeObjectHandle(FPackedObjectRef ObjectRef) { return ResolvePackedObjectRef(ObjectRef); }
inline FObjectHandle MakeObjectHandle(const FObjectRef& ObjectRef) { return ResolveObjectRef(ObjectRef); }
inline FObjectHandle MakeObjectHandle(UObject* Object) { return Object; }

#endif

inline UObject* ResolveObjectHandleNoRead(FObjectHandle& Handle)
{
#if UE_WITH_OBJECT_HANDLE_LATE_RESOLVE
	FObjectHandle LocalHandle = Handle;
	if (IsObjectHandleResolved(LocalHandle))
	{
		UObject* ResolvedObject = ReadObjectHandlePointerNoCheck(LocalHandle);
		return ResolvedObject;
	}
	else
	{
		LocalHandle = MakeObjectHandle(ResolvePackedObjectRef(ReadObjectHandlePackedObjectRefNoCheck(LocalHandle)));
		UObject* ResolvedObject = ReadObjectHandlePointerNoCheck(LocalHandle);
		Handle = LocalHandle;
		return ResolvedObject;
	}
#else
	return ReadObjectHandlePointerNoCheck(Handle);
#endif
}

inline UObject* ResolveObjectHandle(FObjectHandle& Handle)
{
#if UE_WITH_OBJECT_HANDLE_LATE_RESOLVE || UE_WITH_OBJECT_HANDLE_TRACKING
	UObject* ResolvedObject = ResolveObjectHandleNoRead(Handle);
	ObjectHandle_Private::OnHandleRead(ResolvedObject);
	return ResolvedObject;
#else
	return ReadObjectHandlePointerNoCheck(Handle);
#endif
}

inline UClass* ResolveObjectHandleClass(FObjectHandle Handle)
{
	if (IsObjectHandleResolved(Handle))
	{
		return ReadObjectHandlePointerNoCheck(Handle)->GetClass();
	}
	else
	{
		// @TODO: OBJPTR: This should be cached somewhere instead of resolving on every call
		return ResolvePackedObjectRefClass(ReadObjectHandlePackedObjectRefNoCheck(Handle));
	}
}

inline UObject* ReadObjectHandlePointer(FObjectHandle Handle)
{
	if (IsObjectHandleResolved(Handle))
	{
		UObject* ResolvedObject = ReadObjectHandlePointerNoCheck(Handle);
		ObjectHandle_Private::OnHandleRead(ResolvedObject);
		return ResolvedObject;
	}
	return nullptr;
}

inline FPackedObjectRef ReadObjectHandlePackedObjectRef(FObjectHandle Handle)
{
	return !IsObjectHandleResolved(Handle) ? ReadObjectHandlePackedObjectRefNoCheck(Handle) : FPackedObjectRef();
}

#if UE_WITH_OBJECT_HANDLE_LATE_RESOLVE
inline FObjectRef MakeObjectRef(FObjectHandle Handle)
{
	if (IsObjectHandleResolved(Handle))
	{
		return MakeObjectRef(ReadObjectHandlePointerNoCheck(Handle));
	}
	else
	{
		return MakeObjectRef(ReadObjectHandlePackedObjectRefNoCheck(Handle));
	}
}

inline FPackedObjectRef MakePackedObjectRef(FObjectHandle Handle)
{
	if (IsObjectHandleResolved(Handle))
	{
		return MakePackedObjectRef(ReadObjectHandlePointerNoCheck(Handle));
	}
	else
	{
		return ReadObjectHandlePackedObjectRefNoCheck(Handle);
	}
}
#endif
