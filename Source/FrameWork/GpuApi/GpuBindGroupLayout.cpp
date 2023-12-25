#include "CommonHeader.h"
#include "GpuBindGroupLayout.h"
#include "GpuApiInterface.h"

namespace FRAMEWORK
{
	
	GpuBindGroupLayoutBuilder::GpuBindGroupLayoutBuilder(BindingGroupSlot InGroupSlot)
		: AutoSlot(0)
	{
		check(InGroupSlot < GpuResourceLimit::MaxBindableBingGroupNum);
		LayoutDesc.GroupNumber = InGroupSlot;
	}

	GpuBindGroupLayoutBuilder& GpuBindGroupLayoutBuilder::AddExistingBinding(BindingSlot InSlot, BindingType ResourceType, BindingShaderStage InStage)
	{
		checkf(LayoutDesc.Layouts.Contains(InSlot), *FString::Printf(TEXT("Slot:%d already existed."), InSlot));
		LayoutDesc.Layouts.Add(InSlot, {ResourceType, InStage });
		return *this;
	}

	GpuBindGroupLayoutBuilder& GpuBindGroupLayoutBuilder::AddUniformBuffer(const FString& BindingName, const FString& UniformBufferLayoutDeclaration, BindingShaderStage InStage)
	{

		FString FinalUniformBufferDeclaration = FString::Format(*UniformBufferLayoutDeclaration, { BindingName });
		int32 InsertIndex = FinalUniformBufferDeclaration.Find(TEXT("{"));
		check(InsertIndex != INDEX_NONE);
		while (LayoutDesc.Layouts.Contains(AutoSlot)) { AutoSlot++; };
		FinalUniformBufferDeclaration.InsertAt(InsertIndex - 1, FString::Printf(TEXT(": register(b%d, space%d)\r\n"), AutoSlot, LayoutDesc.GroupNumber));

		LayoutDesc.CodegenDeclaration += MoveTemp(FinalUniformBufferDeclaration);
		LayoutDesc.CodegenBindingNameToSlot.Add(BindingName, AutoSlot);
		LayoutDesc.Layouts.Add(AutoSlot++, {BindingType::UniformBuffer, InStage });

		return *this;
	}

	GpuBindGroupLayoutBuilder& GpuBindGroupLayoutBuilder::AddTexture(const FString& BindingName, BindingShaderStage InStage)
	{
		while (LayoutDesc.Layouts.Contains(AutoSlot)) { AutoSlot++; };
		LayoutDesc.CodegenDeclaration += FString::Printf(TEXT("Texture2D %s : register(t%d, space%d);\r\n"), *BindingName, AutoSlot, LayoutDesc.GroupNumber);
		LayoutDesc.CodegenBindingNameToSlot.Add(BindingName, AutoSlot);
		LayoutDesc.Layouts.Add(AutoSlot++, {BindingType::Texture, InStage });
		return *this;
	}

	GpuBindGroupLayoutBuilder& GpuBindGroupLayoutBuilder::AddSampler(const FString& BindingName, BindingShaderStage InStage)
	{
		while (LayoutDesc.Layouts.Contains(AutoSlot)) { AutoSlot++; };
		LayoutDesc.CodegenDeclaration += FString::Printf(TEXT("SamplerState %s : register(s%d, space%d);\r\n"), *BindingName, AutoSlot, LayoutDesc.GroupNumber);
		LayoutDesc.CodegenBindingNameToSlot.Add(BindingName, AutoSlot);
		LayoutDesc.Layouts.Add(AutoSlot++, {BindingType::Sampler, InStage });
		return *this;
	}

	TRefCountPtr<GpuBindGroupLayout> GpuBindGroupLayoutBuilder::Build()
	{
		return GpuApi::CreateBindGroupLayout(LayoutDesc);
	}

}



