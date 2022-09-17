// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_ACCESSIBILITY

#include "CoreMinimal.h"
#include "Widgets/Accessibility/SlateCoreAccessibleWidgets.h"

// SButton
class SLATE_API FSlateAccessibleButton
	: public FSlateAccessibleWidget
	, public IAccessibleActivatable
{
public:
	FSlateAccessibleButton(TWeakPtr<SWidget> InWidget) : FSlateAccessibleWidget(InWidget, EAccessibleWidgetType::Button) {}
	virtual ~FSlateAccessibleButton() {}

	virtual IAccessibleActivatable* AsActivatable() override { return this; }

	// IAccessibleActivatable
	virtual void Activate() override;
	// ~
};
// ~

// SCheckBox
class SLATE_API FSlateAccessibleCheckBox
	: public FSlateAccessibleWidget
	, public IAccessibleActivatable
	, public IAccessibleProperty
{
public:
	FSlateAccessibleCheckBox(TWeakPtr<SWidget> InWidget) : FSlateAccessibleWidget(InWidget, EAccessibleWidgetType::CheckBox) {}
	virtual ~FSlateAccessibleCheckBox() {}

	virtual IAccessibleActivatable* AsActivatable() override { return this; }
	virtual IAccessibleProperty* AsProperty() override { return this; }

	// IAccessibleActivatable
	virtual void Activate() override;
	virtual bool IsCheckable() const override;
	virtual bool GetCheckedState() const override;
	// ~
	
	// IAccessibleProperty
	virtual FString GetValue() const override;
	virtual FVariant GetValueAsVariant() const override;
	// ~
};
// ~

// SEditableText
class SLATE_API FSlateAccessibleEditableText
	: public FSlateAccessibleWidget
	, public IAccessibleText
	, public IAccessibleProperty
{
public:
	FSlateAccessibleEditableText(TWeakPtr<SWidget> InWidget) : FSlateAccessibleWidget(InWidget, EAccessibleWidgetType::TextEdit) {}

	// IAccessibleWidget
	virtual IAccessibleText* AsText() override { return this; }
	virtual IAccessibleProperty* AsProperty() override { return this; }
	// ~

	// IAccessibleText
	virtual const FString& GetText() const override;
	// ~

	// IAccessibleProperty
	virtual bool IsReadOnly() const override;
	virtual bool IsPassword() const override;
	virtual FString GetValue() const override;
	virtual FVariant GetValueAsVariant() const override;
	virtual void SetValue(const FString& Value) override;
	// ~
};
// ~

// SEditableTextBox
class SLATE_API FSlateAccessibleEditableTextBox
	: public FSlateAccessibleWidget
	, public IAccessibleText
	, public IAccessibleProperty
{
public:
	FSlateAccessibleEditableTextBox(TWeakPtr<SWidget> InWidget) : FSlateAccessibleWidget(InWidget, EAccessibleWidgetType::TextEdit) {}

	// IAccessibleWidget
	virtual IAccessibleText* AsText() override { return this; }
	virtual IAccessibleProperty* AsProperty() override { return this; }
	// ~

	// IAccessibleText
	virtual const FString& GetText() const override;
	// ~

	// IAccessibleProperty
	virtual bool IsReadOnly() const override;
	virtual bool IsPassword() const override;
	virtual FString GetValue() const override;
	virtual FVariant GetValueAsVariant() const override;
	virtual void SetValue(const FString& Value) override;
	// ~
};
// ~

// SHyperlink
class SLATE_API FSlateAccessibleHyperlink
	: public FSlateAccessibleButton
{
public:
	FSlateAccessibleHyperlink(TWeakPtr<SWidget> InWidget) : FSlateAccessibleButton(InWidget) { WidgetType = EAccessibleWidgetType::Hyperlink; }
	
	// todo: add way to get URL for external hyperlinks? may need to go with SHyperlinkLaunchURL
};
// ~

// Layouts
class SLATE_API FSlateAccessibleLayout
	: public FSlateAccessibleWidget
{
public:
	FSlateAccessibleLayout(TWeakPtr<SWidget> InWidget) : FSlateAccessibleWidget(InWidget, EAccessibleWidgetType::Layout) {}
};
// ~

// SSlider
class SLATE_API FSlateAccessibleSlider
	: public FSlateAccessibleWidget
	, public IAccessibleProperty
{
public:
	FSlateAccessibleSlider(TWeakPtr<SWidget> InWidget) : FSlateAccessibleWidget(InWidget, EAccessibleWidgetType::Slider) {}

	// IAccessibleWidget
	virtual IAccessibleProperty* AsProperty() override { return this; }
	// ~

	// IAccessibleProperty
	virtual bool IsReadOnly() const override;
	virtual float GetStepSize() const override;
	virtual float GetMaximum() const override;
	virtual float GetMinimum() const override;
	virtual FString GetValue() const override;
	virtual FVariant GetValueAsVariant() const override;
	virtual void SetValue(const FString& Value) override;
	// ~
};
// ~

// STextBlock
class SLATE_API FSlateAccessibleTextBlock
	: public FSlateAccessibleWidget
	//, public IAccessibleText // Disabled until we have better support for text. JAWS will not read these properly as-is.
{
public:
	FSlateAccessibleTextBlock(TWeakPtr<SWidget> InWidget) : FSlateAccessibleWidget(InWidget, EAccessibleWidgetType::Text) {}

	// IAccessibleWidget
	//virtual IAccessibleText* AsText() override { return this; }
	// ~

	// IAccessibleText
	virtual const FString& GetText() const /*override*/;
	// ~
};
// ~

#endif
