#include "CommonHeader.h"
#include "FShaderHelperStyle.h"
#include "Common/Path/BaseResourcePath.h"
#include <Styling/SlateStyleRegistry.h>
#include <Styling/StyleColors.h>
#include <Styling/SlateStyleMacros.h>

#define RootToContentDir Style->RootToContentDir
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo(RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )

namespace SH {

	void FShaderHelperStyle::Init()
	{
		if (!Instance.IsValid()) {
			Instance = Create();
			FSlateStyleRegistry::RegisterSlateStyle(Get());
		}
	}

	void FShaderHelperStyle::ShutDown()
	{
		if (Instance.IsValid()) {
			FSlateStyleRegistry::UnRegisterSlateStyle(Get());
			Instance.Reset();
		}
	}

	TSharedRef<ISlateStyle> FShaderHelperStyle::Create()
	{
		TSharedRef<FSlateStyleSet> Style = MakeShared<FSlateStyleSet>("ShaderHelperStyle");
		Style->SetContentRoot(BaseResourcePath::UE_SlateResourceDir / TEXT("Slate"));

		FTableRowStyle LineNumberItemStyle;
		LineNumberItemStyle.SetEvenRowBackgroundBrush(FSlateNoResource());
		LineNumberItemStyle.SetOddRowBackgroundBrush(FSlateNoResource());

		Style->Set("LineNumberItemStyle", LineNumberItemStyle);

		FTableRowStyle LineTipItemStyle;
		LineTipItemStyle.SetEvenRowBackgroundBrush(FSlateColorBrush{ FLinearColor::White});
		LineTipItemStyle.SetOddRowBackgroundBrush(FSlateColorBrush{ FLinearColor::White });
		
		Style->Set("LineTipItemStyle", LineTipItemStyle);

		FSlateFontInfo CodeFont = TTF_FONT(TEXT("Fonts/DroidSansMono"), 10);
		Style->Set("CodeFont", CodeFont);

		FTextBlockStyle CodeEditorNormalTextStyle = FTextBlockStyle{}
			.SetFont(CodeFont)
			.SetSelectedBackgroundColor(FLinearColor{ 0.1f , 0.3f, 1.0f, 0.4f })
			.SetColorAndOpacity(FLinearColor::White);
		
		FTextBlockStyle CodeEditorPunctuationTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{1.0f, 0.65f, 0.0f, 1.0f});

		FTextBlockStyle CodeEditorKeywordTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{1.0f, 0.4f, 0.7f, 1.0f});

		FTextBlockStyle CodeEditorNumberTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{ 0.5f, 1.0f, 0.66f, 1.0f });
			
		FTextBlockStyle CodeEditorCommentTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{0.0f, 0.4f, 0.0f, 1.0f});

		FTextBlockStyle CodeEditorBuildtinFuncTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{0.58f, 0.43f, 0.82f,1.0f});

		FTextBlockStyle CodeEditorBuildtinTypeTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{ 0.1f, 0.55f, 1.0f, 1.0f });

		FTextBlockStyle CodeEditorPreprocessTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{0.35f, 0.35f, 0.35f, 1.0f});
			
		Style->Set("CodeEditorNormalText", CodeEditorNormalTextStyle);
		Style->Set("CodeEditorPunctuationText", CodeEditorPunctuationTextStyle);
		Style->Set("CodeEditorKeywordText", CodeEditorKeywordTextStyle);
		Style->Set("CodeEditorNumberText", CodeEditorNumberTextStyle);
		Style->Set("CodeEditorCommentText", CodeEditorCommentTextStyle);
		Style->Set("CodeEditorBuildtinFuncText", CodeEditorBuildtinFuncTextStyle);
		Style->Set("CodeEditorBuildtinTypeText", CodeEditorBuildtinTypeTextStyle);
		Style->Set("CodeEditorPreprocessText", CodeEditorPreprocessTextStyle);
		
		return Style;
	}

}

#undef TTF_FONT
#undef RootToContentDir