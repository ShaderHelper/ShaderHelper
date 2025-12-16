#include "CommonHeader.h"
#include "STimeline.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "UI/Styles/FAppCommonStyle.h"
#include "App/App.h"

#include <Widgets/Input/SNumericEntryBox.h>
#include <Fonts/FontMeasure.h>

namespace FW
{
    void STimeline::Construct(const FArguments& InArgs)
    {
        bStop = InArgs._bStop;
        CurTime = InArgs._CurTime;
        MaxTime = InArgs._MaxTime;
		OnHandleMove = InArgs._OnHandleMove;

        ChildSlot
        [
            SNew(SBorder)
            .Padding(0)
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SNumericEntryBox<uint16>)
                        .MinValue(10)
                        .MaxValue(10000)
                        .Justification(ETextJustify::Center)
                        .Value_Lambda( [this] { return *MaxTime; } )
                        .OnValueCommitted_Lambda([this] (uint16 InValue, ETextCommit::Type CommitInfo){
                            *MaxTime = InValue;
                        })
                        .MinDesiredValueWidth(25)
                ]
                +SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SIconButton).Icon(FAppCommonStyle::Get().GetBrush("Timeline.LeftEnd"))
                    .OnClicked_Lambda([this]{
                        *CurTime = 0;
						OnHandleMove(*CurTime);
                        return FReply::Handled();
                    })
                ]
                +SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SIconButton).Icon_Lambda([this]{
                        if(*bStop) {
                            return FAppCommonStyle::Get().GetBrush("Timeline.Play");
                        }
                        return FAppCommonStyle::Get().GetBrush("Timeline.Pause");
                    })
                    .OnClicked_Lambda([this]{
                        *bStop = !*bStop;
                        return FReply::Handled();
                    })
                ]
                +SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SIconButton).Icon(FAppCommonStyle::Get().GetBrush("Timeline.RightEnd"))
                    .OnClicked_Lambda([this]{
                        *CurTime = *MaxTime;
						OnHandleMove(*CurTime);
                        return FReply::Handled();
                    })
                ]
                +SHorizontalBox::Slot()
                .Padding(16.0f, 0, 16.0f ,0)
                [
                    SNew(STimelineArea).CurTime(CurTime).MaxTime(MaxTime)
					.OnHandleMove(InArgs._OnHandleMove)
                ]
            ]

        ];
    }

    void STimeline::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
    {
        if(!*bStop) {
            //InDeltaTime can not be used, because ue limits the value to 1/8.0f for slate.
            *CurTime += GApp->GetDeltaTime();
            if(*CurTime > *MaxTime)
            {
                *CurTime = 0;
            }
        }
    }

    void STimelineArea::Construct(const FArguments& InArgs)
    {
        MaxTime = InArgs._MaxTime;
        CurTime = InArgs._CurTime;
		OnHandleMove = InArgs._OnHandleMove;
    }

    FVector2D STimelineArea::ComputeDesiredSize(float) const
    {
        return FVector2D::Zero();
    }

    FReply STimelineArea::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    {
        if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
        {
			const float ValuesPerPixel = float(*MaxTime / MyGeometry.GetLocalSize().X);
			float NewTime = float(ValuesPerPixel * MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X);
            *CurTime = FMath::Clamp(NewTime, 0.0f, float(*MaxTime));
			if(OnHandleMove)
			{
				OnHandleMove(*CurTime);
			}
            return FReply::Handled().CaptureMouse(SharedThis(this));
        }
        return FReply::Unhandled();
    }

    FReply STimelineArea::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    {
        if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
        {
            return FReply::Handled().ReleaseMouseCapture();
        }
        return FReply::Unhandled();
    }

    FReply STimelineArea::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    {
        if(HasMouseCapture())
        {
             const bool bIsLeftMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);
            if(bIsLeftMouseButtonDown)
            {
                const float ValuesPerPixel = float(*MaxTime / MyGeometry.GetLocalSize().X);
                float NewTime = float(ValuesPerPixel * MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X);
                *CurTime = FMath::Clamp(NewTime, 0.0f, float(*MaxTime));
				if(OnHandleMove)
				{
					OnHandleMove(*CurTime);
				}
            }
        }
        return FReply::Handled();
    }

    int32 STimelineArea::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
    {
        const auto BrushResource = FAppStyle::Get().GetBrush("Brushes.Recessed");
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            FAppStyle::Get().GetBrush("Brushes.Recessed"),
            ESlateDrawEffect::None,
            BrushResource->GetTint(InWidgetStyle)
        );
        
        //Draw scale.
        const float PixelsPerValue = float(AllottedGeometry.GetLocalSize().X / *MaxTime);
        const float Spacing = *MaxTime / 50;
        const int MajorDivide = 5;
        
        TArray<FVector2D> LinePoints;
        LinePoints.AddUninitialized(2);
        
        const FSlateFontInfo& Font = FAppCommonStyle::Get().GetWidgetStyle<FTextBlockStyle>("SmallMinorText").Font;
        for(int Offset = 0; Offset*Spacing < *MaxTime + 1e-3; Offset++)
        {
            float XPos = Offset * Spacing * PixelsPerValue;
            float YOffset = float(AllottedGeometry.GetLocalSize().Y / 1.2f);
            if(Offset % MajorDivide == 0)
            {
                YOffset = float(AllottedGeometry.GetLocalSize().Y / 2);
                //Draw value
                int Value = int(Offset * Spacing);
                FString ValueString = FString::FromInt(Value);
                const TSharedRef< FSlateFontMeasure > FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
                FVector2D TextSize = FontMeasureService->Measure(ValueString, Font);
                FVector2D TextOffset{XPos - TextSize.X / 2, 1.0f};

                FSlateDrawElement::MakeText(
                    OutDrawElements,
                    LayerId,
                    AllottedGeometry.ToPaintGeometry(TextOffset, TextSize),
                    ValueString,
                    Font,
					ESlateDrawEffect::None,
					FStyleColors::Foreground.GetColor(InWidgetStyle)
                );
            }
            LinePoints[0] = FVector2D{XPos, YOffset};
            LinePoints[1] = FVector2D{XPos, AllottedGeometry.GetLocalSize().Y};
            
            FSlateDrawElement::MakeLines(
                OutDrawElements,
                LayerId,
                AllottedGeometry.ToPaintGeometry(),
                LinePoints,
				ESlateDrawEffect::None,
				FStyleColors::Foreground.GetColor(InWidgetStyle)
            );
        }
        
        //Draw handle
        float XPos = *CurTime * PixelsPerValue;
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry({XPos - 3.0f, 0}, {6.0f, AllottedGeometry.GetLocalSize().Y}),
            FAppStyle::Get().GetBrush("WhiteBrush"),
            ESlateDrawEffect::None,
            FLinearColor{0.5f, 0.5f, 0.5f, 0.7f}
        );
        
        return LayerId;
    }

}
