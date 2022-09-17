// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/Optional.h"
#include "Misc/PackageName.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectHandle.h"
#include "UObject/Package.h"

class ITargetPlatform;

#define UE_WITH_PACKAGE_ACCESS_TRACKING UE_WITH_OBJECT_HANDLE_TRACKING

#if UE_WITH_PACKAGE_ACCESS_TRACKING
#define UE_TRACK_REFERENCING_PACKAGE_SCOPED(Package, OpName) PackageAccessTracking_Private::FPackageAccessRefScope ANONYMOUS_VARIABLE(PackageAccessTracker_)(Package, OpName);
#define UE_TRACK_REFERENCING_PACKAGE_DELAYED_SCOPED(TrackerName, OpName) TOptional<PackageAccessTracking_Private::FPackageAccessRefScope> TrackerName; FName TrackerName##_OpName(OpName);
#define UE_TRACK_REFERENCING_PACKAGE_DELAYED(TrackerName, Package) if (TrackerName) TrackerName->SetPackageName(Package->GetFName()); else TrackerName.Emplace(Package->GetFName(), TrackerName##_OpName);
#define UE_TRACK_REFERENCING_PLATFORM_SCOPED(TargetPlatform) PackageAccessTracking_Private::FPackageAccessRefScope ANONYMOUS_VARIABLE(PackageAccessTracker_)(TargetPlatform);
#define UE_TRACK_REFERENCING_OPNAME_SCOPED(OpName) PackageAccessTracking_Private::FPackageAccessRefScope ANONYMOUS_VARIABLE(PackageAccessTracker_)(OpName);
#else
#define UE_TRACK_REFERENCING_PACKAGE_SCOPED(Package, OpName)
#define UE_TRACK_REFERENCING_PACKAGE_DELAYED_SCOPED(TrackerName, OpName)
#define UE_TRACK_REFERENCING_PACKAGE_DELAYED(TrackerName, Package)
#define UE_TRACK_REFERENCING_PLATFORM_SCOPED(TargetPlatform)
#define UE_TRACK_REFERENCING_OPNAME_SCOPED(OpName)
#endif //UE_WITH_PACKAGE_ACCESS_TRACKING

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if UE_WITH_PACKAGE_ACCESS_TRACKING
namespace PackageAccessTracking_Private
{
	struct FTrackedData
	{
		/** Standard constructor; sets variables from DirectData passed to a FPackageAccessRefScope. */
		COREUOBJECT_API FTrackedData(FName PackgeName, FName OpName, const ITargetPlatform* InTargetPlatform);
		/** Accumulating constructor; sets new AccumulatedData by combining DirectData with Outer's AccumulatedData. */
		COREUOBJECT_API FTrackedData(FTrackedData& DirectData, FTrackedData* OuterAccumulatedData);

		FName PackageName;
		FName OpName;
		FName BuildOpName;
		const ITargetPlatform* TargetPlatform = nullptr;
	};

	class FPackageAccessRefScope
	{
	public:
		COREUOBJECT_API FPackageAccessRefScope(FName InPackageName, FName InOpName, const ITargetPlatform* InTargetPlatform = nullptr);
		COREUOBJECT_API FPackageAccessRefScope(const UPackage* InPackage, FName InOpName);
		COREUOBJECT_API FPackageAccessRefScope(FName InOpName);
		COREUOBJECT_API FPackageAccessRefScope(const ITargetPlatform* InTargetPlatform);

		COREUOBJECT_API ~FPackageAccessRefScope();

		COREUOBJECT_API void SetPackageName(FName InPackageName);

		FORCEINLINE FName GetPackageName() const { return DirectData.PackageName; }
		FORCEINLINE FName GetOpName() const { return DirectData.OpName; }
		FORCEINLINE FPackageAccessRefScope* GetOuter() const { return Outer; }
		FORCEINLINE const ITargetPlatform* GetTargetPlatform() const { return DirectData.TargetPlatform; }

		static COREUOBJECT_API FPackageAccessRefScope* GetCurrentThreadScope();
		static COREUOBJECT_API FTrackedData* GetCurrentThreadAccumulatedData();
	private:
		FTrackedData DirectData;
		FTrackedData AccumulatedData;
		FPackageAccessRefScope* Outer = nullptr;
		static thread_local FPackageAccessRefScope* CurrentThreadScope;
	};
};
#endif // UE_WITH_PACKAGE_ACCESS_TRACKING
