#include "CommonHeader.h"
#include "FAppCommonStyle.h"
#include <Styling/SlateStyleRegistry.h>
#include <Styling/StyleColors.h>
#include "Common/Path/BaseResourcePath.h"
#include <Styling/SlateStyleMacros.h>
#include <Styling/StarshipCoreStyle.h>

#define RootToContentDir Style->RootToContentDir
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo(RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )

namespace FRAMEWORK
{
	
	void FAppCommonStyle::Init()
	{
		if (!Instance.IsValid()) {
			Instance = Create();
			FSlateStyleRegistry::RegisterSlateStyle(Get());
		}
	}

	void FAppCommonStyle::ShutDown()
	{
		if (Instance.IsValid()) {
			FSlateStyleRegistry::UnRegisterSlateStyle(Get());
			Instance.Reset();
		}
	}

	TSharedRef<ISlateStyle> FAppCommonStyle::Create()
	{
		TSharedRef<FSlateStyleSet> Style = MakeShared<FSlateStyleSet>("AppCommonStyle");
		Style->SetContentRoot(BaseResourcePath::Custom_SlateResourceDir);
		Style->Set("PropertyView.RowIndentDropShadow", new IMAGE_BRUSH("DropShadow", FVector2D(2, 2)));
		Style->Set("MessageDialog.Boqi", new IMAGE_BRUSH("Boqi", FVector2D(32.0, 32.0)));
		Style->Set("MessageDialog.Boqi2", new IMAGE_BRUSH("Boqi2", FVector2D(32.0, 32.0)));
		Style->Set("AssetBrowser.Folder", new IMAGE_BRUSH_SVG("folder", FVector2D(64.0, 64.0)));

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

		Style->Set("PropertyView.CategoryColor", new FSlateColorBrush(FLinearColor{ 0.07f, 0.07f, 0.07f, 1.0f }));
		Style->Set("PropertyView.ItemColor", new FSlateColorBrush(FLinearColor{ 0.04f, 0.04f, 0.04f, 1.0f }));
		Style->Set("PropertyView.ItemHoverdColor", new FSlateColorBrush(FLinearColor{ 0.05f, 0.05f, 0.05f, 1.0f }));

		const FTableRowStyle DirectoryTableRowStyle =
			FTableRowStyle(FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("SimpleTableView.Row"))
			.SetSelectedTextColor(FStyleColors::Foreground);
		Style->Set("DirectoryTreeView.Row", DirectoryTableRowStyle);

		const FTableRowStyle AssetViewTableRowStyle =
			FTableRowStyle(FAppStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row"))
			.SetEvenRowBackgroundHoveredBrush(FSlateNoResource())
			.SetOddRowBackgroundHoveredBrush(FSlateNoResource());
		Style->Set("AssetView.Row", AssetViewTableRowStyle);

		Style->Set("Icons.FolderPlus", new IMAGE_BRUSH_SVG("Starship/Common/folder-plus", FVector2D(16.0, 16.0)));

		return Style;
	}

}

#undef RootToContentDir