#pragma once

namespace SH {
    class SShaderPassTab : public SDockTab
    {
    public:
        void Construct( const FArguments& InArgs )
        {
            SDockTab::Construct(InArgs);
            
            StaticCastSharedRef<SOverlay>(SBorder::GetContent())->AddSlot()
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
                .VAlign(VAlign_Center)
                .HAlign(HAlign_Right)
                [
                    SNew(STextBlock)
                    .ColorAndOpacity(FLinearColor{0.5f,0.5f,0.5f,0.3f})
                    .Text(FText::FromString("|"))
                    .Visibility(this, &SShaderPassTab::HandleEndMarkIsVisible)
                ]
            ];
        }
        
        EVisibility HandleEndMarkIsVisible() const
        {
            return (IsHovered() || IsForeground()) ? EVisibility::Hidden : EVisibility::Visible;
        }
    };
}
