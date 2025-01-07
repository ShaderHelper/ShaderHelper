#pragma once
#include "AssetObject/AssetObject.h"
#include "RenderResource/UniformBuffer.h"

namespace SH
{
	class StShader : public FW::AssetObject
	{
		REFLECTION_TYPE(StShader)
	public:
		StShader();
		~StShader();

	public:
		void Serialize(FArchive& Ar) override;
		FString FileExtension() const override;
		const FSlateBrush* GetImage() const override;

        FString GetResourceDeclaration() const;
		FString GetFullShader() const;

	public:
		FString PixelShaderBody;
        TUniquePtr<FW::UniformBuffer> BuiltInUniformBuffer;
        
        TRefCountPtr<FW::GpuBindGroup> BuiltInBindGroup;
        TRefCountPtr<FW::GpuBindGroupLayout> BuiltInBindGroupLayout;

        TRefCountPtr<FW::GpuBindGroup> CustomBindGroup;
        TRefCountPtr<FW::GpuBindGroupLayout> CustomBindGroupLayout;
	};

}
