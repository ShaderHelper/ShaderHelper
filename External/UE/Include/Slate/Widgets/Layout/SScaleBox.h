// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Attribute.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "SScaleBox.generated.h"

class FArrangedChildren;
class FPaintArgs;
class FSlateWindowElementList;

UENUM(BlueprintType)
namespace EStretchDirection
{
	enum Type
	{
		/** Will scale the content up or down. */
		Both,
		/** Will only make the content smaller, will never scale it larger than the content's desired size. */
		DownOnly,
		/** Will only make the content larger, will never scale it smaller than the content's desired size. */
		UpOnly
	};
}

UENUM(BlueprintType)
namespace EStretch
{
	enum Type
	{
		/** Does not scale the content. */
		None,
		/** Scales the content non-uniformly filling the entire space of the area. */
		Fill,
		/**
		 * Scales the content uniformly (preserving aspect ratio) 
		 * until it can no longer scale the content without clipping it.
		 */
		ScaleToFit,
		/**
		 * Scales the content uniformly (preserving aspect ratio) 
		 * until it can no longer scale the content without clipping it along the x-axis, 
		 * the y-axis can/will be clipped.
		 */
		ScaleToFitX,
		/**
		 * Scales the content uniformly (preserving aspect ratio) 
		 * until it can no longer scale the content without clipping it along the y-axis, 
		 * the x-axis can/will be clipped.
		 */
		ScaleToFitY,
		/**
		 * Scales the content uniformly (preserving aspect ratio), until all sides meet 
		 * or exceed the size of the area.  Will result in clipping the longer side.
		 */
		ScaleToFill,
		/** Scales the content according to the size of the safe zone currently applied to the viewport. */
		ScaleBySafeZone,
		/** Scales the content by the scale specified by the user. */
		UserSpecified
	};
}

/**
 * Allows you to place content with a desired size and have it scale to meet the constraints placed on this box's alloted area.  If
 * you needed to have a background image scale to fill an area but not become distorted with different aspect ratios, or if you need
 * to auto fit some text to an area, this is the control for you.
 */
class SLATE_API SScaleBox : public SCompoundWidget
{
	SLATE_DECLARE_WIDGET(SScaleBox, SCompoundWidget)

public:
	SLATE_BEGIN_ARGS(SScaleBox)
	: _Content()
	, _HAlign(HAlign_Center)
	, _VAlign(VAlign_Center)
	, _StretchDirection(EStretchDirection::Both)
	, _Stretch(EStretch::None)
	, _UserSpecifiedScale(1.0f)
	, _IgnoreInheritedScale(false)
	{}
		/** Slot for this designers content (optional) */
		SLATE_DEFAULT_SLOT(FArguments, Content)

		/** The horizontal alignment of the content */
		SLATE_ARGUMENT(EHorizontalAlignment, HAlign)

		/** The vertical alignment of the content */
		SLATE_ARGUMENT(EVerticalAlignment, VAlign)
		
		/** Controls in what direction content can be scaled */
		SLATE_ATTRIBUTE(EStretchDirection::Type, StretchDirection)
		
		/** The stretching rule to apply when content is stretched */
		SLATE_ATTRIBUTE(EStretch::Type, Stretch)

		/** Optional scale that can be specified by the User */
		SLATE_ATTRIBUTE(float, UserSpecifiedScale)

		/** Undo any inherited scale factor before applying this scale box's scale */
		SLATE_ATTRIBUTE(bool, IgnoreInheritedScale)

#if WITH_EDITOR
		/** Force a particular screen size to be used instead of the reported device size. */
		SLATE_ARGUMENT(TOptional<FVector2D>, OverrideScreenSize)
#endif

	SLATE_END_ARGS()

protected:
	/** Constructor */
	SScaleBox();

public:
	virtual ~SScaleBox();

	void Construct(const FArguments& InArgs);
	
	// SWidget interface
	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	// End SWidget of interface

	/** See Content slot */
	void SetContent(TSharedRef<SWidget> InContent);

	/** See HAlign argument */
	void SetHAlign(EHorizontalAlignment HAlign);

	/** See VAlign argument */
	void SetVAlign(EVerticalAlignment VAlign);

	/** See StretchDirection argument */
	void SetStretchDirection(EStretchDirection::Type InStretchDirection);

	/** See Stretch argument */
	void SetStretch(EStretch::Type InStretch);

	/** See UserSpecifiedScale argument */
	void SetUserSpecifiedScale(float InUserSpecifiedScale);

	/** Set IgnoreInheritedScale argument */
	void SetIgnoreInheritedScale(bool InIgnoreInheritedScale);

#if WITH_EDITOR
	void SetOverrideScreenInformation(TOptional<FVector2D> InScreenSize);
#endif
	
protected:
	// Begin SWidget overrides.
	virtual bool CustomPrepass(float LayoutScaleMultiplier) override;
	virtual FVector2D ComputeDesiredSize(float InScale) const override;
	virtual float GetRelativeLayoutScale(int32 ChildIndex, float LayoutScaleMultiplier) const override;
	// End SWidget overrides.

	bool DoesScaleRequireNormalizingPrepassOrLocalGeometry() const;
	bool IsDesiredSizeDependentOnAreaAndScale() const;
	float ComputeContentScale(const FGeometry& PaintGeometry) const;

	void RefreshSafeZoneScale();
	void HandleSafeFrameChangedEvent();

#if WITH_EDITOR
	void DebugSafeAreaUpdated(const FMargin& NewSafeZone, bool bShouldRecacheMetrics);
#endif

protected:
#if WITH_EDITORONLY_DATA
	UE_DEPRECATED(5.0, "Direct access to StretchDirection is now deprecated. Use the setter or getter.")
	TSlateDeprecatedTAttribute<EStretchDirection::Type> StretchDirection;
	UE_DEPRECATED(5.0, "Direct access to Stretch is now deprecated. Use the setter or getter.")
	TSlateDeprecatedTAttribute<EStretch::Type> Stretch;
	UE_DEPRECATED(5.0, "Direct access to UserSpecifiedScale is now deprecated. Use the setter or getter.")
	TSlateDeprecatedTAttribute<float> UserSpecifiedScale;
	UE_DEPRECATED(5.0, "Direct access to IgnoreInheritedScale is now deprecated. Use the setter or getter.")
	TSlateDeprecatedTAttribute<bool> IgnoreInheritedScale;
#endif

private:
	/** The allowed direction of stretching of the content */
	TSlateAttribute<EStretchDirection::Type> StretchDirectionAttribute;

	/** The method of scaling that is applied to the content. */
	TSlateAttribute<EStretch::Type> StretchAttribute;

	/** Optional scale that can be specified by the User */
	TSlateAttribute<float> UserSpecifiedScaleAttribute;

	/** Optional bool to ignore the inherited scale */
	TSlateAttribute<bool> IgnoreInheritedScaleAttribute;

	/** Computed scale when scaled by safe zone padding */
	float SafeZoneScale;

	/** Delegate handle to unhook the safe frame changed. */
	FDelegateHandle OnSafeFrameChangedHandle;

	mutable TOptional<FVector2D> LastAllocatedArea;
	mutable TOptional<FGeometry> LastPaintGeometry;

	mutable TOptional<FVector2D> NormalizedContentDesiredSize;

	/**  */
	mutable TOptional<float> ComputedContentScale;




	/**  */
	mutable FVector2D LastFinalOffset;

	/**  */
	mutable FVector2D LastSlotWidgetDesiredSize;

#if WITH_EDITOR
	TOptional<FVector2D> OverrideScreenSize;
#endif
};
