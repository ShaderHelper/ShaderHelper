#include "CommonHeader.h"
#include "FAppCommonStyle.h"
#include <Styling/SlateStyleRegistry.h>
#include <Styling/StyleColors.h>
#include "Common/Path/BaseResourcePath.h"
#include <Styling/SlateStyleMacros.h>

#define RootToContentDir Style->RootToContentDir

namespace FRAMEWORK
{
	
	void FAppCommonStyle::Init()
	{
		if (!Instance.IsValid()) {
			FSlateStyleRegistry::RegisterSlateStyle(*Create());
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

		Style->SetContentRoot(BaseResourcePath::UE_SlateResourceDir / TEXT("Slate"));

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
			.SetFont(DEFAULT_FONT("Mono", LogFontSize))
			.SetColorAndOpacity(EStyleColor::User3)
			.SetSelectedBackgroundColor(EStyleColor::User2)
			.SetHighlightColor(FStyleColors::Black);

		Style->Set("Log.Normal", NormalLogText);

		return Style;
	}

}