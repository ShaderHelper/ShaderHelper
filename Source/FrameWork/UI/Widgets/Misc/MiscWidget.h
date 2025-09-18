#pragma once
#include <Widgets/Text/SInlineEditableTextBlock.h>
#include "UI/Styles/FAppCommonStyle.h"

namespace FW
{
	class SShInlineEditableTextBlock : public SInlineEditableTextBlock
	{
	public:
		void Construct(const FArguments& InArgs)
		{
			SInlineEditableTextBlock::Construct(InArgs);
		}
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return FReply::Unhandled(); }
		virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override
		{
			if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !bIsReadOnly.Get())
			{
				EnterEditingMode();
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}
	};

	class SShToggleButton : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SShToggleButton) 
			: _OnCheckStateChanged()
			, _IsChecked(ECheckBoxState::Unchecked)
			{}
			SLATE_ATTRIBUTE(const FSlateBrush*, Icon)
			SLATE_EVENT(FOnCheckStateChanged, OnCheckStateChanged)
			SLATE_ATTRIBUTE(ECheckBoxState, IsChecked)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			IsCheckboxChecked = InArgs._IsChecked;
			OnCheckStateChanged = InArgs._OnCheckStateChanged;
			ChildSlot
			[
				SNew(SBorder)
				.Padding(4)
				.BorderImage(FAppCommonStyle::Get().GetBrush("Effect.Toggle"))
				.BorderBackgroundColor_Lambda([this] {
					if (IsCheckboxChecked.Get() == ECheckBoxState::Checked)
					{
						return FLinearColor{ 0.1f,0.5f,0.9f,1.0f };
					}
					else if(IsHovered())
					{
						return FLinearColor{ 0.5f,0.5f,0.5f,1.0f };
					}
					return FLinearColor::Transparent;
				})
				[
					SNew(SImage).Image(InArgs._Icon)
					.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent&) {
						if (IsCheckboxChecked.Get() == ECheckBoxState::Checked)
						{
							OnCheckStateChanged.ExecuteIfBound(ECheckBoxState::Unchecked);
						}
						else
						{
							OnCheckStateChanged.ExecuteIfBound(ECheckBoxState::Checked);
						}
						return FReply::Handled();
					})
				]
			];
		}

		TAttribute<ECheckBoxState> IsCheckboxChecked;
		FOnCheckStateChanged OnCheckStateChanged;
	};
}
