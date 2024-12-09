#pragma once

namespace SH {
    class SShaderTab : public SDockTab
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
                    .ColorAndOpacity(FLinearColor{0.5f,0.5f,0.5f,0.4f})
                    .Text(FText::FromString("|"))
                    .Visibility(this, &SShaderTab::HandleEndMarkIsVisible)
                ]
            ];
        }
        
        EVisibility HandleEndMarkIsVisible() const
        {
            return (IsHovered() || IsForeground()) ? EVisibility::Hidden : EVisibility::Visible;
        }
    };
}
