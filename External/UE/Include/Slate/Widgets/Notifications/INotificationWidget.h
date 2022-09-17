// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "Widgets/Notifications/SNotificationList.h"

class SLATE_API INotificationWidget
{
public:
	virtual void OnSetCompletionState(SNotificationItem::ECompletionState State) = 0;
	virtual TSharedRef< SWidget > AsWidget() = 0;
	virtual bool UseNotificationBackground() const { return true; }
};
