// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

OBJC_EXPORT @interface FCocoaMenu : NSMenu
{
@private
	bool bHighlightingKeyEquivalent;
}
- (bool)isHighlightingKeyEquivalent;
- (bool)highlightKeyEquivalent:(NSEvent *)Event;
@end
