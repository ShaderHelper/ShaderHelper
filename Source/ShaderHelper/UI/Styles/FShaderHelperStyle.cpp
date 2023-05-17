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
		Style->SetContentRoot(BaseResourcePath::UE_SlateResourceDir / TEXT("Custom") );

		FTableRowStyle LineNumberItemStyle;
		LineNumberItemStyle.SetEvenRowBackgroundBrush(FSlateNoResource());
		LineNumberItemStyle.SetOddRowBackgroundBrush(FSlateNoResource());

		Style->Set("LineNumberItemStyle", LineNumberItemStyle);

		FTableRowStyle LineTipItemStyle;
		LineTipItemStyle.SetEvenRowBackgroundBrush(FSlateColorBrush{ FLinearColor::White});
		LineTipItemStyle.SetOddRowBackgroundBrush(FSlateColorBrush{ FLinearColor::White });
		
		Style->Set("LineTipItemStyle", LineTipItemStyle);

		FSlateFontInfo CodeFont = TTF_FONT(TEXT("Fonts/Consolas"), 10);
		Style->Set("CodeFont", CodeFont);

		FTextBlockStyle CodeEditorTextStyle = FTextBlockStyle{ FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText") }
			.SetFont(CodeFont)
			.SetSelectedBackgroundColor(FLinearColor{ 0.1f , 0.3f, 1.0f, 0.4f });
		Style->Set("CodeEditor", CodeEditorTextStyle);
		
		return Style;
	}

}

#undef TTF_FONT
#undef RootToContentDir