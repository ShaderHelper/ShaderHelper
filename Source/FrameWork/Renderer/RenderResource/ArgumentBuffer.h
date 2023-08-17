#pragma once
#include "UniformBuffer.h"

namespace FRAMEWORK
{
	struct ArgumentBufferLayout
	{
		FString Declaration;
		TRefCountPtr<GpuBindGroupLayout> BindGroupLayout;
	};

	struct ArgumentBuffer
	{
		TArray<TSharedPtr<UniformBuffer>> UniformBuffers;
		TRefCountPtr<GpuBindGroup> BindGroup;
		//TArray<> textures;

	/*	TRefCountPtr<GpuBindGroup> CreateBindGroup() const
		{
			GpuBindGroupDesc Desc;
			for (auto& Ub : UniformBuffers)
			{
				auto& UbMetaData = Ub->GetMetaData();
				Desc.Buffers.Emplace()
			}

			return GpuApi::CreateBindGroup(Desc);
		}*/
	};

	class ArgumentBufferBuilder
	{
	public:
		ArgumentBufferBuilder(int32 InBindGroupSlot)
		{
			check(InBindGroupSlot < GpuResourceLimit::MaxBindableBingGroupNum);
			BindGroupSlot = InBindGroupSlot;
		}
		
	public:

		ArgumentBufferBuilder& AddUniformBuffer(TSharedPtr<UniformBuffer> InUniformBuffer)
		{
			FString UniformBufferDeclaration = InUniformBuffer->GetMetaData().UniformBufferDeclaration;
			int32 InsertIndex = UniformBufferDeclaration.Find(TEXT("{"));
			check(InsertIndex != INDEX_NONE);
			UniformBufferDeclaration.InsertAt(InsertIndex - 1, FString::Printf(TEXT(": register(space%d)\r\n"), BindGroupSlot));

			Buffer.ArgumentBufferDeclaration += MoveTemp(UniformBufferDeclaration);
			Buffer.UniformBuffers.Add(InUniformBuffer);
			return *this;
		}

		operator TUniquePtr<ArgumentBuffer>() const
		{
			return MakeUnique<ArgumentBuffer>(Buffer);
		}

		//ArgumentBufferBuilder& AddTexture() 
		
	private:
		int32 BindGroupSlot;
		ArgumentBuffer Buffer;
	};
}