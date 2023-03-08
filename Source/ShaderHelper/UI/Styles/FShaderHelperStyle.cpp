#include "CommonHeader.h"
#include "FShaderHelperStyle.h"
#include "Common/Path/BaseResourcePath.h"
#include <Styling/SlateStyleRegistry.h>
#include <Styling/StyleColors.h>
#include <Styling/SlateStyleMacros.h>

#define RootToContentDir Style->RootToContentDir

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
		Style->SetContentRoot(BaseResourcePath::UE_SlateResourceDir / TEXT("Slate/Custom") );

		//Style->Set("ShaderHelper.Logo", new IMAGE_BRUSH("ShaderHelperLogo",FVector2D(24,24), FStyleColors::White));
		
		return Style;
	}

}

#undef RootToContentDir