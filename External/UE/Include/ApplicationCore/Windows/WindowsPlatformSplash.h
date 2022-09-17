// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "GenericPlatform/GenericPlatformSplash.h"


/**
 * Windows splash implementation.
 */
struct APPLICATIONCORE_API FWindowsPlatformSplash
	: public FGenericPlatformSplash
{
	/** Show the splash screen. */
	static void Show();

	/** Hide the splash screen. */
	static void Hide();

	/**
	 * Sets the progress displayed on the application icon (for startup/loading progress).
	 *
	 * @param InType Progress value in percent.
	 */
	static void SetProgress(int ProgressPercent);

	/**
	 * Sets the text displayed on the splash screen (for startup/loading progress).
	 *
	 * @param InType Type of text to change.
	 * @param InText Text to display.
	 */
	static void SetSplashText( const SplashTextType::Type InType, const TCHAR* InText );

	/**
	 * Return whether the splash screen is being shown or not
	 */
	static bool IsShown();
};


typedef FWindowsPlatformSplash FPlatformSplash;
