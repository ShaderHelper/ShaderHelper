#include "CommonHeader.h"
#include "GpuBindGroupLayout.h"
#include "GpuRhi.h"
#include "RenderResource/UniformBuffer.h"

namespace FW
{
	
	GpuBindGroupLayoutBuilder::GpuBindGroupLayoutBuilder(BindingGroupSlot InGroupSlot)
		: AutoSlot(0)
	{
		check(InGroupSlot < GpuResourceLimit::MaxBindableBingGroupNum);
		LayoutDesc.GroupNumber = InGroupSlot;
	}

	GpuBindGroupLayoutBuilder& GpuBindGroupLayoutBuilder::AddExistingBinding(BindingSlot InSlot, BindingType Type, BindingShaderStage InStage)
	{
		checkf(!LayoutDesc.Layouts.Contains(InSlot), *FString::Printf(TEXT("Slot:%d already existed."), InSlot));
		LayoutDesc.Layouts.Add(InSlot, { Type, InStage });
		return *this;
	}

	GpuBindGroupLayoutBuilder& GpuBindGroupLayoutBuilder::AddUniformBuffer(const FString& BindingName, const UniformBufferBuilder& UbBuilder, BindingShaderStage InStage)
	{
		while (LayoutDesc.Layouts.Contains(AutoSlot)) { AutoSlot++; };

		LayoutDesc.HlslCodegenDeclaration += FString::Format(*UbBuilder.GetLayoutDeclaration(GpuShaderLanguage::HLSL), {BindingName, AutoSlot, LayoutDesc.GroupNumber});
		LayoutDesc.GlslCodegenDeclaration += FString::Format(*UbBuilder.GetLayoutDeclaration(GpuShaderLanguage::GLSL), { BindingName, AutoSlot, LayoutDesc.GroupNumber });
		LayoutDesc.CodegenBindingNameToSlot.Add(BindingName, AutoSlot);
		LayoutDesc.Layouts.Add(AutoSlot++, {BindingType::UniformBuffer, InStage });

		return *this;
	}

	GpuBindGroupLayoutBuilder& GpuBindGroupLayoutBuilder::AddTexture(const FString& BindingName, BindingShaderStage InStage)
	{
		while (LayoutDesc.Layouts.Contains(AutoSlot)) { AutoSlot++; };
		LayoutDesc.HlslCodegenDeclaration += FString::Printf(TEXT("Texture2D %s : register(t%d, space%d);\n"), *BindingName, AutoSlot, LayoutDesc.GroupNumber);
		LayoutDesc.GlslCodegenDeclaration += FString::Printf(TEXT("layout(binding = %d, set = %d) uniform sampler2D %s;\n"), AutoSlot, LayoutDesc.GroupNumber,*BindingName);
		LayoutDesc.CodegenBindingNameToSlot.Add(BindingName, AutoSlot);
		LayoutDesc.Layouts.Add(AutoSlot++, {BindingType::Texture, InStage });
		return *this;
	}

	GpuBindGroupLayoutBuilder& GpuBindGroupLayoutBuilder::AddSampler(const FString& BindingName, BindingShaderStage InStage)
	{
		while (LayoutDesc.Layouts.Contains(AutoSlot)) { AutoSlot++; };
		LayoutDesc.HlslCodegenDeclaration += FString::Printf(TEXT("SamplerState %s : register(s%d, space%d);\n"), *BindingName, AutoSlot, LayoutDesc.GroupNumber);
		LayoutDesc.CodegenBindingNameToSlot.Add(BindingName, AutoSlot);
		LayoutDesc.Layouts.Add(AutoSlot++, {BindingType::Sampler, InStage });
		return *this;
	}

	TRefCountPtr<GpuBindGroupLayout> GpuBindGroupLayoutBuilder::Build() const
	{
		return GGpuRhi->CreateBindGroupLayout(LayoutDesc);
	}

	const FString& GpuBindGroupLayout::GetCodegenDeclaration(GpuShaderLanguage Language) const
	{
		return Language == GpuShaderLanguage::HLSL ? Desc.HlslCodegenDeclaration : Desc.GlslCodegenDeclaration;
	}

	const FString& GpuBindGroupLayoutBuilder::GetCodegenDeclaration(GpuShaderLanguage Language) const
	{
		return Language == GpuShaderLanguage::HLSL ? LayoutDesc.HlslCodegenDeclaration : LayoutDesc.GlslCodegenDeclaration;
	}

}



