#pragma once

namespace FW
{
    class FRAMEWORK_API STimeline : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(STimeline)
        {}
            SLATE_ARGUMENT(bool*, bStop)
            SLATE_ARGUMENT(float*, CurTime)
            SLATE_ARGUMENT(float*, MaxTime)
        SLATE_END_ARGS()
        void Construct(const FArguments& InArgs);
        virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
    private:
        bool* bStop = nullptr;
        float* CurTime = nullptr;
        float* MaxTime = nullptr;
    };

    class FRAMEWORK_API STimelineArea : public SLeafWidget
    {
    public:
        SLATE_BEGIN_ARGS(STimelineArea)
        {}
            SLATE_ARGUMENT(float*, CurTime)
            SLATE_ARGUMENT(float*, MaxTime)
        SLATE_END_ARGS()

        void Construct(const FArguments& InArgs);
    public:
        virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;
        virtual FVector2D ComputeDesiredSize(float) const override;
        virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
        
    private:
        float* CurTime = nullptr;
        float* MaxTime = nullptr;
    };
}
