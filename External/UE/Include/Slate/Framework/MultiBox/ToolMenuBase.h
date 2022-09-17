// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/NamePermissionList.h"

#include "ToolMenuBase.generated.h"

class FMultiBox;

UENUM()
enum class ECustomizedToolMenuVisibility
{
	None,
	Visible,
	Hidden
};

USTRUCT()
struct FCustomizedToolMenuEntry
{
	GENERATED_BODY()

	FCustomizedToolMenuEntry() :
		Visibility(ECustomizedToolMenuVisibility::None)
	{
	}

	UPROPERTY()
	ECustomizedToolMenuVisibility Visibility;
};

USTRUCT()
struct FCustomizedToolMenuSection
{
	GENERATED_BODY()

	FCustomizedToolMenuSection() :
		Visibility(ECustomizedToolMenuVisibility::None)
	{
	}

	UPROPERTY()
	ECustomizedToolMenuVisibility Visibility;
};

USTRUCT()
struct FCustomizedToolMenuNameArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FName> Names;
};

USTRUCT()
struct SLATE_API FCustomizedToolMenu
{
	GENERATED_BODY()

	UPROPERTY()
	FName Name;

	UPROPERTY()
	TMap<FName, FCustomizedToolMenuEntry> Entries;

	UPROPERTY()
	TMap<FName, FCustomizedToolMenuSection> Sections;

	UPROPERTY()
	TMap<FName, FCustomizedToolMenuNameArray> EntryOrder;

	UPROPERTY()
	TArray<FName> SectionOrder;

	FNamePermissionList MenuPermissions;

	FCustomizedToolMenuEntry* FindEntry(const FName InEntryName);
	const FCustomizedToolMenuEntry* FindEntry(const FName InEntryName) const;
	FCustomizedToolMenuEntry* AddEntry(const FName InEntryName);
	ECustomizedToolMenuVisibility GetEntryVisiblity(const FName InSectionName) const;
	bool IsEntryHidden(const FName InEntryName) const;
	FName GetEntrySectionName(const FName InEntryName) const;

	FCustomizedToolMenuSection* FindSection(const FName InSectionName);
	const FCustomizedToolMenuSection* FindSection(const FName InSectionName) const;
	FCustomizedToolMenuSection* AddSection(const FName InSectionName);
	ECustomizedToolMenuVisibility GetSectionVisiblity(const FName InSectionName) const;
	bool IsSectionHidden(const FName InSectionName) const;
};

struct SLATE_API FCustomizedToolMenuHierarchy
{
	FName GetEntrySectionName(const FName InEntryName) const;
	bool IsEntryHidden(const FName InEntryName) const;

	bool IsSectionHidden(const FName InSectionName) const;

	FCustomizedToolMenu GenerateFlattened() const;

	TArray<const FCustomizedToolMenu*> Hierarchy;
	TArray<const FCustomizedToolMenu*> RuntimeHierarchy;
};

UCLASS(Abstract)
class SLATE_API UToolMenuBase : public UObject
{
	GENERATED_BODY()

public:

	virtual bool IsEditing() const { return false; }
	virtual FName GetSectionName(const FName InEntryName) const { return NAME_None; }

	virtual bool ContainsSection(const FName InName) const { return false; }
	virtual bool ContainsEntry(const FName InName) const { return false; }

	virtual FCustomizedToolMenu* FindMenuCustomization() const { return nullptr; }
	virtual FCustomizedToolMenu* AddMenuCustomization() const { return nullptr; }
	virtual FCustomizedToolMenuHierarchy GetMenuCustomizationHierarchy() const { return FCustomizedToolMenuHierarchy(); }
	virtual void UpdateMenuCustomizationFromMultibox(const TSharedRef<const FMultiBox>& InMultiBox) {}
	virtual void OnMenuDestroyed() {}
};

