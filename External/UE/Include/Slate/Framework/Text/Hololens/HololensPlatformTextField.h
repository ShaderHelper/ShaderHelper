// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IPlatformTextField.h"

ref class VirtualKeyboardInputContext;

class SLATE_API FHoloLensPlatformTextField : public IPlatformTextField
{
public:
	FHoloLensPlatformTextField();
	~FHoloLensPlatformTextField();

	void LevelChanging(const FString& MapName);
	
	virtual void ShowVirtualKeyboard(bool bShow, int32 UserIndex, TSharedPtr<IVirtualKeyboardEntry> TextEntryWidget) override;
	virtual bool AllowMoveCursor() override { return true; };
	
private:
	VirtualKeyboardInputContext^ inputContext = nullptr;
	FDelegateHandle PreLoadMapHandle;
};

typedef FHoloLensPlatformTextField FPlatformTextField;
