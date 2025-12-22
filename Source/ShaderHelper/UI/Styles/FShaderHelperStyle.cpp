#include "CommonHeader.h"
#include "FShaderHelperStyle.h"
#include "Common/Path/BaseResourcePath.h"
#include "UI/Styles/FAppCommonStyle.h"

#include <Styling/SlateStyleRegistry.h>
#include <Styling/StyleColors.h>
#include <Styling/SlateStyleMacros.h>
#include <Styling/ToolBarStyle.h>

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
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Foreground, LOCALIZATION("Foreground"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::ForegroundHover, LOCALIZATION("ForegroundHover"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Border, LOCALIZATION("Border"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Background, LOCALIZATION("Background"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Header, LOCALIZATION("Header"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Panel, LOCALIZATION("Panel"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Input, LOCALIZATION("Input"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Recessed, LOCALIZATION("Recessed"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Secondary, LOCALIZATION("Secondary"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Dropdown, LOCALIZATION("Dropdown"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::SelectHover, LOCALIZATION("SelectHover"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Hover, LOCALIZATION("Hover"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Primary, LOCALIZATION("Primary"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Select, LOCALIZATION("Select"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::SelectInactive, LOCALIZATION("SelectInactive"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::Title, LOCALIZATION("Title"));

		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User1, LOCALIZATION("Keyword"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User2, LOCALIZATION("Number"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User3, LOCALIZATION("Comment"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User4, LOCALIZATION("BuiltinFunc"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User5, LOCALIZATION("BuiltinType"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User6, LOCALIZATION("Preprocess"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User7, LOCALIZATION("String"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User8, LOCALIZATION("Parameter"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User9, LOCALIZATION("Function"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User10, LOCALIZATION("Type"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User11, LOCALIZATION("GlobalVar"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User12, LOCALIZATION("Other"));
		USlateThemeManager::Get().SetColorDisplayName(EStyleColor::User13, LOCALIZATION("Punctuation"));

		TSharedRef<FSlateStyleSet> Style = MakeShared<FSlateStyleSet>("ShaderHelperStyle");
        Style->SetContentRoot(BaseResourcePath::Custom_SlateResourceDir);
        Style->Set("AssetBrowser.Shader", new IMAGE_BRUSH_SVG("Shader", FVector2D(64.0, 64.0)));
		Style->Set("Icons.StepInto", new IMAGE_BRUSH_SVG("StepInto", FVector2D(16.0, 16.0)));
		Style->Set("Icons.StepOver", new IMAGE_BRUSH_SVG("StepOver", FVector2D(16.0, 16.0)));
		Style->Set("Icons.Bug", new IMAGE_BRUSH("Bug", FVector2D(16.0, 16.0), FLinearColor::Green));
		Style->Set("Icons.StepBack", new IMAGE_BRUSH_SVG("StepBack", FVector2D(16.0, 16.0)));
		Style->Set("Icons.StepOut", new IMAGE_BRUSH_SVG("StepOut", FVector2D(16.0, 16.0)));
		Style->Set("Icons.Pause", new IMAGE_BRUSH_SVG("Pause", FVector2D(16.0, 16.0), FLinearColor::Red));
		Style->Set("Icons.ArrowBoldRight", new IMAGE_BRUSH_SVG("ArrowBoldRight", FVector2D(14.0, 14.0)));
		Style->Set("LineTip.BreakPointEffect", new IMAGE_BRUSH("BreakPointEffect", FVector2D(10.0, 10.0)));
		Style->Set("Icons.CallStack", new IMAGE_BRUSH_SVG("CallStack", FVector2D(16.0, 16.0)));
		Style->Set("Icons.Variable", new IMAGE_BRUSH_SVG("Variable", FVector2D(16.0, 16.0)));
		Style->Set("Icons.Uninitialized", new IMAGE_BRUSH_SVG("Watch", FVector2D(16.0, 16.0)));
		Style->Set("Icons.MatchCase", new IMAGE_BRUSH("MatchCase", FVector2D{ 16.0f,16.0f }));
		Style->Set("Icons.MatchWhole", new IMAGE_BRUSH("MatchWhole", FVector2D{ 16.0f,16.0f }));
		Style->Set("Icons.Replace", new IMAGE_BRUSH_SVG("Replace", FVector2D{ 16.0f,16.0f }));
		Style->Set("Icons.ReplaceAll", new IMAGE_BRUSH_SVG("ReplaceAll", FVector2D{ 16.0f,16.0f }));

		Style->SetContentRoot(BaseResourcePath::UE_SlateResourceDir);
		FToolBarStyle ShToolbarStyle = FToolBarStyle::GetDefault();
		ShToolbarStyle.SetButtonPadding(FMargin{4,0});
		ShToolbarStyle.SetBackground(FSlateColorBrush(FStyleColors::Recessed));
		ShToolbarStyle.SetButtonStyle(FAppCommonStyle::Get().GetWidgetStyle< FButtonStyle >("SuperSimpleButton"));
		Style->Set("Toolbar.ShaderHelper", ShToolbarStyle);
		Style->Set("Icons.FullText", new IMAGE_BRUSH_SVG("Starship/Insights/MemTags_20", FVector2D(16.0, 16.0)));
		Style->Set("Icons.Validation", new IMAGE_BRUSH_SVG("Starship/Insights/Tasks_20", FVector2D(16.0, 16.0)));
		Style->Set("LineTip.BreakPointEffect2", new IMAGE_BRUSH("Common/Window/WindowTitle_Flashing", FVector2D(16.0, 16.0), FLinearColor::White, ESlateBrushTileType::Horizontal));

		const FHeaderRowStyle& CoreHeaderRowStyle = FAppStyle::Get().GetWidgetStyle<FHeaderRowStyle>("TableView.Header");
		const FTableColumnHeaderStyle CoreTableColumnHeaderStyle = FAppStyle::Get().GetWidgetStyle<FTableColumnHeaderStyle>("TableView.Header.Column");
		const FTableColumnHeaderStyle TableColumnHeaderStyle = FTableColumnHeaderStyle(CoreTableColumnHeaderStyle);
		const FTableColumnHeaderStyle TableLastColumnHeaderStyle = FTableColumnHeaderStyle(TableColumnHeaderStyle);
		
		const FSplitterStyle TableHeaderSplitterStyle = FSplitterStyle()
				.SetHandleNormalBrush(FSlateColorBrush(FStyleColors::Recessed))
				.SetHandleHighlightBrush(FSlateColorBrush(FStyleColors::Recessed));
		
		Style->Set("TableView.DebuggerHeader", FHeaderRowStyle(CoreHeaderRowStyle)
			.SetColumnStyle(TableColumnHeaderStyle)
			.SetLastColumnStyle(TableLastColumnHeaderStyle)
			.SetColumnSplitterStyle(TableHeaderSplitterStyle)
		    .SetSplitterHandleSize(2.0f)
			.SetHorizontalSeparatorThickness(2.0f)
		);

		FTableRowStyle LineNumberItemStyle;
		LineNumberItemStyle.SetEvenRowBackgroundBrush(FSlateNoResource());
		LineNumberItemStyle.SetOddRowBackgroundBrush(FSlateNoResource());

		Style->Set("LineNumberItemStyle", LineNumberItemStyle);

		FTableRowStyle LineTipItemStyle;
		LineTipItemStyle.SetEvenRowBackgroundBrush(FSlateColorBrush{ FLinearColor::White});
		LineTipItemStyle.SetOddRowBackgroundBrush(FSlateColorBrush{ FLinearColor::White });
		
		Style->Set("LineTipItemStyle", LineTipItemStyle);

		FTableRowStyle KeybindingRowStyle;
		KeybindingRowStyle.SetEvenRowBackgroundBrush(FSlateColorBrush(FStyleColors::Recessed));
		KeybindingRowStyle.SetEvenRowBackgroundHoveredBrush(FSlateColorBrush(FStyleColors::Hover));
		KeybindingRowStyle.SetOddRowBackgroundBrush(FSlateColorBrush(FStyleColors::Header));
		KeybindingRowStyle.SetOddRowBackgroundHoveredBrush(FSlateColorBrush(FStyleColors::Hover));
		KeybindingRowStyle.SetActiveBrush(FSlateColorBrush(FStyleColors::Select));
		KeybindingRowStyle.SetActiveHoveredBrush(FSlateColorBrush(FStyleColors::Select));
		KeybindingRowStyle.SetInactiveBrush(FSlateColorBrush(FStyleColors::SelectInactive));
		KeybindingRowStyle.SetInactiveHoveredBrush(FSlateColorBrush(FStyleColors::SelectInactive));
		KeybindingRowStyle.SetSelectorFocusedBrush(FSlateNoResource());
		Style->Set("Keybinding.Row", KeybindingRowStyle);

        TSharedRef<FCompositeFont> CodeFont = MakeShared<FStandaloneCompositeFont>();
        CodeFont->DefaultTypeface.AppendFont(TEXT("Code"), BaseResourcePath::UE_SlateFontDir / TEXT("DroidSansMono.ttf"), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
        CodeFont->FallbackTypeface.Typeface.AppendFont(TEXT("Code"), BaseResourcePath::UE_SlateFontDir / TEXT("DroidSansFallback.ttf"), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
        FSlateFontInfo CodeFontInfo = FSlateFontInfo(CodeFont, 10);
        Style->Set("CodeFont", CodeFontInfo);

		FTextBlockStyle CodeEditorDefaultTextStyle = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
		CodeEditorDefaultTextStyle.SetHighlightColor(FLinearColor{ 1.0f, 0.53f, 0.0f, .5f });
		CodeEditorDefaultTextStyle.SetSelectedBackgroundColor(FStyleColors::Select);
		Style->Set("CodeEditorDefaultText", CodeEditorDefaultTextStyle);

		FTextBlockStyle CodeEditorNormalTextStyle = FTextBlockStyle{}
			.SetColorAndOpacity(EStyleColor::User12);
		
		FTextBlockStyle CodeEditorKeywordTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(EStyleColor::User1);
		FTextBlockStyle CodeEditorNumberTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(EStyleColor::User2);
		FTextBlockStyle CodeEditorCommentTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(EStyleColor::User3);
		FTextBlockStyle CodeEditorBuiltinFuncTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(EStyleColor::User4);
		FTextBlockStyle CodeEditorBuiltinTypeTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(EStyleColor::User5);
		FTextBlockStyle CodeEditorPreprocessTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(EStyleColor::User6);
		FTextBlockStyle CodeEditorStringTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(EStyleColor::User7);
		FTextBlockStyle CodeEditorParmTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(EStyleColor::User8);
		FTextBlockStyle CodeEditorFuncTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(EStyleColor::User9);
		FTextBlockStyle CodeEditorTypeTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(EStyleColor::User10);
		FTextBlockStyle CodeEditorVarTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(EStyleColor::User11);
		FTextBlockStyle CodeEditorPunctuationTextStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(EStyleColor::User13);
		
		Style->Set("CodeEditorNormalText", CodeEditorNormalTextStyle);
		Style->Set("CodeEditorKeywordText", CodeEditorKeywordTextStyle);
		Style->Set("CodeEditorNumberText", CodeEditorNumberTextStyle);
		Style->Set("CodeEditorCommentText", CodeEditorCommentTextStyle);
		Style->Set("CodeEditorBuiltinFuncText", CodeEditorBuiltinFuncTextStyle);
		Style->Set("CodeEditorBuiltinTypeText", CodeEditorBuiltinTypeTextStyle);
		Style->Set("CodeEditorPreprocessText", CodeEditorPreprocessTextStyle);
		Style->Set("CodeEditorStringText", CodeEditorStringTextStyle);
		Style->Set("CodeEditorPunctuationText", CodeEditorPunctuationTextStyle);
		
		Style->Set("CodeEditorFuncText", CodeEditorFuncTextStyle);
		Style->Set("CodeEditorTypeText", CodeEditorTypeTextStyle);
		Style->Set("CodeEditorParmText", CodeEditorParmTextStyle);
		Style->Set("CodeEditorVarText", CodeEditorVarTextStyle);

		FTextBlockStyle CodeEditorErrorInfoStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FStyleColors::Error);
		Style->Set("CodeEditorErrorInfoText", CodeEditorErrorInfoStyle);
		FTextBlockStyle CodeEditorWarnInfoStyle = FTextBlockStyle{ CodeEditorNormalTextStyle }
			.SetColorAndOpacity(FStyleColors::Warning);
		Style->Set("CodeEditorWarnInfoText", CodeEditorWarnInfoStyle);
		
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
            .SetNormalThumbImage(FSlateColorBrush(FStyleColors::Secondary))
            .SetDraggedThumbImage(FSlateColorBrush(FStyleColors::Hover))
            .SetHoveredThumbImage(FSlateColorBrush(FStyleColors::Hover))
            .SetThickness(5.0f);
		Style->Set("CustomScrollbar", ScrollBar);

		Style->Set("ShaderEditor.Border", new BOX_BRUSH("Common/DebugBorder", 8.0f / 16.0f));
        
		return Style;
	}

}

#undef TTF_FONT
#undef RootToContentDir
