#pragma once
#include "GpuResourceCommon.h"

namespace FRAMEWORK
{
	struct GpuBindGroupDesc
	{
		struct TextureBinding
		{
			FString BindingName;
			TRefCountPtr<GpuTexture> Texture;
		};

		struct BufferBinding
		{
			FString BindingName;
			TRefCountPtr<GpuBuffer> Buffer;
		};

		TArray<TextureBinding> Textures;
		TArray<BufferBinding> Buffers;
	};
	
	class GpuBindGroup : public GpuResource
	{
		
	};


	class GpuBindGroupDescBuilder
	{
	public:
		GpuBindGroupDescBuilder& AddTexture(const FString& BindingName, TRefCountPtr<GpuTexture> Texture)
		{
			Desc.Textures.Add({ BindingName, MoveTemp(Texture) });
			return *this;
		}

		GpuBindGroupDescBuilder& AddBuffer(const FString& BindingName, TRefCountPtr<GpuBuffer> Buffer)
		{
			Desc.Buffers.Add({ BindingName, MoveTemp(Buffer) });
			return *this;
		}

		operator GpuBindGroupDesc() const
		{
			return Desc;
		}
		
	private:
		GpuBindGroupDesc Desc;
	};


}