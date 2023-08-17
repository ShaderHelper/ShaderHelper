#pragma once
#include "Dx12Common.h"
#include "Dx12Descriptor.h"
#include "Dx12Device.h"
#include "Dx12Util.h"
#include "Dx12PSO.h"
#include "Dx12Texture.h"

namespace FRAMEWORK
{
	extern void InitFrameResource();

	class StaticFrameResource : public FNoncopyable
	{
	public:
		using RtvAllocatorType = CpuDescriptorAllocator<256, DescriptorType::RTV>;
		using ShaderViewAllocatorType = GpuDescriptorAllocator<1024, DescriptorType::SHADER_VIEW>;
		using SamplerAllocatorType = GpuDescriptorAllocator<256, DescriptorType::SAMPLER>;

		struct DescriptorAllocatorStorage
		{
			TUniquePtr<RtvAllocatorType> RtvAllocator;
            TUniquePtr<ShaderViewAllocatorType> ShaderViewAllocator;
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
        CommandListContext(FrameResourceStorage InitFrameResources, TRefCountPtr<ID3D12GraphicsCommandList> InGraphicsCmdList);
        
    public:
		ID3D12GraphicsCommandList* GetCommandListHandle() const { return GraphicsCmdList; }
        
        void SetClearColors(TArray<TOptional<Vector4f>> InClearColorValues) {ClearColorValues = MoveTemp(InClearColorValues);}
        void SetPipeline(Dx12Pso* InPso) { CurrentPso = InPso; }
        void SetRenderTargets(TArray<Dx12Texture*> InRTs) { CurrentRenderTargets = MoveTemp(InRTs); }
        void SetVertexBuffer(Dx12Buffer* InBuffer) { CurrentVertexBuffer = InBuffer; }
        void SetPrimitiveType(PrimitiveType InType) { DrawType = InType; }
        void SetViewPort(TUniquePtr<D3D12_VIEWPORT> InViewPort, TUniquePtr<D3D12_RECT> InSissorRect) {
            CurrentViewPort = MoveTemp(InViewPort);
            CurrentSissorRect = MoveTemp(InSissorRect);
        }
        
        void PrepareDrawingEnv();
        void MarkPipelineDirty(bool IsDirty) { IsPipelineDirty = IsDirty; }
        void MarkRenderTartgetDirty(bool IsDirty) { IsRenderTargetDirty = IsDirty; }
        void MarkVertexBufferDirty(bool IsDirty) { IsVertexBufferDirty = IsDirty; }
        void MarkViewportDirty(bool IsDirty) { IsViewportDirty = IsDirty; }
        
        void ClearBinding();
        
    public:
		void ResetStaticFrameResource(uint32 FrameResourceIndex);
		void BindStaticFrameResource(uint32 FrameResourceIndex);
		const StaticFrameResource& GetCurFrameResource() const {
			uint32 Index = GetCurFrameSourceIndex();
			return FrameResources[Index];
		}
		void Transition(ID3D12Resource* InResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After);
        
	private:
		FrameResourceStorage FrameResources;
		TRefCountPtr<ID3D12GraphicsCommandList> GraphicsCmdList;
		Dx12Pso* CurrentPso;
		Dx12Buffer* CurrentVertexBuffer;
		PrimitiveType DrawType;
		TUniquePtr<D3D12_VIEWPORT> CurrentViewPort;
		TUniquePtr<D3D12_RECT> CurrentSissorRect;
        TArray<Dx12Texture*> CurrentRenderTargets;
        TArray<TOptional<Vector4f>> ClearColorValues;

		bool IsPipelineDirty = true;
		bool IsRenderTargetDirty = true;
		bool IsVertexBufferDirty = true;
		bool IsViewportDirty = true;
	};

	inline TUniquePtr<CommandListContext> GCommandListContext;
}
