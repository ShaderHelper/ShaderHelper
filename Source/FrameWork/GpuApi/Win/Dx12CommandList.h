#pragma once
#include "Dx12Common.h"
#include "Dx12Descriptor.h"
#include "Dx12Device.h"
#include "Dx12Util.h"
#include "Dx12PSO.h"
#include "Dx12Texture.h"
#include "Dx12RS.h"

namespace FRAMEWORK
{
	extern void InitCommandListContext();

	class CommandListContext
	{
	public:
        CommandListContext();
        
    public:
		ID3D12GraphicsCommandList* GetCommandListHandle();

        void SetPipeline(Dx12Pso* InPso) 
		{ 
			if (CurrentPso != InPso)
			{
				CurrentPso = InPso;
				MarkPipelineDirty(true);
			}
			
		}
        void SetRenderTargets(TArray<Dx12Texture*> InRTs, TArray<TOptional<Vector4f>> InClearColorValues)
		{ 
			if (InRTs != CurrentRenderTargets || InClearColorValues != ClearColorValues)
			{
				CurrentRenderTargets = MoveTemp(InRTs);
				ClearColorValues = MoveTemp(InClearColorValues);
				MarkRenderTartgetDirty(true);
			}
		}

        void SetVertexBuffer(Dx12Buffer* InBuffer) 
		{ 
			if (CurrentVertexBuffer != InBuffer)
			{
				CurrentVertexBuffer = InBuffer;
				MarkVertexBufferDirty(true);
			}
			
		}

		void SetRootSignature(Dx12RootSignature* InRootSignature) 
		{ 
			if (CurrentRootSignature != InRootSignature)
			{
				CurrentRootSignature = InRootSignature;
				MarkRootSigDirty(true);
			}
		}
		void SetBindGroups(Dx12BindGroup* InGroup0, Dx12BindGroup* InGroup1, Dx12BindGroup* InGroup2, Dx12BindGroup* InGroup3) {
			if (InGroup0 != CurrentBindGroup0) {
				CurrentBindGroup0 = InGroup0;
				MarkBindGroup0Dirty(true);
			}
			
			if (InGroup1 != CurrentBindGroup1) {
				CurrentBindGroup1 = InGroup1;
				MarkBindGroup1Dirty(true);
			}

			if (InGroup2 != CurrentBindGroup2) {
				CurrentBindGroup2 = InGroup2;
				MarkBindGroup2Dirty(true);
			}

			if (InGroup3 != CurrentBindGroup3) {
				CurrentBindGroup3 = InGroup3;
				MarkBindGroup3Dirty(true);
			}
		}

        void SetViewPort(D3D12_VIEWPORT InViewPort, D3D12_RECT InSissorRect) 
		{
			if (!CurrentViewPort || FMemory::Memcmp(&*CurrentViewPort, &InViewPort, sizeof(D3D12_VIEWPORT)))
			{
				CurrentViewPort = MoveTemp(InViewPort);
				CurrentSissorRect = MoveTemp(InSissorRect);
				MarkViewportDirty(true);
			}
        }
        
        void PrepareDrawingEnv();
        void MarkPipelineDirty(bool IsDirty) { IsPipelineDirty = IsDirty; }
        void MarkRenderTartgetDirty(bool IsDirty) { IsRenderTargetDirty = IsDirty; }
        void MarkVertexBufferDirty(bool IsDirty) { IsVertexBufferDirty = IsDirty; }
        void MarkViewportDirty(bool IsDirty) { IsViewportDirty = IsDirty; }
		void MarkRootSigDirty(bool IsDirty) { IsRootSigDirty = IsDirty; }

		void MarkBindGroup0Dirty(bool IsDirty) { IsBindGroup0Dirty = IsDirty; }
		void MarkBindGroup1Dirty(bool IsDirty) { IsBindGroup1Dirty = IsDirty; }
		void MarkBindGroup2Dirty(bool IsDirty) { IsBindGroup2Dirty = IsDirty; }
		void MarkBindGroup3Dirty(bool IsDirty) { IsBindGroup3Dirty = IsDirty; }
        
        void ClearBinding();
        
    public:
		void Transition(TrackedResource* InResource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After);
		void SubmitCommandList();

	public:
		void InitCommandListState();
		TRefCountPtr<ID3D12CommandAllocator> RetrieveFreeCommandAllocator();

		TUniquePtr<CpuDescriptor> AllocRtv();
		TUniquePtr<CpuDescriptor> AllocCpuCbvSrvUav();
		TUniquePtr<GpuDescriptorRange> AllocGpuCbvSrvUavRange(uint32 InDescriptorNum);

        
	private:
		class PendingCommandAllocator
		{
		public:
			PendingCommandAllocator() = default;
			PendingCommandAllocator(TRefCountPtr<ID3D12CommandAllocator> InCmdAllocator);

			bool IsFree() { 
				return !!Fence->GetCompletedValue(); 
			}
			void Reset() { 
				DxCheck(CommandAllocator->Reset()); 
			}
			
			operator TRefCountPtr<ID3D12CommandAllocator>() const { return CommandAllocator; }

		private:
			TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;
			TRefCountPtr<ID3D12Fence> Fence;
		};

		TRefCountPtr<ID3D12GraphicsCommandList> GraphicsCmdList;
		TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;
		//Command Allocators that have already been submitted to GPU.
		TArray<PendingCommandAllocator> PendingCommandAllocators;
		TQueue<TRefCountPtr<ID3D12CommandAllocator>> FreeCommandAllocatorPool;

		CpuDescriptorAllocator RtvAllocator;
		CpuDescriptorAllocator Cpu_CbvSrvUavAllocator;
		GpuDescriptorAllocator Gpu_CbvSrvUavAllocator;
		
		Dx12Pso* CurrentPso;
		Dx12Buffer* CurrentVertexBuffer;
		TOptional<D3D12_VIEWPORT> CurrentViewPort;
		TOptional<D3D12_RECT> CurrentSissorRect;
        TArray<Dx12Texture*> CurrentRenderTargets;
        TArray<TOptional<Vector4f>> ClearColorValues;

		Dx12RootSignature* CurrentRootSignature;
		Dx12BindGroup* CurrentBindGroup0;
		Dx12BindGroup* CurrentBindGroup1;
		Dx12BindGroup* CurrentBindGroup2;
		Dx12BindGroup* CurrentBindGroup3;

		//State cache
		bool IsPipelineDirty : 1;
		bool IsRenderTargetDirty : 1;
		bool IsVertexBufferDirty : 1;
		bool IsViewportDirty : 1;
		bool IsRootSigDirty : 1;

		bool IsBindGroup0Dirty : 1;
		bool IsBindGroup1Dirty : 1;
		bool IsBindGroup2Dirty : 1;
		bool IsBindGroup3Dirty : 1;
	};

	inline CommandListContext* GCommandListContext;
}
