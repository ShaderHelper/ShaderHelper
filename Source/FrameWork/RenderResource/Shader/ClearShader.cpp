#include "CommonHeader.h"
#include "ClearShader.h"
#include "Common/Path/PathHelper.h"

namespace FW
{
	template<BindingType InType>
	ClearShader<InType>::ClearShader(const std::set<FString>& VariantDefinitions)
	{
		TArray<FString> ExtraArgs;
		ExtraArgs.Add("-D");
		if constexpr(InType == BindingType::RWStorageBuffer)
		{
			ExtraArgs.Add("RESOURCE_STORAGE_BUFFER");
		}

		ClearUbBuilder.AddUint("Size");

		BindGroupLayout = GpuBindGroupLayoutBuilder{ BindingContext::ShaderSlot }
			.AddExistingBinding(0, InType, BindingShaderStage::Compute)
			.AddUniformBuffer("ClearUb", ClearUbBuilder.GetLayoutDeclaration(), BindingShaderStage::Compute)
			.Build();

		Cs = GGpuRhi->CreateShaderFromFile({
			.FileName = PathHelper::ShaderDir() / "Clear.hlsl",
			.Type = ShaderType::ComputeShader,
			.EntryPoint = "ClearCS",
			.ExtraDecl = BindGroupLayout->GetCodegenDeclaration()
		});

		FString ErrorInfo, WarnInfo;
		GGpuRhi->CompileShader(Cs, ErrorInfo, WarnInfo, ExtraArgs);
		check(ErrorInfo.IsEmpty());
	}

	template<BindingType InType>
	TRefCountPtr<GpuBindGroup> ClearShader<InType>::GetBindGroup(GpuResource* InResource, uint32 ResourceByteSize)
	{
		TUniquePtr<UniformBuffer> ClearUb = ClearUbBuilder.Build();
		check(ResourceByteSize % 4 == 0);
		ClearUb->GetMember<uint32>("Size") = ResourceByteSize / 4;

		return GpuBindGroupBuilder{ BindGroupLayout }
			.SetExistingBinding(0, InResource)
			.SetUniformBuffer("ClearUb", ClearUb->GetGpuResource())
			.Build();
	}

	template class ClearShader<BindingType::RWStorageBuffer>;
}
