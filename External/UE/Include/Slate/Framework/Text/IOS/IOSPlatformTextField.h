// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IPlatformTextField.h"
#include "Internationalization/Text.h"
#include "IOSView.h"

#import <UIKit/UIKit.h>

class IVirtualKeyboardEntry;

@class SlateTextField;

class FIOSPlatformTextField : public IPlatformTextField
{
public:
	FIOSPlatformTextField();
	virtual ~FIOSPlatformTextField();

	virtual void ShowVirtualKeyboard(bool bShow, int32 UserIndex, TSharedPtr<IVirtualKeyboardEntry> TextEntryWidget) override;
	virtual bool AllowMoveCursor() override { return true; };

private:
	SlateTextField* TextField;
};

typedef FIOSPlatformTextField FPlatformTextField;

#if !PLATFORM_TVOS

@interface SlateTextField : UIAlertController
{
	TWeakPtr<IVirtualKeyboardEntry> TextWidget;
	FText TextEntry;
    
    bool bTransitioning;
    bool bWantsToShow;
    NSString* CachedTextContents;
    NSString* CachedPlaceholderContents;
    FKeyboardConfig CachedKeyboardConfig;
    
    UIAlertController* AlertController;
}

-(void)show:(TSharedPtr<IVirtualKeyboardEntry>)InTextWidget text:(NSString*)TextContents placeholder:(NSString*)PlaceholderContents keyboardConfig:(FKeyboardConfig)KeyboardConfig;
-(void)hide;
-(void)updateToDesiredState;
-(bool)hasTextWidget;

@end
#endif
