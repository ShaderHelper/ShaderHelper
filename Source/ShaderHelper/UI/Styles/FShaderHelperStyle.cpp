#include "CommonHeader.h"
#include "FShaderHelperStyle.h"
#include "Common/Path/BaseResourcePath.h"
#include <Styling/SlateStyleRegistry.h>
#include <Styling/StyleColors.h>
#include <Styling/SlateStyleMacros.h>
#include <Styling/ToolBarStyle.h>
#include "UI/Styles/FAppCommonStyle.h"

#define RootToContentDir Style->RootToContentDir
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo(RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )

using namespace FW;

namespace SH {

    const ISlateStyle& FShaderHelperStyle::Get()
    {
        if (!Instance.IsValid()) {
            Instance = Create();
            FSlateStyleRegistry::RegisterSlateStyle(Get());
        }
        return *Instance;
    }

	TSharedRef<ISlateStyle> FShaderHelperStyle::Create()
	{
		TSharedRef<FSlateStyleSet> Style = MakeShared<FSlateStyleSet>("ShaderHelperStyle");
        Style->SetContentRoot(BaseResourcePath::Custom_SlateResourceDir);
        Style->Set("AssetBrowser.Shader", new IMAGE_BRUSH_SVG("Shader", FVector2D(64.0, 64.0)));
		Style->Set("Icons.StepInto", new IMAGE_BRUSH_SVG("StepInto", FVector2D(16.0, 16.0)));
		Style->Set("Icons.StepOver", new IMAGE_BRUSH_SVG("StepOver", FVector2D(16.0, 16.0)));
		Style->Set("Icons.Bug", new IMAGE_BRUSH("Bug", FVector2D(16.0, 16.0)));
		Style->Set("Icons.StepBack", new IMAGE_BRUSH_SVG("StepBack", FVector2D(16.0, 16.0)));
		Style->Set("Icons.StepOut", new IMAGE_BRUSH_SVG("StepOut", FVector2D(16.0, 16.0)));
		Style->Set("Icons.Pause", new IMAGE_BRUSH_SVG("Pause", FVector2D(16.0, 16.0), FLinearColor::Red));
		Style->Set("Icons.ArrowBoldRight", new IMAGE_BRUSH_SVG("ArrowBoldRight", FVector2D(14.0, 14.0)));
		Style->Set("LineTip.BreakPointEffect", new IMAGE_BRUSH("BreakPointEffect", FVector2D(10.0, 10.0)));
		
		Style->SetContentRoot(BaseResourcePath::UE_SlateResourceDir);
		FToolBarStyle ShToolbarStyle = FToolBarStyle::GetDefault();
		ShToolbarStyle.SetButtonPadding(FMargin{4,0});
		ShToolbarStyle.SetBackground(FSlateColorBrush(FStyleColors::Recessed));
		ShToolbarStyle.SetButtonStyle(FAppCommonStyle::Get().GetWidgetStyle< FButtonStyle >("SuperSimpleButton"));
		Style->Set("Toolbar.ShaderHelper", ShToolbarStyle);
		
		Style->Set("Icons.FullText", new IMAGE_BRUSH_SVG("Starship/Insights/MemTags_20", FVector2D(16.0, 16.0)));
		Style->Set("LineTip.BreakPointEffect2", new IMAGE_BRUSH("Common/Window/WindowTitle_Flashing", FVector2D(16.0, 16.0), FLinearColor::White, ESlateBrushTileType::Horizontal));

		FTableRowStyle LineNumberItemStyle;
		LineNumberItemStyle.SetEvenRowBackgroundBrush(FSlateNoResource());
		LineNumberItemStyle.SetOddRowBackgroundBrush(FSlateNoResource());

		Style->Set("LineNumberItemStyle", LineNumberItemStyle);

		FTableRowStyle LineTipItemStyle;
		LineTipItemStyle.SetEvenRowBackgroundBrush(FSlateColorBrush{ FLinearColor::White});
		LineTipItemStyle.SetOddRowBackgroundBrush(FSlateColorBrush{ FLinearColor::White });
		
		Style->Set("LineTipItemStyle", LineTipItemStyle);

        TSharedRef<FCompositeFont> CodeFont = MakeShared<FStandaloneCompositeFont>();
        CodeFont->DefaultTypeface.AppendFont(TEXT("Code"), BaseResourcePath::UE_SlateFontDir / TEXT("DroidSansMono.ttf"), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
        CodeFont->FallbackTypeface.Typeface.AppendFont(TEXT("Code"), BaseResourcePath::UE_SlateFontDir / TEXT("DroidSansFallback.ttf"), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
        FSlateFontInfo CodeFontInfo = FSlateFontInfo(CodeFont, 10);
        Style->Set("CodeFont", CodeFontInfo);

		FTextBlockStyle CodeEditorNormalTextStyle = FTextBlockStyle{}
			.SetFont(CodeFontInfo)
			.SetColorAndOpacity(FLinearColor::White);
		
		FTextBlockStyle CodeEditorKeywordTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{1.0f, 0.4f, 0.7f, 1.0f});
		FTextBlockStyle CodeEditorNumberTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{ 0.5f, 1.0f, 0.66f, 1.0f });
		FTextBlockStyle CodeEditorCommentTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{0.2f, 0.9f, 0.2f, 1.0f});
		FTextBlockStyle CodeEditorBuildtinFuncTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{0.58f, 0.43f, 0.82f,1.0f});
		FTextBlockStyle CodeEditorBuildtinTypeTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{ 0.1f, 0.55f, 1.0f, 1.0f });
		FTextBlockStyle CodeEditorPreprocessTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{0.35f, 0.35f, 0.35f, 1.0f});
		FTextBlockStyle CodeEditorStringTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{ 1.0f, 0.8f, 0.66f, 1.0f });
		
		FTextBlockStyle CodeEditorFuncTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{ 1.0f, 0.93f, 0.54f, 1.0f });
		FTextBlockStyle CodeEditorTypeTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{ 0.0f, 0.81f, 0.81f, 1.0f });
		FTextBlockStyle CodeEditorParmTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{ 0.39f, 0.72f, 1.0f, 1.0f });
		FTextBlockStyle CodeEditorVarTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor{ 0.93f, 0.51f, 0.38f, 1.0f });
		
		Style->Set("CodeEditorNormalText", CodeEditorNormalTextStyle);
		Style->Set("CodeEditorKeywordText", CodeEditorKeywordTextStyle);
		Style->Set("CodeEditorNumberText", CodeEditorNumberTextStyle);
		Style->Set("CodeEditorCommentText", CodeEditorCommentTextStyle);
		Style->Set("CodeEditorBuildtinFuncText", CodeEditorBuildtinFuncTextStyle);
		Style->Set("CodeEditorBuildtinTypeText", CodeEditorBuildtinTypeTextStyle);
		Style->Set("CodeEditorPreprocessText", CodeEditorPreprocessTextStyle);
		Style->Set("CodeEditorStringText", CodeEditorStringTextStyle);
		
		Style->Set("CodeEditorFuncText", CodeEditorFuncTextStyle);
		Style->Set("CodeEditorTypeText", CodeEditorTypeTextStyle);
		Style->Set("CodeEditorParmText", CodeEditorParmTextStyle);
		Style->Set("CodeEditorVarText", CodeEditorVarTextStyle);

		FTextBlockStyle CodeEditorErrorInfoStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FLinearColor::Red);
		Style->Set("CodeEditorErrorInfoText", CodeEditorErrorInfoStyle);
		
		const FVector2D Icon14x14(14.0f, 14.0f);
		
		const FButtonStyle ArrowDownButton = FButtonStyle()
			.SetNormal(IMAGE_BRUSH("Extra/ArrowDown", Icon14x14))
			.SetHovered(IMAGE_BRUSH("Extra/ArrowDown", Icon14x14))
			.SetPressed(IMAGE_BRUSH("Extra/ArrowDown", Icon14x14));

		const FButtonStyle ArrowRightButton = FButtonStyle()
			.SetNormal(IMAGE_BRUSH("Extra/ArrowRight", Icon14x14))
			.SetHovered(IMAGE_BRUSH("Extra/ArrowRight", Icon14x14))
			.SetPressed(IMAGE_BRUSH("Extra/ArrowRight", Icon14x14));

		Style->Set("ArrowDownButton", ArrowDownButton);
		Style->Set("ArrowRightButton", ArrowRightButton);

		Style->Set("Icons.World", new IMAGE_BRUSH_SVG("Starship/Common/world", FVector2D{16.0f,16.0f}));
        
        const FScrollBarStyle ScrollBar = FScrollBarStyle()
            .SetVerticalBackgroundImage(FSlateColorBrush(FStyleColors::Recessed))
            .SetNormalThumbImage(FSlateColorBrush(FStyleColors::Hover))
            .SetDraggedThumbImage(FSlateColorBrush(FStyleColors::Hover2))
            .SetHoveredThumbImage(FSlateColorBrush(FStyleColors::Hover2))
            .SetThickness(5.0f);
		Style->Set("CustomScrollbar", ScrollBar);
        
		return Style;
	}

}

#undef TTF_FONT
#undef RootToContentDir
