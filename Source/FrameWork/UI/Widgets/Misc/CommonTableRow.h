#pragma once

namespace FRAMEWORK
{
	//The TableRow with bounding box when a drag-drop operation is detected.
    template<typename T>
    class SDropTargetTableRow : public STableRow<T>
    {
    public:
        void Construct(const typename STableRow<T>::FArguments& InArgs,
                       const TSharedRef<STableViewBase>& OwnerTableView)
        {
            STableRow<T>::Construct(
                InArgs,
                OwnerTableView);
        }
        
        void SetDragFilter(const TFunction<bool(const TSharedPtr<FDragDropOperation>&)>& InFilter)
        {
            Filter = InFilter;
        }
        
        void EnableTickDrag() {
            TickDrag = true;
        }
        
    public:
        void OnDragEnter(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent) override
        {
            STableRow<T>::OnDragEnter(MyGeometry, DragDropEvent);
            TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
            if(Filter && Filter(DragDropOp))
            {
                bRecognizedDragDrop = true;
                DragDropOp->SetCursorOverride(EMouseCursor::GrabHand);
            }
            else
            {
                DragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);
            }
        }
        
        void OnDragLeave(FDragDropEvent const& DragDropEvent) override
        {
            STableRow<T>::OnDragLeave(DragDropEvent);
            TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
            bRecognizedDragDrop = false;
            DragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());
        }
        
        FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
        {
            bRecognizedDragDrop = false;
            TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
            if (Filter && Filter(DragDropOp) && STableRow<T>::OnDrop_Handler.IsBound() )
            {
                return STableRow<T>::OnDrop_Handler.Execute(DragDropEvent);
            }
            return FReply::Handled();
        }
        
        void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
        {
            if(TickDrag)
            {
                if ( FSlateApplication::Get().IsDragDropping() )
                {
                    TSharedPtr<FDragDropOperation> DragDropOp = FSlateApplication::Get().GetDragDroppingContent();
                    if(Filter && Filter(DragDropOp))
                    {
                        bRecognizedDragDrop = true;
                    }
                    else
                    {
                        bRecognizedDragDrop = false;
                    }
                }
                else
                {
                    bRecognizedDragDrop = false;
                }
            }
        }
        
        int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override
        {
            LayerId = STableRow<T>::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
            
            if(bRecognizedDragDrop)
            {
                FSlateDrawElement::MakeBox(
                   OutDrawElements,
                   LayerId + 1,
                   AllottedGeometry.ToPaintGeometry(),
                   FAppStyle::Get().GetBrush(TEXT("Debug.Border")),
                   ESlateDrawEffect::None,
                   FLinearColor(0.12f, 0.56f, 1.0f)
                );
            }
            
            return LayerId;
        }
        
    private:
        bool bRecognizedDragDrop = false;
        bool TickDrag = false;
        TFunction<bool(const TSharedPtr<FDragDropOperation>&)> Filter;
    };
	
}
