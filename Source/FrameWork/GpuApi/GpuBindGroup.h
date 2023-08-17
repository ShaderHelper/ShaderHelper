#pragma once
#include "GpuResourceCommon.h"
#include "GpuTexture.h"
#include "GpuBuffer.h"

namespace FRAMEWORK
{


	struct GpuBindGroupDesc
	{
		//struct TextureBinding
		//{
		//	FString BindingName;
		//	GpuTexture Texture;
		//};

		struct BufferBinding
		{
			FString BindingName;
			GpuBuffer* Buffer;
		};

		//TArray<TextureBinding> Textures;
		TArray<BufferBinding> Buffers;
	};
	
	class GpuBindGroup : public GpuResource
	{

	};
}