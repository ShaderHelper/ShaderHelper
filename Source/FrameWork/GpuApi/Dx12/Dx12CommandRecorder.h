#pragma once
#include "Dx12Common.h"
#include "Dx12Descriptor.h"
#include "Dx12Device.h"
#include "Dx12Util.h"
#include "Dx12PSO.h"
#include "Dx12Texture.h"
#include "Dx12RS.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	class Dx12StateCache
	{
	public:
		Dx12StateCache();

		void ApplyDrawState(ID3D12GraphicsCommandList* InCmdList);
		//void ApplyComputeState();
		void Clear();

		void SetViewPort(D3D12_VIEWPORT InViewPort, D3D12_RECT InSissorRect);
		void SetVertexBuffer(Dx12Buffer* InBuffer);
		void SetPipeline(Dx12RenderPso* InPso);
		void SetRootSignature(Dx12RootSignature* InRootSignature);
		void SetRenderTargets(TArray<Dx12Texture*> InRTs, TArray<Vector4f> InClearColorValues);
		void SetBindGroups(Dx12BindGroup* InGroup0, Dx12BindGroup* InGroup1, Dx12BindGroup* InGroup2, Dx12BindGroup* InGroup3);

	public:
		bool IsRenderPipelineDirty : 1;
		bool IsRenderTargetDirty : 1;
		bool IsVertexBufferDirty : 1;
		bool IsViewportDirty : 1;
		bool IsRootSigDirty : 1;

		bool IsBindGroup0Dirty : 1;
		bool IsBindGroup1Dirty : 1;
		bool IsBindGroup2Dirty : 1;
		bool IsBindGroup3Dirty : 1;
	
	private:
		Dx12RenderPso* CurrentPso;
		Dx12Buffer* CurrentVertexBuffer;
		TOptional<D3D12_VIEWPORT> CurrentViewPort;
		TOptional<D3D12_RECT> CurrentSissorRect;
		TArray<Dx12Texture*> CurrentRenderTargets;
		TArray<Vector4f> ClearColorValues;

		Dx12RootSignature* CurrentRootSignature;
		Dx12BindGroup* CurrentBindGroup0;
		Dx12BindGroup* CurrentBindGroup1;
		Dx12BindGroup* CurrentBindGroup2;
		Dx12BindGroup* CurrentBindGroup3;

	};

	class Dx12RenderPassRecorder : public GpuRenderPassRecorder
	{
	public:
		Dx12RenderPassRecorder(ID3D12GraphicsCommandList* InCmdList, Dx12StateCache& InStateCache) 
			: CmdList(InCmdList), StateCache(InStateCache)
		{}

	public:
		void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount) override;
		void SetRenderPipelineState(GpuPipelineState* InPipelineState) override;
		void SetVertexBuffer(GpuBuffer* InVertexBuffer) override;
		void SetViewPort(const GpuViewPortDesc& InViewPortDesc) override;
		void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) override;
	
	private:
		ID3D12GraphicsCommandList* CmdList;
		Dx12StateCache& StateCache;
	};

	class Dx12CmdRecorder : public GpuCmdRecorder
	{
	public:
		Dx12CmdRecorder(TRefCountPtr<ID3D12GraphicsCommandList> InCmdList, TRefCountPtr<ID3D12CommandAllocator> InCmdAllocator)
			: CmdList(MoveTemp(InCmdList)), CommandAllocator(MoveTemp(InCmdAllocator))
		{
			DxCheck(GDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(Fence.GetInitReference())));
			BindDescriptorHeap();
		}

		ID3D12GraphicsCommandList* GetCommandList() const { return CmdList; }

	public:
		GpuRenderPassRecorder* BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName) override;
		void EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder) override;
		void BeginCaptureEvent(const FString& EventName) override;
		void EndCaptureEvent() override;
		void Barrier(GpuTrackedResource* InResource, GpuResourceState NewState) override;
		void CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture) override;
		void CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer) override;
		void Reset();
		void SetName(const FString& Name);
		void BindDescriptorHeap();

		void MarkFinished() { 
			DxCheck(GGraphicsQueue->Signal(Fence, 1));
		}
		bool IsFree() {
			return !!Fence->GetCompletedValue();
		}

	private:
		TRefCountPtr<ID3D12GraphicsCommandList> CmdList;
		TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;
		Dx12StateCache StateCache;
		TArray<TUniquePtr<Dx12RenderPassRecorder>> RequestedRenderPassRecorders;
		TRefCountPtr<ID3D12Fence> Fence;
	};

	class Dx12CmdRecorderPool
	{
    public:
		Dx12CmdRecorder* ObtainDxCmdRecorder(const FString& RecorderName = {});
		TSharedPtr<Dx12CmdRecorder> RetrieveFreeCmdRecorder();

	private:
		TArray<TSharedPtr<Dx12CmdRecorder>> ActiveDx12CmdRecorders;
		TQueue<TSharedPtr<Dx12CmdRecorder>> FreeDx12CmdRecorders;
	};

	inline Dx12CmdRecorderPool GDx12CmdRecorderPool;
}
