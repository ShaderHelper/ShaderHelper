#include "CommonHeader.h"
#include "SGraphPin.h"
#include "AssetObject/Graph.h"
#include "UI/Widgets/Graph/SGraphPanel.h"

namespace FW
{

	void SGraphPin::Construct(const FArguments& InArgs, SGraphNode* InOwner)
	{
		PinData = InArgs._PinData;
		Owner = InOwner;
		OwnerPanel = InOwner->Owner;

		auto PinIcon = SNew(SImage)
			.ColorAndOpacity(PinData->GetPinColor())
			.DesiredSizeOverride(FVector2D{ 16,16 })
			.Image(FAppStyle::Get().GetBrush("Icons.BulletPoint"));
		
		ChildSlot
			[
				PinIcon
			];
	}

	FReply SGraphPin::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		if (DragDropOp->IsOfType<GraphDragDropOp>())
		{
			SGraphPin* StartPin = StaticCastSharedPtr<GraphDragDropOp>(DragDropOp)->StartPin;
			if (PinData->Direction != StartPin->PinData->Direction && StartPin->Owner != Owner)
			{
				OwnerPanel->PreviewEnd = OwnerPanel->GetTickSpaceGeometry().AbsoluteToLocal(MyGeometry.GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f)));
				return FReply::Handled();
			}
		}
		return FReply::Unhandled();
	}

	FReply SGraphPin::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		if (DragDropOp->IsOfType<GraphDragDropOp>())
		{
			SGraphPin* StartPin = StaticCastSharedPtr<GraphDragDropOp>(DragDropOp)->StartPin;
			SGraphPin* EndPin = this;
			if (PinData->Direction != StartPin->PinData->Direction && StartPin->Owner != Owner)
			{
				if (PinData->Direction == PinDirection::Output)
				{
					OwnerPanel->RemoveLink(StartPin);
					OwnerPanel->AddLink(EndPin, StartPin);
                    OwnerPanel->GetGraphData()->MarkDirty();
				}
				else
				{
					OwnerPanel->RemoveLink(EndPin);
					OwnerPanel->AddLink(StartPin, EndPin);
                    OwnerPanel->GetGraphData()->MarkDirty();
				}
				OwnerPanel->PreviewStart.Reset();
				return FReply::Handled().ReleaseMouseLock();
			}
		}
		return FReply::Unhandled();
	}

	FReply SGraphPin::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
        if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
        {
            if (PinData->Direction == PinDirection::Input && OwnerPanel->GetOuputPinInLink(this))
            {
                SGraphPin* OutputPin = OwnerPanel->GetOuputPinInLink(this);
                OwnerPanel->PreviewStartDir = OutputPin->PinData->Direction;
                OwnerPanel->PreviewStart = OwnerPanel->GetTickSpaceGeometry().AbsoluteToLocal(OutputPin->GetTickSpaceGeometry().GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f)));
                OwnerPanel->PreviewEnd = OwnerPanel->GetTickSpaceGeometry().AbsoluteToLocal(MyGeometry.GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f)));
                OwnerPanel->RemoveLink(this);
                OwnerPanel->GetGraphData()->MarkDirty();
                
                return FReply::Handled().BeginDragDrop(MakeShared<GraphDragDropOp>(OutputPin));
            }
            else
            {
                OwnerPanel->PreviewStartDir = PinData->Direction;
                OwnerPanel->PreviewStart = OwnerPanel->GetTickSpaceGeometry().AbsoluteToLocal(MyGeometry.GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f)));
                OwnerPanel->PreviewEnd = *OwnerPanel->PreviewStart;
                return FReply::Handled().BeginDragDrop(MakeShared<GraphDragDropOp>(this));
            }
        }
        return FReply::Unhandled();
	}

}
