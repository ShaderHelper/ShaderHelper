#pragma once
#include "UI/Widgets/ShaderCodeEditor/ShaderCodeTokenizer.h"

#include <Framework/Text/BaseTextLayoutMarshaller.h>
#include <Framework/Text/SlateTextLayout.h>
#include <Internationalization/IBreakIterator.h>
#include <Widgets/Text/SMultiLineEditableText.h>

namespace SH
{
    class SShaderEditorBox;

    extern const FString FoldMarkerText;

    class FShaderEditorMarshaller : public FBaseTextLayoutMarshaller
    {
    public:
        FShaderEditorMarshaller(SShaderEditorBox* InOwnerWidget, TSharedPtr<ShaderTokenizer> InTokenizer);

    public:
        virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FTextLayout::FLineModel>&& OldLineModels) override;
        virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;

        bool RequiresLiveUpdate() const override
        {
            return true;
        }

    public:
        SShaderEditorBox* OwnerWidget;
        FTextLayout* TextLayout;
        TSharedPtr<ShaderTokenizer> Tokenizer;
        TArray<ShaderTokenizer::TokenizedLine> TokenizedLines;
        TMap<int32, ShaderTokenizer::BracketGroup> FoldingBraceGroups;
        TArray<ShaderTokenizer::BracketGroup> BracketGroups;
        int32 FontSize{};
    };

    class TokenBreakIterator : public IBreakIterator
    {
    public:
        TokenBreakIterator(FShaderEditorMarshaller* InMarshaller);

        virtual void SetString(FString&& InString) override;
        virtual void SetStringRef(FStringView InString) override;

        virtual int32 GetCurrentPosition() const override;

        virtual int32 ResetToBeginning() override;
        virtual int32 ResetToEnd() override;

        virtual int32 MoveToPrevious() override;
        virtual int32 MoveToNext() override;
        virtual int32 MoveToCandidateBefore(const int32 InIndex) override;
        virtual int32 MoveToCandidateAfter(const int32 InIndex) override;

        bool IngoreWhitespace() override { return false; }

    private:
        FString InternalString;
        int32 CurrentPosition{};
        FShaderEditorMarshaller* Marshaller;
    };

    class ShaderTextLayout : public FSlateTextLayout
    {
    public:
        ShaderTextLayout(SWidget* InOwner, FTextBlockStyle InDefaultTextStyle, FShaderEditorMarshaller* Marshaller)
            : FSlateTextLayout(InOwner, InDefaultTextStyle)
        {
            WordBreakIterator = MakeShared<TokenBreakIterator>(Marshaller);
        }
    };

    class FShaderEditorEffectMarshaller : public FBaseTextLayoutMarshaller
    {
    public:
        FShaderEditorEffectMarshaller(SShaderEditorBox* InOwnerWidget)
            : OwnerWidget(InOwnerWidget)
            , TextLayout(nullptr)
        {}

    public:
        virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FTextLayout::FLineModel>&& OldLineModels) override;
        virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;

        void SubmitEffectText();

    public:
        struct DiagEffectInfo
        {
            bool IsError{};
            bool IsWarn{};
            FTextRange DummyRange;
            FTextRange InfoRange;
            FString TotalInfo;
        };

    public:
        SShaderEditorBox* OwnerWidget;
        FTextLayout* TextLayout;
        TMap<int32, DiagEffectInfo> LineNumberToDiagInfo;
    };

    class SShaderMultiLineEditableText : public SMultiLineEditableText
    {
    public:
        void Construct(const FArguments& InArgs, SShaderEditorBox* InOwner)
        {
            Owner = InOwner;
            SMultiLineEditableText::Construct(InArgs);
        }
        virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
        virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
        virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
        virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
        virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

    private:
        SShaderEditorBox* Owner = nullptr;
    };
}