#pragma once
#include <Widgets/Text/SInlineEditableTextBlock.h>

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
}
