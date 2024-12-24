#include "CommonHeader.h"
#include "StShader.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "Common/Path/PathHelper.h"

using namespace FRAMEWORK;

namespace SH
{
const FString DefaultPixelShaderBody =
R"(void mainImage(out float4 fragColor, in float2 fragCoord)
{
    float2 uv = fragCoord/iResolution.xy;
    float3 col = 0.5 + 0.5*cos(iTime + uv.xyx + float3(0,2,4));
    fragColor = float4(col,1);
})";

	GLOBAL_REFLECTION_REGISTER(AddClass<StShader>()
                                .BaseClass<AssetObject>()
	)

	StShader::StShader()
	{
        PixelShaderBody = DefaultPixelShaderBody;
        
        BuiltInUniformBuffer = UniformBufferBuilder{ UniformBufferUsage::Persistant }
                                .AddVector2f("iResolution")
                                .AddFloat("iTime")
                                .Build();

        BuiltInBindGroupLayout = GpuBindGroupLayoutBuilder{ 0 }
                                .AddUniformBuffer("BuiltIn", BuiltInUniformBuffer->GetDeclaration(), BindingShaderStage::Pixel)
                                .Build();

        BuiltInBindGroup = GpuBindGrouprBuilder{ BuiltInBindGroupLayout }
                            .SetUniformBuffer("BuiltIn", BuiltInUniformBuffer->GetGpuResource())
                            .Build();
	}

	void StShader::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);

		Ar << PixelShaderBody;
	}

	FString StShader::FileExtension() const
	{
		return "stShader";
	}

	const FSlateBrush* StShader::GetImage() const
	{
		return FShaderHelperStyle::Get().GetBrush("AssetBrowser.Shader");
	}

    FString StShader::GetResourceDeclaration() const
    {
		FString Template;
		FFileHelper::LoadFileToString(Template, *(PathHelper::ShaderDir() / "ShaderHelper/StShaderTemplate.hlsl"));
		return BuiltInBindGroupLayout->GetCodegenDeclaration() + Template;
    }

}
