#include "CommonHeader.h"
#include "FAppCommonStyle.h"
#include <Styling/SlateStyleRegistry.h>
#include <Styling/StyleColors.h>
#include "Common/Path/BaseResourcePath.h"
#include <Styling/SlateStyleMacros.h>
#include <Styling/StarshipCoreStyle.h>

#define RootToContentDir Style->RootToContentDir
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo(RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )

namespace FW
{
    const ISlateStyle& FAppCommonStyle::Get()
    {
        if (!Instance.IsValid()) {
            Instance = Create();
            FSlateStyleRegistry::RegisterSlateStyle(Get());
        }
        return *Instance;
    }

	TSharedRef<ISlateStyle> FAppCommonStyle::Create()
	{
		TSharedRef<FSlateStyleSet> Style = MakeShared<FSlateStyleSet>("AppCommonStyle");
		Style->SetContentRoot(BaseResourcePath::Custom_SlateResourceDir);
		Style->Set("PropertyView.RowIndentDropShadow", new IMAGE_BRUSH("DropShadow", FVector2D(2, 2)));
		Style->Set("MessageDialog.Boqi", new IMAGE_BRUSH("Boqi", FVector2D(32.0, 32.0)));
		Style->Set("MessageDialog.Boqi2", new IMAGE_BRUSH_SVG("Boqi2", FVector2D(32.0, 32.0)));
		Style->Set("MessageDialog.Boqi3", new IMAGE_BRUSH("Boqi3", FVector2D(32.0, 32.0)));
		Style->Set("AssetBrowser.Folder", new IMAGE_BRUSH_SVG("folder", FVector2D(64.0, 64.0)));
		Style->Set("ProjectLauncher.Background", new IMAGE_BRUSH("Launcher", FVector2D(560, 270)));
		Style->Set("ProjectLauncher.Logo", new IMAGE_BRUSH("ShaderHelper", FVector2D(100, 30)));
        
        Style->Set("Timeline.LeftEnd", new IMAGE_BRUSH_SVG("LeftEnd", FVector2D(24.0, 24.0)));
        Style->Set("Timeline.Pause", new IMAGE_BRUSH_SVG("Pause", FVector2D(24.0, 24.0)));
        Style->Set("Timeline.Play", new IMAGE_BRUSH_SVG("Play", FVector2D(24.0, 24.0)));
        Style->Set("Timeline.RightEnd", new IMAGE_BRUSH_SVG("RightEnd", FVector2D(24.0, 24.0)));

        Style->Set("Graph.Shadow", new BOX_BRUSH( "WindowBorder", 0.48f ) );
		Style->Set("Graph.NodeShadow", new BOX_BRUSH("NodeShadow", FMargin(18.0f / 64.0f)));
		Style->Set("Graph.NodeTitleBackground", new FSlateRoundedBoxBrush(FStyleColors::White, FVector4(5.0, 5.0, 0, 0)));
		Style->Set("Graph.NodeContentBackground", new FSlateRoundedBoxBrush(FStyleColors::Panel, FVector4(0, 0, 5.0, 5.0)));
		Style->Set("Graph.NodeOutline", new FSlateRoundedBoxBrush(FStyleColors::White, 5.0f));
		Style->Set("Effect.Toggle", new IMAGE_BRUSH("Toggle", FVector2D(16.0, 16.0)));

		Style->SetContentRoot(BaseResourcePath::UE_SlateResourceDir);
		//StarshipCoreStyle is used as the app style.
		const FEditableTextBoxStyle& NormalEditableTextBoxStyle =  FAppStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
		const FTextBlockStyle& NormalText = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
		
		Style->Set("Log.TextBox", FEditableTextBoxStyle(NormalEditableTextBoxStyle)
			.SetBackgroundImageNormal(BOX_BRUSH("Common/WhiteGroupBorder", FMargin(4.0f / 16.0f)))
			.SetBackgroundImageHovered(BOX_BRUSH("Common/WhiteGroupBorder", FMargin(4.0f / 16.0f)))
			.SetBackgroundImageFocused(BOX_BRUSH("Common/WhiteGroupBorder", FMargin(4.0f / 16.0f)))
			.SetBackgroundImageReadOnly(BOX_BRUSH("Common/WhiteGroupBorder", FMargin(4.0f / 16.0f)))
			.SetBackgroundColor(FStyleColors::Recessed)
		);

		
		const int32 LogFontSize =  9;
		const FTextBlockStyle NormalLogText = FTextBlockStyle(NormalText)
			.SetFont(TTF_FONT(TEXT("Fonts/DroidSansMono"), LogFontSize))
			.SetColorAndOpacity(FStyleColors::White)
			.SetSelectedBackgroundColor(FStyleColors::Select)
			.SetHighlightColor(FStyleColors::Highlight);

		Style->Set("Log.Normal", NormalLogText);

		Style->Set("Log.Warning", FTextBlockStyle(NormalLogText)
			.SetColorAndOpacity(FStyleColors::Warning)
		);

		Style->Set("Log.Error", FTextBlockStyle(NormalLogText)
			.SetColorAndOpacity(FStyleColors::Error)
		);

		Style->Set("PropertyView.CategoryColor", new FSlateColorBrush(FStyleColors::Panel));
		Style->Set("PropertyView.ItemColor", new FSlateColorBrush(FStyleColors::Recessed));
		Style->Set("PropertyView.CompositeItemColor", new FSlateColorBrush(FStyleColors::Dropdown));

		const FTableRowStyle DirectoryTableRowStyle =
			FTableRowStyle(FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("SimpleTableView.Row"))
			.SetSelectedTextColor(FStyleColors::Foreground);
		Style->Set("DirectoryTreeView.Row", DirectoryTableRowStyle);

		const FTableRowStyle AssetViewTableRowStyle =
			FTableRowStyle(FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row"))
			.SetEvenRowBackgroundHoveredBrush(FSlateNoResource())
			.SetSelectorFocusedBrush(FSlateNoResource())
			.SetOddRowBackgroundHoveredBrush(FSlateNoResource());
		Style->Set("AssetView.Row", AssetViewTableRowStyle);

		Style->Set("Icons.Folder", new IMAGE_BRUSH_SVG("Starship/Common/folder-closed", FVector2D(16.0, 16.0)));
		Style->Set("Icons.FolderPlus", new IMAGE_BRUSH_SVG("Starship/Common/folder-plus", FVector2D(16.0, 16.0)));
		//Style->Set("Icons.Graph", new IMAGE_BRUSH_SVG("Starship/Insights/Callers_20", FVector2D(20.0, 20.0)));
		Style->Set("Icons.File", new IMAGE_BRUSH_SVG("Starship/Common/file", FVector2D(16.0, 16.0)));
        Style->Set("Icons.Log", new IMAGE_BRUSH_SVG("Starship/Insights/Log", FVector2D(16.0, 16.0)));
		Style->Set("CommonCommands.Save", new IMAGE_BRUSH_SVG("Starship/Common/save", FVector2D(16.0, 16.0)));

		FButtonStyle CloseButton = FButtonStyle()
			.SetNormal(IMAGE_BRUSH_SVG("Starship/Common/close-small", FVector2D(30.0, 30.0), FLinearColor{0.7f,0.7f,0.7f}))
			.SetPressed(IMAGE_BRUSH_SVG("Starship/Common/close-small", FVector2D(30.0, 30.0), FLinearColor{ 0.7f,0.7f,0.7f }))
			.SetHovered(IMAGE_BRUSH_SVG("Starship/Common/close-small", FVector2D(30.0, 30.0)));
		Style->Set("CloseButton", CloseButton);
        
        FButtonStyle SuperSimpleButton = FButtonStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
            .SetPressed(FSlateNoResource())
            .SetHovered(FSlateNoResource());
        Style->Set("SuperSimpleButton", SuperSimpleButton);
		
		Style->Set("Graph.Selector", new BORDER_BRUSH("Common/Selector", FMargin(6.0f / 32.0f)));
        
        FTextBlockStyle MinorText = FTextBlockStyle{}
            .SetFont(TTF_FONT(TEXT("Fonts/DroidSansMono"), 8))
            .SetColorAndOpacity(FLinearColor{0.4f,0.4f,0.4f,1.0f});
        FTextBlockStyle SmallMinorText = FTextBlockStyle{MinorText}
            .SetFontSize(7);
        Style->Set("MinorText", MinorText);
        Style->Set("SmallMinorText", SmallMinorText);
		return Style;
	}

}

#undef RootToContentDir
