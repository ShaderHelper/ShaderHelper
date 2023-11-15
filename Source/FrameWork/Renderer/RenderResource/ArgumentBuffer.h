#pragma once
#include "UniformBuffer.h"

namespace FRAMEWORK
{
	class ArgumentBufferLayout
	{
		friend class ArgumentBufferBuilder;
	public:
		GpuBindGroupLayout* GetBindLayout() const { return BindGroupLayout; }
		FString GetDeclaration() const { return Declaration; }

	private:
		FString Declaration;
		GpuBindGroupLayoutDesc LayoutDesc;
		TRefCountPtr<GpuBindGroupLayout> BindGroupLayout;
	};

	class ArgumentBuffer
	{
		friend class ArgumentBufferBuilder;
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
		ArgumentBufferBuilder(BindingGroupSlot InSlot)
		{
			check(InSlot < GpuResourceLimit::MaxBindableBingGroupNum);
			GroupNumber = InSlot;
		}
		
	public:
		ArgumentBufferBuilder& AddUniformBuffer(TSharedPtr<UniformBuffer> InUniformBuffer, BindingShaderStage InStage = BindingShaderStage::All)
		{
			Buffer.UniformBuffers.Add(InUniformBuffer);
			Buffer.Desc.Resources.Add({ BindingNum, InUniformBuffer->GetGpuResource() });

			FString UniformBufferDeclaration = InUniformBuffer->GetMetaData().UniformBufferDeclaration;
			int32 InsertIndex = UniformBufferDeclaration.Find(TEXT("{"));
			check(InsertIndex != INDEX_NONE);
			UniformBufferDeclaration.InsertAt(InsertIndex - 1, FString::Printf(TEXT(": register(b%d, space%d)\r\n"), BindingNum, GroupNumber));

			BufferLayout.Declaration += MoveTemp(UniformBufferDeclaration);
			BufferLayout.LayoutDesc.Layouts.Add({ BindingNum, BindingType::UniformBuffer, InStage});
			BufferLayout.LayoutDesc.GroupNumber = GroupNumber;
			BindingNum++;
	
			return *this;
		}

		auto Build()
		{
			BufferLayout.BindGroupLayout = GpuApi::CreateBindGroupLayout(BufferLayout.LayoutDesc);
			Buffer.Desc.Layout = BufferLayout.GetBindLayout();
			Buffer.BindGroup = GpuApi::CreateBindGroup(Buffer.Desc);
			return MakeTuple(MakeUnique<ArgumentBuffer>(Buffer), MakeUnique<ArgumentBufferLayout>(BufferLayout));
		}

	private:
		BindingGroupSlot GroupNumber;
		int32 BindingNum = 0;
		ArgumentBuffer Buffer;
		ArgumentBufferLayout BufferLayout;
	};
}