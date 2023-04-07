#pragma once
#include "D3D12Common.h"
#include "D3D12Descriptor.h"
#include "D3D12Device.h"
#include "D3D12Util.h"
#include "D3D12PSO.h"
#include "D3D12Texture.h"

namespace FRAMEWORK
{
	extern void InitFrameResource();

	//The FrameResources that are immutable and always valid.
	class StaticFrameResource : public FNoncopyable
	{
	public:
		using RtvAllocatorType = CpuDescriptorAllocator<256, DescriptorType::RTV>;
		using SrvAllocatorType = GpuDescriptorAllocator<1024, DescriptorType::SRV>;
		using SamplerAllocatorType = GpuDescriptorAllocator<256, DescriptorType::SAMPLER>;

		struct DescriptorAllocatorStorage
		{
			TUniquePtr<RtvAllocatorType> RtvAllocator;
			TUniquePtr<SrvAllocatorType> SrvAllocator;
			TUniquePtr<SamplerAllocatorType> SamplerAllocator;
		};
		StaticFrameResource(TRefCountPtr<ID3D12CommandAllocator> InCommandAllocator, DescriptorAllocatorStorage&& InDescriptorAllocators);
		void Reset();
		void BindToCommandList(ID3D12GraphicsCommandList* InGraphicsCmdList);
		const DescriptorAllocatorStorage& GetDescriptorAllocators() const { return DescriptorAllocators; }

	private:
		TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;
		DescriptorAllocatorStorage DescriptorAllocators;
	};

	class CommandListContext
	{
	public:
		using FrameResourceStorage = TArray<StaticFrameResource, TFixedAllocator<FrameSourceNum>>;
	public:
		CommandListContext(FrameResourceStorage InitFrameResources, TRefCountPtr<ID3D12GraphicsCommandList> InGraphicsCmdList);
		ID3D12GraphicsCommandList* GetCommandListHandle() const { return GraphicsCmdList; }
		void ResetStaticFrameResource(uint32 FrameResourceIndex);
		void BindStaticFrameResource(uint32 FrameResourceIndex);
		const StaticFrameResource& GetCurFrameResource() const {
			uint32 Index = GetCurFrameSourceIndex();
			return FrameResources[Index];
		}
		void Transition(ID3D12Resource* InResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After);
		void SetPipeline(Dx12Pso* InPso) { CurrentPso = InPso; }
		void SetRenderTarget(Dx12Texture* InRT) { CurrentRenderTarget = InRT; }
		void SetVertexBuffer(Dx12Buffer* InBuffer) { CurrentVertexBuffer = InBuffer; }
		void SetPrimitiveType(PrimitiveType InType) { DrawType = InType; }
		void SetClearColor(TUniquePtr<Vector4f> InClearColor) { ClearColorValue = MoveTemp(InClearColor); }
		void SetViewPort(TUniquePtr<D3D12_VIEWPORT> InViewPort, TUniquePtr<D3D12_RECT> InSissorRect) {
			CurrentViewPort = MoveTemp(InViewPort);
			CurrentSissorRect = MoveTemp(InSissorRect);
		}

		void PrepareDrawingEnv();
		void MarkPipelineDirty(bool IsDirty) { IsPipelineDirty = IsDirty; }
		void MarkRenderTartgetDirty(bool IsDirty) { IsRenderTargetDirty = IsDirty; }
		void MarkVertexBufferDirty(bool IsDirty) { IsVertexBufferDirty = IsDirty; }
		void MarkViewportDirty(bool IsDirty) { IsViewportDirty = IsDirty; }

	private:
		FrameResourceStorage FrameResources;
		TRefCountPtr<ID3D12GraphicsCommandList> GraphicsCmdList;
		Dx12Pso* CurrentPso;
		Dx12Texture* CurrentRenderTarget;
		Dx12Buffer* CurrentVertexBuffer;
		PrimitiveType DrawType;
		TUniquePtr<Vector4f> ClearColorValue;
		TUniquePtr<D3D12_VIEWPORT> CurrentViewPort;
		TUniquePtr<D3D12_RECT> CurrentSissorRect;

		bool IsPipelineDirty = false;
		bool IsRenderTargetDirty = false;
		bool IsVertexBufferDirty = false;
		bool IsViewportDirty = false;
	};

	inline TUniquePtr<CommandListContext> GCommandListContext;
}