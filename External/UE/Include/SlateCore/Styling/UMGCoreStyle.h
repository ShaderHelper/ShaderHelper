// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/ISlateStyle.h"
#include "Styling/AppStyle.h"

/**
 * Core slate style
 */
class SLATECORE_API FUMGCoreStyle
{
public:

	static TSharedRef<class ISlateStyle> Create();

	static const ISlateStyle& Get()
	{
		return *(Instance.Get());
	}

	/** Get the default font for Slate */
	static TSharedRef<const FCompositeFont> GetDefaultFont();

	/** Get a font style using the default for for Slate */
	static FSlateFontInfo GetDefaultFontStyle(const FName InTypefaceFontName, const int32 InSize, const FFontOutlineSettings& InOutlineSettings = FFontOutlineSettings());

	static void ResetToDefault( );

	/** Used to override the default selection colors */
	static void SetSelectorColor( const FLinearColor& NewColor );
	static void SetSelectionColor( const FLinearColor& NewColor );
	static void SetInactiveSelectionColor( const FLinearColor& NewColor );
	static void SetPressedSelectionColor( const FLinearColor& NewColor );
	static void SetFocusBrush(FSlateBrush* NewBrush);

	static const int32 RegularTextSize = 10;
	static const int32 SmallTextSize = 8;

	static bool IsInitialized() { return Instance.IsValid(); }
private:

	static void SetStyle(const TSharedRef<class ISlateStyle>& NewStyle);

private:

	/** Singleton instances of this style. */
	static TSharedPtr< class ISlateStyle > Instance;
};
