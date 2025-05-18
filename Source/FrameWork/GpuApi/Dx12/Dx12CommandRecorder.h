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
	//TODO More detailed states.
	class Dx12StateCache
	{
	public:
		Dx12StateCache();

		void ApplyDrawState(ID3D12GraphicsCommandList* InCmdList);
		void ApplyComputeState(ID3D12GraphicsCommandList* InCmdList);
		void Clear();

		void SetViewPort(D3D12_VIEWPORT InViewPort, D3D12_RECT InSissorRect);
		void SetVertexBuffer(Dx12Buffer* InBuffer);
		void SetPipeline(Dx12RenderPso* InPso);
		void SetPipeline(Dx12ComputePso* InPso);
		void SetGraphicsRootSignature(Dx12RootSignature* InRootSignature);
		void SetComputeRootSignature(Dx12RootSignature* InRootSignature);
		void SetRenderTargets(TArray<Dx12Texture*> InRTs, TArray<Vector4f> InClearColorValues);
		void SetGraphicsBindGroups(Dx12BindGroup* InGroup0, Dx12BindGroup* InGroup1, Dx12BindGroup* InGroup2, Dx12BindGroup* InGroup3);
		void SetComputeBindGroups(Dx12BindGroup* InGroup0, Dx12BindGroup* InGroup1, Dx12BindGroup* InGroup2, Dx12BindGroup* InGroup3);

	public:
		bool IsPipelineStateDirty : 1;
		bool IsRenderTargetDirty : 1;
		bool IsVertexBufferDirty : 1;
		bool IsViewportDirty : 1;

		bool IsGraphicsRootSigDirty : 1;
		bool IsGraphicsBindGroup0Dirty : 1;
		bool IsGraphicsBindGroup1Dirty : 1;
		bool IsGraphicsBindGroup2Dirty : 1;
		bool IsGraphicsBindGroup3Dirty : 1;

		bool IsComputeRootSigDirty : 1;
		bool IsComputeBindGroup0Dirty : 1;
		bool IsComputeBindGroup1Dirty : 1;
		bool IsComputeBindGroup2Dirty : 1;
		bool IsComputeBindGroup3Dirty : 1;
	
	private:
		Dx12RenderPso* RenderPso = nullptr;
		Dx12ComputePso* ComputePso = nullptr;
		Dx12Buffer* CurrentVertexBuffer = nullptr;
		TOptional<D3D12_VIEWPORT> CurrentViewPort;
		TOptional<D3D12_RECT> CurrentSissorRect;
		TArray<Dx12Texture*> CurrentRenderTargets;
		TArray<Vector4f> ClearColorValues;

		Dx12RootSignature* CurrentGraphicsRootSignature = nullptr;
		Dx12BindGroup* CurrentGraphicsBindGroup0 = nullptr;
		Dx12BindGroup* CurrentGraphicsBindGroup1 = nullptr;
		Dx12BindGroup* CurrentGraphicsBindGroup2 = nullptr;
		Dx12BindGroup* CurrentGraphicsBindGroup3 = nullptr;

		Dx12RootSignature* CurrentComputeRootSignature = nullptr;
		Dx12BindGroup* CurrentComputeBindGroup0 = nullptr;
		Dx12BindGroup* CurrentComputeBindGroup1 = nullptr;
		Dx12BindGroup* CurrentComputeBindGroup2 = nullptr;
		Dx12BindGroup* CurrentComputeBindGroup3 = nullptr;
	};

	class Dx12ComputePassRecorder : public GpuComputePassRecorder
	{
	public:
		Dx12ComputePassRecorder(ID3D12GraphicsCommandList* InCmdList, Dx12StateCache& InStateCache)
			: CmdList(InCmdList), StateCache(InStateCache)
		{
		}

	public:
		void Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) override;
		void SetComputePipelineState(GpuComputePipelineState* InPipelineState) override;
		void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3) override;

	private:
		ID3D12GraphicsCommandList* CmdList;
		Dx12StateCache& StateCache;
	};

	class Dx12RenderPassRecorder : public GpuRenderPassRecorder
	{
	public:
		Dx12RenderPassRecorder(ID3D12GraphicsCommandList* InCmdList, Dx12StateCache& InStateCache) 
			: CmdList(InCmdList), StateCache(InStateCache)
		{}

	public:
		void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount) override;
		void SetRenderPipelineState(GpuRenderPipelineState* InPipelineState) override;
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
		enum State
		{
			NotSubmitted,
			Submitted,
			Finished
		};
		
		Dx12CmdRecorder(TRefCountPtr<ID3D12GraphicsCommandList> InCmdList, TRefCountPtr<ID3D12CommandAllocator> InCmdAllocator)
			: CmdList(MoveTemp(InCmdList)), CommandAllocator(MoveTemp(InCmdAllocator))
		{
			DxCheck(GDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(Fence.GetInitReference())));
			BindDescriptorHeap();
		}

		ID3D12GraphicsCommandList* GetCommandList() const { return CmdList; }

	public:
		GpuComputePassRecorder* BeginComputePass(const FString& PassName) override;
		void EndComputePass(GpuComputePassRecorder* InComputePassRecorder) override;
		GpuRenderPassRecorder* BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName) override;
		void EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder) override;
		void BeginCaptureEvent(const FString& EventName) override;
		void EndCaptureEvent() override;
		void Barriers(const TArray<GpuBarrierInfo>& BarrierInfos) override;
		void CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture) override;
		void CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer) override;
		void CopyBufferToBuffer(GpuBuffer* SrcBuffer, uint32 SrcOffset, GpuBuffer* DestBuffer, uint32 DestOffset, uint32 Size) override;
		void Reset();
		void SetName(const FString& Name);
		void BindDescriptorHeap();

		void MarkSubmitted() {
			DxCheck(Fence->Signal(Submitted));
			DxCheck(GGraphicsQueue->Signal(Fence, Finished));
		}
		bool IsSubmitted() {
			return Fence->GetCompletedValue() == Submitted;
		}
		bool IsFinished() {
			return Fence->GetCompletedValue() == Finished;
		}
		bool IsFree() {
			return IsFinished() || IsUnsubmittedAtFrameEnd;
		}
	public:
		bool IsUnsubmittedAtFrameEnd = false;

	private:
		TRefCountPtr<ID3D12GraphicsCommandList> CmdList;
		TRefCountPtr<ID3D12CommandAllocator> CommandAllocator;
		Dx12StateCache StateCache;
		TArray<TUniquePtr<Dx12RenderPassRecorder>> RequestedRenderPassRecorders;
		TArray<TUniquePtr<Dx12ComputePassRecorder>> RequestedComputePassRecorders;
		TRefCountPtr<ID3D12Fence> Fence;
	};

	class Dx12CmdRecorderPool
	{
    public:
		Dx12CmdRecorder* ObtainDxCmdRecorder(const FString& RecorderName = {});
		TSharedPtr<Dx12CmdRecorder> RetrieveFreeCmdRecorder();
		void EndFrame();

	private:
		TArray<TSharedPtr<Dx12CmdRecorder>> ActiveDx12CmdRecorders;
		TQueue<TSharedPtr<Dx12CmdRecorder>> FreeDx12CmdRecorders;
	};

	inline Dx12CmdRecorderPool GDx12CmdRecorderPool;
}
