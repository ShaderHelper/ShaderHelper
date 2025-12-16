#pragma once

namespace SH {
    class SShaderTab : public SDockTab
    {
    public:
		~SShaderTab()
		{
			
		}
		
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
                    .Text(FText::FromString("|"))
                    .Visibility(this, &SShaderTab::IsEndMarkVisible)
                ]
            ];
        }
        
        EVisibility IsEndMarkVisible() const
        {
            return (IsHovered() || IsForeground()) ? EVisibility::Hidden : EVisibility::Visible;
        }
    };
}
