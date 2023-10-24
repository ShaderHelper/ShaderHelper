#pragma once
#include "UniformBuffer.h"

namespace FRAMEWORK
{
	class ArgumentBufferLayout
	{
		friend ArgumentBufferBuilder;
	public:
		GpuBindGroupLayout* GetBindLayout() const { return BindGroupLayout; }

	private:
		FString Declaration;
		GpuBindGroupLayoutDesc LayoutDesc;
		TRefCountPtr<GpuBindGroupLayout> BindGroupLayout;
	};

	class ArgumentBuffer
	{
		friend ArgumentBufferBuilder;
	public:
		GpuBindGroup* GetBindGroup() const { return BindGroup; }

	private:
		TArray<TSharedPtr<UniformBuffer>> UniformBuffers;
		GpuBindGroupDesc Desc;
		TRefCountPtr<GpuBindGroup> BindGroup;
	};

	class ArgumentBufferBuilder
	{
	public:
		ArgumentBufferBuilder(BindingGroupIndex InIndex)
		{
			check(InIndex < GpuResourceLimit::MaxBindableBingGroupNum);
			Index = InIndex;
		}
		
	public:
		ArgumentBufferBuilder& AddUniformBuffer(TSharedPtr<UniformBuffer> InUniformBuffer, BindingShaderStage InStage = BindingShaderStage::All)
		{
			Buffer.UniformBuffers.Add(InUniformBuffer);
			Buffer.Desc.Resources.Add({ BindingNum, InUniformBuffer->GetGpuResource() });

			FString UniformBufferDeclaration = InUniformBuffer->GetMetaData().UniformBufferDeclaration;
			int32 InsertIndex = UniformBufferDeclaration.Find(TEXT("{"));
			check(InsertIndex != INDEX_NONE);
			UniformBufferDeclaration.InsertAt(InsertIndex - 1, FString::Printf(TEXT(": register(b%d, space%d)\r\n"), BindingNum, Index));

			BufferLayout.Declaration += MoveTemp(UniformBufferDeclaration);
			BufferLayout.LayoutDesc.Layouts.Add({ BindingNum, BindingType::UniformBuffer, InStage});
			BufferLayout.LayoutDesc.GroupNumber = Index;
			BindingNum++;
	
			return *this;
		}

		auto Build() &&
		{
			Buffer.BindGroup = GpuApi::CreateBindGroup(Buffer.Desc);
			BufferLayout.BindGroupLayout = GpuApi::CreateBindGroupLayout(BufferLayout.LayoutDesc);
			return MakeTuple(MakeUnique<ArgumentBuffer>(MoveTemp(Buffer)), MakeUnique<ArgumentBufferLayout>(MoveTemp(BufferLayout)));
		}

	private:
		BindingGroupIndex Index;
		int32 BindingNum = 0;
		ArgumentBuffer Buffer;
		ArgumentBufferLayout BufferLayout;
	};
}