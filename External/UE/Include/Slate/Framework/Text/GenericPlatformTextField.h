// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Application/IPlatformTextField.h"

class IVirtualKeyboardEntry;

class FGenericPlatformTextField : public IPlatformTextField
{
public:
	virtual void ShowVirtualKeyboard(bool bShow, int32 UserIndex, TSharedPtr<IVirtualKeyboardEntry> TextEntryWidget) override {};
	virtual bool AllowMoveCursor() override { return true; };
};

typedef FGenericPlatformTextField FPlatformTextField;
