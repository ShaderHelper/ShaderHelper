// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Geometry.h"
#include "Input/Events.h"

class SWidget;

/**
 * A pair: Widget and its Geometry. Widgets populate an list of WidgetGeometries
 * when they arrange their children. See SWidget::ArrangeChildren.
 */
class FArrangedWidget
{
public:

	FArrangedWidget( TSharedRef<SWidget> InWidget, const FGeometry& InGeometry )
		: Geometry(InGeometry)
		, Widget(InWidget)
	{ }

	SLATECORE_API static const FArrangedWidget& GetNullWidget();

public:

	/** The widget that is being arranged. */
	SWidget* GetWidgetPtr() const
	{
		return &Widget.Get();
	}

	/**
	 * Gets the string representation of the Widget and corresponding Geometry.
	 *
	 * @return String representation.
	 */
	FString ToString( ) const;

public:

	/**
	 * Compares this widget arrangement with another for equality.
	 *
	 * @param Other The other arrangement to compare with.
	 * @return true if the two arrangements are equal, false otherwise.
	 */
	bool operator==( const FArrangedWidget& Other ) const 
	{
		return Widget == Other.Widget;
	}

public:

	/** The widget's geometry. */
	FGeometry Geometry;

	/** The widget that is being arranged. */
	TSharedRef<SWidget> Widget;
};

PRAGMA_DISABLE_DEPRECATION_WARNINGS
struct SLATECORE_API FWidgetAndPointer : public FArrangedWidget
{
public:
	FWidgetAndPointer();
	FWidgetAndPointer(const FArrangedWidget& InWidget);
	FWidgetAndPointer(const FArrangedWidget& InWidget, TOptional<FVirtualPointerPosition> InPosition);

	UE_DEPRECATED(5.0, "FWidgetAndPointer constructor that takes a shared ptr is deprecated.")
	FWidgetAndPointer( const FArrangedWidget& InWidget, const TSharedPtr<const FVirtualPointerPosition>& InPosition );

	TOptional<FVirtualPointerPosition> GetPointerPosition() const
	{
		return OptionalPointerPosition;
	}

	void SetPointerPosition(TOptional<FVirtualPointerPosition> InPosition)
	{
		OptionalPointerPosition = InPosition;
	}

public:
#if WITH_EDITOR
	UE_DEPRECATED(5.0, "Direct access to PointerPosition is now deprecated. Use the getter or setter.")
	TSharedPtr<const FVirtualPointerPosition> PointerPosition;
#endif

private:
	TOptional<FVirtualPointerPosition> OptionalPointerPosition;
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS
