#include "CommonHeader.h"
#include "ShaderPass.h"
#include "UI/Styles/FShaderHelperStyle.h"

using namespace FRAMEWORK;

namespace SH
{
	GLOBAL_REFLECTION_REGISTER(
		ShReflectToy::AddClass<ShaderPass>()
						.BaseClass<AssetObject>();
	)

	ShaderPass::ShaderPass()
	{
        PixelShaderBody =  R"(float4 MainPS(PIn Input) : SV_Target
                        {
                            float2 uv = fragCoord/iResolution.xy;
                            float3 col = 0.5 + 0.5*cos(iTime + uv.xyx + float3(0,2,4));
                            return float4(col,1);
                        })";
	}

	void ShaderPass::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);

		Ar << PixelShaderBody;
	}

	FString ShaderPass::FileExtension() const
	{
		return "ShaderPass";
	}

	const FSlateBrush* ShaderPass::GetImage() const
	{
		return FShaderHelperStyle::Get().GetBrush("AssetBrowser.ShaderPass");
	}

}
