#include "CommonHeader.h"
#include "ClearShader.h"
#include "Common/Path/PathHelper.h"

namespace FW
{
	template<BindingType InType>
	ClearShader<InType>::ClearShader()
	{
		TArray<FString> Definitions;
		if constexpr(InType == BindingType::RWStorageBuffer)
		{
			Definitions.Add("RESOURCE_TYPE==0");
		}

		ClearUbBuilder.AddUint("Size");

		Cs = GGpuRhi->CreateShaderFromFile(
			PathHelper::ShaderDir() / "Clear.hlsl",
			ShaderType::ComputeShader,
			"ClearCS"
		);

		FString ErrorInfo;
		GGpuRhi->CompileShader(Cs, ErrorInfo, Definitions);
		check(ErrorInfo.IsEmpty());

		BindGroupLayout = GpuBindGroupLayoutBuilder{ BindingContext::ShaderSlot }
			.AddExistingBinding(0, InType, BindingShaderStage::Compute)
			.AddUniformBuffer("ClearUb", ClearUbBuilder.GetLayoutDeclaration(), BindingShaderStage::Compute)
			.Build();
	}

	template<BindingType InType>
	TRefCountPtr<GpuBindGroup> ClearShader<InType>::GetBindGroup(GpuResource* InResource, uint32 ResourceByteSize)
	{
		TUniquePtr<UniformBuffer> ClearUb = ClearUbBuilder.Build();
		check(ResourceByteSize % 4 == 0);
		ClearUb->GetMember<uint32>("Size") = ResourceByteSize / 4;

		return GpuBindGrouprBuilder{ BindGroupLayout }
			.SetExistingBinding(0, InResource)
			.SetUniformBuffer("ClearUb", ClearUb->GetGpuResource())
			.Build();
	}

	template ClearShader<BindingType::RWStorageBuffer>;
}