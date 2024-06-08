#include "CommonHeader.h"
#include "ShaderPass.h"
#include "UI/Styles/FShaderHelperStyle.h"

using namespace FRAMEWORK;

namespace SH
{
const FString DefaultPixelShaderInput =
R"(
struct PIn
{
    float4 Pos : SV_Position;
};
)";

const FString DefaultPixelShaderMacro =
R"(
#define fragCoord float2(Input.Pos.x, iResolution.y - Input.Pos.y)
)";

const FString DefaultPixelShaderBody =
R"(float4 MainPS(PIn Input) : SV_Target
{
    float2 uv = fragCoord/iResolution.xy;
    float3 col = 0.5 + 0.5*cos(iTime + uv.xyx + float3(0,2,4));
    return float4(col,1);
})";

	GLOBAL_REFLECTION_REGISTER(AddClass<ShaderPass>()
                                .BaseClass<AssetObject>()
	)

	ShaderPass::ShaderPass()
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

	void ShaderPass::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);

		Ar << PixelShaderBody;
	}

	FString ShaderPass::FileExtension() const
	{
		return "shaderpass";
	}

	const FSlateBrush* ShaderPass::GetImage() const
	{
		return FShaderHelperStyle::Get().GetBrush("AssetBrowser.ShaderPass");
	}

    FString ShaderPass::GetResourceDeclaration() const
    {
        FString Declaration = BuiltInBindGroupLayout->GetCodegenDeclaration();
        if (CustomBindGroupLayout)
        {
            Declaration += CustomBindGroupLayout->GetCodegenDeclaration();
        }
        return DefaultPixelShaderInput + Declaration + DefaultPixelShaderMacro;
    }

}
