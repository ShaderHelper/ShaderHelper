#pragma once
#include "AssetObject/AssetObject.h"
#include "RenderResource/UniformBuffer.h"

namespace SH
{
	class StShader : public FRAMEWORK::AssetObject
	{
		REFLECTION_TYPE(StShader)
	public:
		StShader();

	public:
		void Serialize(FArchive& Ar) override;
		FString FileExtension() const override;
		const FSlateBrush* GetImage() const override;
        const FString& GetPixelShaderBody() const { return PixelShaderBody; }
        FString GetResourceDeclaration() const;

	private:
		FString PixelShaderBody;
        TUniquePtr<FRAMEWORK::UniformBuffer> BuiltInUniformBuffer;
        
        TRefCountPtr<FRAMEWORK::GpuBindGroup> BuiltInBindGroup;
        TRefCountPtr<FRAMEWORK::GpuBindGroupLayout> BuiltInBindGroupLayout;

        TRefCountPtr<FRAMEWORK::GpuBindGroup> CustomBindGroup;
        TRefCountPtr<FRAMEWORK::GpuBindGroupLayout> CustomBindGroupLayout;
	};

}
