// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"

/**
 * AppStyle class
 *
 * AppStyle is a Singleton accessor to a named SlateStyle to be used as an Application wide
 * base style definition.
 * 
 * Moving forward, all references in any core Slate Application Widgets should use FAppStyle::Get()
 *
 * FEditorStyle::Get accessors and FCoreStyle::Get accessors should be replaced with FAppStyle::Get()
 *
 * Currently, this code defaults to use FCoreStyle::GetCoreStyle() if the named style is not 
 * found.
 *
 */

class SLATECORE_API FAppStyle
{

public:

	static const ISlateStyle& Get();
	
	static const FName GetAppStyleSetName();
	static void SetAppStyleSetName(const FName& InName);

	static void SetAppStyleSet(const ISlateStyle& InStyle);

private:

	static FName AppStyleName;

};