#include "CommonHeaderForUE.h"
#include "FShaderHelperStyle.h"
#include "Misc/CommonPath/BaseResourcePath.h"
#include <Styling/SlateStyleRegistry.h>
#include <Styling/StyleColors.h>
#include <Styling/SlateStyleMacros.h>

#define RootToContentDir Style->RootToContentDir

namespace SH {

	void FShaderHelperStyle::Init()
	{
		if (!Instance.IsValid()) {
			FSlateStyleRegistry::RegisterSlateStyle(*Create());
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
		Style->SetContentRoot(SH::BaseResourcePath::UE_SlateResourceDir / TEXT("Slate/Custom") );

		//Style->Set("ShaderHelper.Logo", new IMAGE_BRUSH("ShaderHelperLogo",FVector2D(24,24), FStyleColors::White));
		
		return Style;
	}

}
