#include "CommonHeader.h"
#include "Dx12CommandRecorder.h"
#include "Dx12GpuRhiBackend.h"
#include "Dx12Map.h"

namespace FW
{
	static void ResolveDx12MsaaTarget(ID3D12GraphicsCommandList* CmdList, GpuCmdRecorder* Recorder, Dx12TextureView* SourceView, Dx12TextureView* ResolveView)
	{
		Dx12Texture* SourceTexture = static_cast<Dx12Texture*>(SourceView->GetTexture());
		Dx12Texture* ResolveTexture = static_cast<Dx12Texture*>(ResolveView->GetTexture());
		check(SourceTexture->GetSampleCount() > 1);
		check(ResolveTexture->GetSampleCount() == 1);
		check(SourceTexture->GetNumMips() == 1);
		check(SourceTexture->GetArrayLayerCount() == 1);

		const uint32 ResolveMip = ResolveView->GetBaseMipLevel();
		const uint32 ResolveArrayLayer = ResolveView->GetBaseArrayLayer();
		const uint32 SourceSubResource = 0;
		const uint32 ResolveSubResource = D3D12CalcSubresource(ResolveMip, ResolveArrayLayer, 0, ResolveTexture->GetNumMips(), ResolveTexture->GetArrayLayerCount());

		const GpuResourceState SourceState = Recorder->GetLocalTextureSubResourceState(SourceTexture, 0, 0);
		const GpuResourceState ResolveState = Recorder->GetLocalTextureSubResourceState(ResolveTexture, ResolveMip, ResolveArrayLayer);
		const D3D12_RESOURCE_STATES SourceDxState = MapResourceState(SourceState);
		const D3D12_RESOURCE_STATES ResolveDxState = MapResourceState(ResolveState);

		TArray<CD3DX12_RESOURCE_BARRIER, TFixedAllocator<2>> Barriers;
		if (SourceDxState != D3D12_RESOURCE_STATE_RESOLVE_SOURCE)
		{
			Barriers.Add(CD3DX12_RESOURCE_BARRIER::Transition(SourceTexture->GetResource(), SourceDxState, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, SourceSubResource));
		}
		if (ResolveDxState != D3D12_RESOURCE_STATE_RESOLVE_DEST)
		{
			Barriers.Add(CD3DX12_RESOURCE_BARRIER::Transition(ResolveTexture->GetResource(), ResolveDxState, D3D12_RESOURCE_STATE_RESOLVE_DEST, ResolveSubResource));
		}
		if (Barriers.Num() > 0)
		{
			CmdList->ResourceBarrier(Barriers.Num(), Barriers.GetData());
		}

		CmdList->ResolveSubresource(ResolveTexture->GetResource(), ResolveSubResource, SourceTexture->GetResource(), SourceSubResource, MapTextureFormat(SourceTexture->GetFormat()));

		Barriers.Reset();
		if (SourceDxState != D3D12_RESOURCE_STATE_RESOLVE_SOURCE)
		{
			Barriers.Add(CD3DX12_RESOURCE_BARRIER::Transition(SourceTexture->GetResource(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE, SourceDxState, SourceSubResource));
		}
		if (ResolveDxState != D3D12_RESOURCE_STATE_RESOLVE_DEST)
		{
			Barriers.Add(CD3DX12_RESOURCE_BARRIER::Transition(ResolveTexture->GetResource(), D3D12_RESOURCE_STATE_RESOLVE_DEST, ResolveDxState, ResolveSubResource));
		}
		if (Barriers.Num() > 0)
		{
			CmdList->ResourceBarrier(Barriers.Num(), Barriers.GetData());
		}
	}

	Dx12StateCache::Dx12StateCache()
		: IsPipelineStateDirty(false)
		, IsRenderTargetDirty(false)
		, IsVertexBufferDirty(false)
		, IsIndexBufferDirty(false)
		, IsViewportDirty(false)
		, IsScissorRectDirty(false)
		, IsGraphicsRootSigDirty(false)
		, IsGraphicsBindGroup0Dirty(false)
		, IsGraphicsBindGroup1Dirty(false)
		, IsGraphicsBindGroup2Dirty(false)
		, IsGraphicsBindGroup3Dirty(false)
		, IsComputeRootSigDirty(false)
		, IsComputeBindGroup0Dirty(false)
		, IsComputeBindGroup1Dirty(false)
		, IsComputeBindGroup2Dirty(false)
		, IsComputeBindGroup3Dirty(false)
	{
	}

	void Dx12StateCache::ApplyComputeState(ID3D12GraphicsCommandList* InCmdList)
	{
		check(InCmdList);

		if (IsPipelineStateDirty)
		{
			check(ComputePso);
			InCmdList->SetPipelineState(ComputePso->GetResource());
			IsPipelineStateDirty = false;
		}

		if (IsComputeRootSigDirty)
		{
			check(CurrentComputeRootSignature);
			InCmdList->SetComputeRootSignature(CurrentComputeRootSignature->GetResource());
			IsComputeRootSigDirty = false;
		}

		if (CurrentComputeBindGroup0 && IsComputeBindGroup0Dirty) {
			CurrentComputeBindGroup0->ApplyComputeBinding(InCmdList, CurrentComputeRootSignature);
			IsComputeBindGroup0Dirty = false;
		}

		if (CurrentComputeBindGroup1 && IsComputeBindGroup1Dirty) {
			CurrentComputeBindGroup1->ApplyComputeBinding(InCmdList, CurrentComputeRootSignature);
			IsComputeBindGroup1Dirty = false;
		}

		if (CurrentComputeBindGroup2 && IsComputeBindGroup2Dirty) {
			CurrentComputeBindGroup2->ApplyComputeBinding(InCmdList, CurrentComputeRootSignature);
			IsComputeBindGroup2Dirty = false;
		}

		if (CurrentComputeBindGroup3 && IsComputeBindGroup3Dirty) {
			CurrentComputeBindGroup3->ApplyComputeBinding(InCmdList, CurrentComputeRootSignature);
			IsComputeBindGroup3Dirty = false;
		}

	}

	void Dx12StateCache::ApplyDrawState(ID3D12GraphicsCommandList* InCmdList)
	{
		check(InCmdList);

		if (!CurrentViewPort.IsSet())
		{
			D3D12_VIEWPORT DefaultViewPort{};
			DefaultViewPort.Width = (float)CurrentRenderTargetViews[0]->GetWidth();
			DefaultViewPort.Height = (float)CurrentRenderTargetViews[0]->GetHeight();
			DefaultViewPort.MinDepth = 0;
			DefaultViewPort.MaxDepth = 1;
			DefaultViewPort.TopLeftX = 0;
			DefaultViewPort.TopLeftY = 0;
			SetViewPort(MoveTemp(DefaultViewPort));
		}
		if (!CurrentScissorRect.IsSet() && CurrentViewPort)
		{
			const LONG ScissorLeft = (LONG)(*CurrentViewPort).TopLeftX;
			const LONG ScissorTop = (LONG)(*CurrentViewPort).TopLeftY;
			const LONG ScissorRight = (LONG)((*CurrentViewPort).TopLeftX + (*CurrentViewPort).Width);
			const LONG ScissorBottom = (LONG)((*CurrentViewPort).TopLeftY + (*CurrentViewPort).Height);
			D3D12_RECT DefaultScissorRect = CD3DX12_RECT(ScissorLeft, ScissorTop, ScissorRight, ScissorBottom);
			SetScissorRect(MoveTemp(DefaultScissorRect));
		}

		if (IsViewportDirty && CurrentViewPort)
		{
			InCmdList->RSSetViewports(1, &*CurrentViewPort);
			IsViewportDirty = false;
		}

		if (IsScissorRectDirty && CurrentScissorRect)
		{
			InCmdList->RSSetScissorRects(1, &*CurrentScissorRect);
			IsScissorRectDirty = false;
		}

		if (IsVertexBufferDirty)
		{
			if (RenderPso && RenderPso->GetDesc().VertexLayout.Num() > 0) {
				for (int32 BufferSlot = 0; BufferSlot < RenderPso->GetDesc().VertexLayout.Num(); ++BufferSlot)
				{
					const GpuVertexLayoutDesc& BufferLayout = RenderPso->GetDesc().VertexLayout[BufferSlot];
					const Dx12VertexBufferBinding& Binding = CurrentVertexBuffers[BufferSlot];
					if (!Binding.Buffer)
					{
						continue;
					}
					D3D12_VERTEX_BUFFER_VIEW View{};
					View.BufferLocation = Binding.Buffer->GetAllocation().GetGpuAddr() + Binding.Offset;
					View.StrideInBytes = BufferLayout.ByteStride;
					View.SizeInBytes = Binding.Buffer->GetByteSize() - Binding.Offset;
					InCmdList->IASetVertexBuffers(BufferSlot, 1, &View);
				}
			}
			else {
				InCmdList->IASetVertexBuffers(0, 0, nullptr);
			}
			IsVertexBufferDirty = false;
		}

		if (IsIndexBufferDirty)
		{
			if (CurrentIndexBuffer)
			{
				D3D12_INDEX_BUFFER_VIEW View{};
				View.BufferLocation = CurrentIndexBuffer->GetAllocation().GetGpuAddr() + CurrentIndexOffset;
				View.SizeInBytes = CurrentIndexBuffer->GetByteSize() - CurrentIndexOffset;
				View.Format = MapIndexFormat(CurrentIndexFormat);
				InCmdList->IASetIndexBuffer(&View);
			}
			else
			{
				InCmdList->IASetIndexBuffer(nullptr);
			}
			IsIndexBufferDirty = false;
		}

		if (IsPipelineStateDirty)
		{
			check(RenderPso);
			InCmdList->SetPipelineState(RenderPso->GetResource());
			InCmdList->IASetPrimitiveTopology(RenderPso->GetPritimiveTopology());
			IsPipelineStateDirty = false;
		}

		if (IsGraphicsRootSigDirty)
		{
			check(CurrentGraphicsRootSignature);
			InCmdList->SetGraphicsRootSignature(CurrentGraphicsRootSignature->GetResource());
			IsGraphicsRootSigDirty = false;
		}

		if (CurrentGraphicsBindGroup0 && IsGraphicsBindGroup0Dirty) {
			CurrentGraphicsBindGroup0->ApplyDrawBinding(InCmdList, CurrentGraphicsRootSignature);
			IsGraphicsBindGroup0Dirty = false;
		}

		if (CurrentGraphicsBindGroup1 && IsGraphicsBindGroup1Dirty) {
			CurrentGraphicsBindGroup1->ApplyDrawBinding(InCmdList, CurrentGraphicsRootSignature);
			IsGraphicsBindGroup1Dirty = false;
		}

		if (CurrentGraphicsBindGroup2 && IsGraphicsBindGroup2Dirty) {
			CurrentGraphicsBindGroup2->ApplyDrawBinding(InCmdList, CurrentGraphicsRootSignature);
			IsGraphicsBindGroup2Dirty = false;
		}

		if (CurrentGraphicsBindGroup3 && IsGraphicsBindGroup3Dirty) {
			CurrentGraphicsBindGroup3->ApplyDrawBinding(InCmdList, CurrentGraphicsRootSignature);
			IsGraphicsBindGroup3Dirty = false;
		}

		if (IsRenderTargetDirty)
		{
			const uint32 RenderTargetNum = CurrentRenderTargetViews.Num();
			if (RenderTargetNum > 0)
			{
				check(ClearColorValues.Num() == RenderTargetNum);
				check(LoadActions.Num() == RenderTargetNum)

				TArray<D3D12_CPU_DESCRIPTOR_HANDLE> RenderTargetDescriptors;
				RenderTargetDescriptors.SetNumUninitialized(RenderTargetNum);

				for (uint32 i = 0; i < RenderTargetNum; i++)
				{
					Dx12TextureView* RtView = CurrentRenderTargetViews[i];
					check(RtView);
					check(RtView->GetRTV()->IsValid());

					if (LoadActions[i] == RenderTargetLoadAction::Clear)
					{
						Vector4f OptimizedClearValue = RtView->GetTexture()->GetResourceDesc().ClearValues;
						Vector4f ClearColorValue = ClearColorValues[i];
						if (!ClearColorValue.Equals(OptimizedClearValue))
						{
							SH_LOG(LogDx12, Warning, TEXT("OptimizedClearValue(%s) != ClearColorValue(%s) that may result in invalid fast clear optimization."), *OptimizedClearValue.ToString(), *ClearColorValue.ToString());
						}
						InCmdList->ClearRenderTargetView(RtView->GetRTV()->GetHandle(), ClearColorValue.GetData(), 0, nullptr);
					}
			
					RenderTargetDescriptors[i] = RtView->GetRTV()->GetHandle();
				}

				InCmdList->OMSetRenderTargets(RenderTargetNum, RenderTargetDescriptors.GetData(), false, 
					CurrentDSVView ? &CurrentDSVView->GetDSV()->GetHandle() : nullptr);

				if (CurrentDSVView && DSVLoadAction == RenderTargetLoadAction::Clear)
				{
					InCmdList->ClearDepthStencilView(CurrentDSVView->GetDSV()->GetHandle(), D3D12_CLEAR_FLAG_DEPTH, DSVClearDepth, 0, 0, nullptr);
				}
			}
			IsRenderTargetDirty = false;
		}
	}

	void Dx12StateCache::Clear()
	{
		RenderPso = nullptr;
		ComputePso = nullptr;
		for (Dx12VertexBufferBinding& Binding : CurrentVertexBuffers)
		{
			Binding = {};
		}
		CurrentIndexBuffer = nullptr;
		CurrentIndexFormat = GpuFormat::R16_UINT;
		CurrentIndexOffset = 0;
		CurrentViewPort.Reset();
		CurrentScissorRect.Reset();

		CurrentGraphicsRootSignature = nullptr;
		CurrentGraphicsBindGroup0 = nullptr;
		CurrentGraphicsBindGroup1 = nullptr;
		CurrentGraphicsBindGroup2 = nullptr;
		CurrentGraphicsBindGroup3 = nullptr;

		CurrentComputeRootSignature = nullptr;
		CurrentComputeBindGroup0 = nullptr;
		CurrentComputeBindGroup1 = nullptr;
		CurrentComputeBindGroup2 = nullptr;
		CurrentComputeBindGroup3 = nullptr;

		CurrentRenderTargetViews.Empty();
		LoadActions.Reset();
		ClearColorValues.Reset();

		IsPipelineStateDirty = false;
		IsRenderTargetDirty = false;
		IsVertexBufferDirty = false;
		IsIndexBufferDirty = false;
		IsViewportDirty = false;
		IsScissorRectDirty = false;

		IsGraphicsRootSigDirty = false;
		IsGraphicsBindGroup0Dirty = false;
		IsGraphicsBindGroup1Dirty = false;
		IsGraphicsBindGroup2Dirty = false;
		IsGraphicsBindGroup3Dirty = false;

		IsComputeRootSigDirty = false;
		IsComputeBindGroup0Dirty = false;
		IsComputeBindGroup1Dirty = false;
		IsComputeBindGroup2Dirty = false;
		IsComputeBindGroup3Dirty = false;

	}

	void Dx12StateCache::SetViewPort(D3D12_VIEWPORT InViewPort)
	{
		if (!CurrentViewPort || FMemory::Memcmp(&*CurrentViewPort, &InViewPort, sizeof(D3D12_VIEWPORT)))
		{
			CurrentViewPort = MoveTemp(InViewPort);
			IsViewportDirty = true;
		}
	}

	void Dx12StateCache::SetScissorRect(D3D12_RECT InScissorRect)
	{
		if (!CurrentScissorRect || FMemory::Memcmp(&*CurrentScissorRect, &InScissorRect, sizeof(D3D12_RECT)))
		{
			CurrentScissorRect = MoveTemp(InScissorRect);
			IsScissorRectDirty = true;
		}
	}

	void Dx12StateCache::SetVertexBuffer(uint32 Slot, Dx12Buffer* InBuffer, uint32 Offset)
	{
		Dx12VertexBufferBinding NewBinding{ InBuffer, Offset };
		if (CurrentVertexBuffers[Slot].Buffer != NewBinding.Buffer || CurrentVertexBuffers[Slot].Offset != NewBinding.Offset)
		{
			CurrentVertexBuffers[Slot] = NewBinding;
			IsVertexBufferDirty = true;
		}
	}

	void Dx12StateCache::SetIndexBuffer(Dx12Buffer* InBuffer, GpuFormat InIndexFormat, uint32 Offset)
	{
		if (CurrentIndexBuffer != InBuffer || CurrentIndexFormat != InIndexFormat || CurrentIndexOffset != Offset)
		{
			CurrentIndexBuffer = InBuffer;
			CurrentIndexFormat = InIndexFormat;
			CurrentIndexOffset = Offset;
			IsIndexBufferDirty = true;
		}
	}

	void Dx12StateCache::SetPipeline(Dx12RenderPso* InPso)
	{
		if (RenderPso != InPso)
		{
			RenderPso = InPso;
			ComputePso = nullptr;
			IsPipelineStateDirty = true;
		}
	}

	void Dx12StateCache::SetPipeline(Dx12ComputePso* InPso)
	{
		if (ComputePso != InPso)
		{
			ComputePso = InPso;
			RenderPso = nullptr;
			IsPipelineStateDirty = true;
		}
	}

	void Dx12StateCache::SetGraphicsRootSignature(Dx12RootSignature* InRootSignature)
	{
		if (CurrentGraphicsRootSignature != InRootSignature)
		{
			CurrentGraphicsRootSignature = InRootSignature;
			IsGraphicsRootSigDirty = true;
		}
	}

	void Dx12StateCache::SetComputeRootSignature(Dx12RootSignature* InRootSignature)
	{
		if (CurrentComputeRootSignature != InRootSignature)
		{
			CurrentComputeRootSignature = InRootSignature;
			IsComputeRootSigDirty = true;
		}
	}

	void Dx12StateCache::SetRenderTargets(TArray<Dx12TextureView*> InRTViews, TArray<Vector4f> InClearColorValues, TArray<RenderTargetLoadAction> InLoadActions)
	{
		CurrentViewPort.Reset();
		CurrentScissorRect.Reset();

		CurrentDSVView = nullptr;

		if (InRTViews != CurrentRenderTargetViews || InClearColorValues != ClearColorValues || LoadActions != InLoadActions)
		{
			CurrentRenderTargetViews = MoveTemp(InRTViews);
			ClearColorValues = MoveTemp(InClearColorValues);
			LoadActions = MoveTemp(InLoadActions);
			IsRenderTargetDirty = true;
		}
	}

	void Dx12StateCache::SetDepthStencilTarget(Dx12TextureView* InDSVView, RenderTargetLoadAction InLoadAction, float InClearDepth)
	{
		CurrentDSVView = InDSVView;
		DSVLoadAction = InLoadAction;
		DSVClearDepth = InClearDepth;
		IsRenderTargetDirty = true;
	}

	void Dx12StateCache::SetGraphicsBindGroups(Dx12BindGroup* InGroup0, Dx12BindGroup* InGroup1, Dx12BindGroup* InGroup2, Dx12BindGroup* InGroup3)
	{
		if (InGroup0 != CurrentGraphicsBindGroup0) {
			CurrentGraphicsBindGroup0 = InGroup0;
			IsGraphicsBindGroup0Dirty = true;
		}
		if (InGroup1 != CurrentGraphicsBindGroup1) {
			CurrentGraphicsBindGroup1 = InGroup1;
			IsGraphicsBindGroup1Dirty = true;
		}
		if (InGroup2 != CurrentGraphicsBindGroup2) {
			CurrentGraphicsBindGroup2 = InGroup2;
			IsGraphicsBindGroup2Dirty = true;
		}
		if (InGroup3 != CurrentGraphicsBindGroup3) {
			CurrentGraphicsBindGroup3 = InGroup3;
			IsGraphicsBindGroup3Dirty = true;
		}
	}

	void Dx12StateCache::SetComputeBindGroups(Dx12BindGroup* InGroup0, Dx12BindGroup* InGroup1, Dx12BindGroup* InGroup2, Dx12BindGroup* InGroup3)
	{
		if (InGroup0 != CurrentComputeBindGroup0) {
			CurrentComputeBindGroup0 = InGroup0;
			IsComputeBindGroup0Dirty = true;
		}
		if (InGroup1 != CurrentComputeBindGroup1) {
			CurrentComputeBindGroup1 = InGroup1;
			IsComputeBindGroup1Dirty = true;
		}
		if (InGroup2 != CurrentComputeBindGroup2) {
			CurrentComputeBindGroup2 = InGroup2;
			IsComputeBindGroup2Dirty = true;
		}
		if (InGroup3 != CurrentComputeBindGroup3) {
			CurrentComputeBindGroup3 = InGroup3;
			IsComputeBindGroup3Dirty = true;
		}
	}

	void Dx12CmdRecorderPool::EndFrame()
	{
		for(auto& ActiveDx12CmdRecorder : ActiveDx12CmdRecorders)
		{
			if(ActiveDx12CmdRecorder->IsUnsubmitted())
			{
				ActiveDx12CmdRecorder->End();
				ActiveDx12CmdRecorder->IsUnsubmittedAtFrameEnd = true;
			}
		}
	}

	TSharedPtr<Dx12CmdRecorder> Dx12CmdRecorderPool::RetrieveFreeCmdRecorder()
	{
		for (auto It = ActiveDx12CmdRecorders.CreateIterator(); It; It++)
		{
			auto& ActiveDx12CmdRecorder = *It;
			if (ActiveDx12CmdRecorder->IsFree())
			{
				ActiveDx12CmdRecorder->Reset();
				FreeDx12CmdRecorders.Enqueue(ActiveDx12CmdRecorder);
				It.RemoveCurrent();
				break;
			}			
		}

		if (!FreeDx12CmdRecorders.IsEmpty())
		{
			TSharedPtr<Dx12CmdRecorder> FreeCmdRecorder;
			FreeDx12CmdRecorders.Dequeue(FreeCmdRecorder);
			return FreeCmdRecorder;
		}
		
		return nullptr;
	}

	Dx12CmdRecorder* Dx12CmdRecorderPool::ObtainDxCmdRecorder(const FString& RecorderName)
	{
		TSharedPtr<Dx12CmdRecorder> FreeDx12CmdRecorder = RetrieveFreeCmdRecorder();
		if (FreeDx12CmdRecorder)
		{
			FreeDx12CmdRecorder->SetName(RecorderName);
			ActiveDx12CmdRecorders.Emplace(FreeDx12CmdRecorder);
			return FreeDx12CmdRecorder.Get();
		}
		else
		{
			TRefCountPtr<ID3D12GraphicsCommandList> NewCmdList;
			TRefCountPtr<ID3D12CommandAllocator> NewCmdAllocator;
			DxCheck(GDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(NewCmdAllocator.GetInitReference())));
			DxCheck(GDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, NewCmdAllocator, nullptr, IID_PPV_ARGS(NewCmdList.GetInitReference())));
			TSharedPtr<Dx12CmdRecorder> NewCmdRecorder = MakeShared<Dx12CmdRecorder>(MoveTemp(NewCmdList), MoveTemp(NewCmdAllocator));
			NewCmdRecorder->SetName(RecorderName);
			ActiveDx12CmdRecorders.Emplace(NewCmdRecorder);
			return NewCmdRecorder.Get();
		}
	}

	void Dx12RenderPassRecorder::DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount)
	{
		StateCache.ApplyDrawState(CmdList);
		CmdList->DrawInstanced(VertexCount, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}

	void Dx12RenderPassRecorder::DrawIndexed(uint32 StartIndexLocation, uint32 IndexCount, int32 BaseVertexLocation, uint32 StartInstanceLocation, uint32 InstanceCount)
	{
		StateCache.ApplyDrawState(CmdList);
		CmdList->DrawIndexedInstanced(IndexCount, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
	}

	void Dx12RenderPassRecorder::SetRenderPipelineState(GpuRenderPipelineState* InPipelineState)
	{
		StateCache.SetPipeline(static_cast<Dx12RenderPso*>(InPipelineState));
		if (!StateCache.GetGraphicsRootSignature())
		{
			RootSignatureDesc EmptyRsDesc{};
			StateCache.SetGraphicsRootSignature(Dx12RootSignatureManager::GetRootSignature(EmptyRsDesc));
		}
	}

	void Dx12RenderPassRecorder::SetVertexBuffer(uint32 Slot, GpuBuffer* InVertexBuffer, uint32 Offset)
	{
		StateCache.SetVertexBuffer(Slot, static_cast<Dx12Buffer*>(InVertexBuffer), Offset);
	}

	void Dx12RenderPassRecorder::SetIndexBuffer(GpuBuffer* InIndexBuffer, GpuFormat IndexFormat, uint32 Offset)
	{
		StateCache.SetIndexBuffer(static_cast<Dx12Buffer*>(InIndexBuffer), IndexFormat, Offset);
	}

	void Dx12RenderPassRecorder::SetViewPort(const GpuViewPortDesc& InViewPortDesc)
	{
		D3D12_VIEWPORT ViewPort{};
		ViewPort.Width = InViewPortDesc.Width;
		ViewPort.Height = InViewPortDesc.Height;
		ViewPort.MinDepth = InViewPortDesc.ZMin;
		ViewPort.MaxDepth = InViewPortDesc.ZMax;
		ViewPort.TopLeftX = InViewPortDesc.TopLeftX;
		ViewPort.TopLeftY = InViewPortDesc.TopLeftY;

		StateCache.SetViewPort(MoveTemp(ViewPort));
	}

	void Dx12RenderPassRecorder::SetScissorRect(const GpuScissorRectDesc& InScissorRectDes)
	{
		D3D12_RECT ScissorRect = CD3DX12_RECT(InScissorRectDes.Left, InScissorRectDes.Top, InScissorRectDes.Right, InScissorRectDes.Bottom);
		StateCache.SetScissorRect(MoveTemp(ScissorRect));
	}

	void Dx12RenderPassRecorder::SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
		RootSignatureDesc RsDesc{
		BindGroup0 ? static_cast<Dx12BindGroupLayout*>(BindGroup0->GetLayout()) : nullptr,
		BindGroup1 ? static_cast<Dx12BindGroupLayout*>(BindGroup1->GetLayout()) : nullptr,
		BindGroup2 ? static_cast<Dx12BindGroupLayout*>(BindGroup2->GetLayout()) : nullptr,
		BindGroup3 ? static_cast<Dx12BindGroupLayout*>(BindGroup3->GetLayout()) : nullptr
		};

		Dx12BindGroup* Dx12BindGroup0 = static_cast<Dx12BindGroup*>(BindGroup0);
		Dx12BindGroup* Dx12BindGroup1 = static_cast<Dx12BindGroup*>(BindGroup1);
		Dx12BindGroup* Dx12BindGroup2 = static_cast<Dx12BindGroup*>(BindGroup2);
		Dx12BindGroup* Dx12BindGroup3 = static_cast<Dx12BindGroup*>(BindGroup3);

		StateCache.SetGraphicsRootSignature(Dx12RootSignatureManager::GetRootSignature(RsDesc));
		StateCache.SetGraphicsBindGroups(Dx12BindGroup0, Dx12BindGroup1, Dx12BindGroup2, Dx12BindGroup3);
	}


	void Dx12ComputePassRecorder::Dispatch(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
	{
		StateCache.ApplyComputeState(CmdList);
		CmdList->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
	}

	void Dx12ComputePassRecorder::SetComputePipelineState(GpuComputePipelineState* InPipelineState)
	{
		StateCache.SetPipeline(static_cast<Dx12ComputePso*>(InPipelineState));
	}

	void Dx12ComputePassRecorder::SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
		RootSignatureDesc RsDesc{
		BindGroup0 ? static_cast<Dx12BindGroupLayout*>(BindGroup0->GetLayout()) : nullptr,
		BindGroup1 ? static_cast<Dx12BindGroupLayout*>(BindGroup1->GetLayout()) : nullptr,
		BindGroup2 ? static_cast<Dx12BindGroupLayout*>(BindGroup2->GetLayout()) : nullptr,
		BindGroup3 ? static_cast<Dx12BindGroupLayout*>(BindGroup3->GetLayout()) : nullptr
		};

		Dx12BindGroup* Dx12BindGroup0 = static_cast<Dx12BindGroup*>(BindGroup0);
		Dx12BindGroup* Dx12BindGroup1 = static_cast<Dx12BindGroup*>(BindGroup1);
		Dx12BindGroup* Dx12BindGroup2 = static_cast<Dx12BindGroup*>(BindGroup2);
		Dx12BindGroup* Dx12BindGroup3 = static_cast<Dx12BindGroup*>(BindGroup3);

		StateCache.SetComputeRootSignature(Dx12RootSignatureManager::GetRootSignature(RsDesc));
		StateCache.SetComputeBindGroups(Dx12BindGroup0, Dx12BindGroup1, Dx12BindGroup2, Dx12BindGroup3);
	}

	GpuComputePassRecorder* Dx12CmdRecorder::BeginComputePass(const FString& PassName)
	{
		BeginCaptureEvent(PassName);
		auto NewPassRecorder = MakeUnique<Dx12ComputePassRecorder>(CmdList, StateCache);
		RequestedComputePassRecorders.Add(MoveTemp(NewPassRecorder));
		return RequestedComputePassRecorders.Last().Get();
	}

	void Dx12CmdRecorder::EndComputePass(GpuComputePassRecorder* InComputePassRecorder)
	{
		EndCaptureEvent();
	}

	GpuRenderPassRecorder* Dx12CmdRecorder::BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName)
	{
		BeginCaptureEvent(PassName);

		TArray<Dx12TextureView*> RTViews;
		TArray<Vector4f> ClearColorValues;
		TArray<RenderTargetLoadAction> LoadActions;

		for (int32 i = 0; i < PassDesc.ColorRenderTargets.Num(); i++) {
			Dx12TextureView* RtView = static_cast<Dx12TextureView*>(PassDesc.ColorRenderTargets[i].View);
			RTViews.Add(RtView);
			ClearColorValues.Add(PassDesc.ColorRenderTargets[i].ClearColor);
			LoadActions.Add(PassDesc.ColorRenderTargets[i].LoadAction);
		}

		CurrentRenderPassDesc = PassDesc;
		CurrentTimestampWrites = PassDesc.TimestampWrites;
		if (CurrentTimestampWrites)
		{
			Dx12QuerySet* DxQuerySet = static_cast<Dx12QuerySet*>(CurrentTimestampWrites->QuerySet);
			CmdList->EndQuery(DxQuerySet->GetHeap(), D3D12_QUERY_TYPE_TIMESTAMP, CurrentTimestampWrites->BeginningOfPassWriteIndex);
		}

		StateCache.SetRenderTargets(MoveTemp(RTViews), MoveTemp(ClearColorValues), MoveTemp(LoadActions));

		if (PassDesc.DepthStencilTarget)
		{
			Dx12TextureView* DsvView = static_cast<Dx12TextureView*>(PassDesc.DepthStencilTarget->View);
			StateCache.SetDepthStencilTarget(DsvView, PassDesc.DepthStencilTarget->LoadAction, PassDesc.DepthStencilTarget->ClearDepth);
		}

		auto NewPassRecorder = MakeUnique<Dx12RenderPassRecorder>(CmdList, StateCache);
		RequestedRenderPassRecorders.Add(MoveTemp(NewPassRecorder));
		return RequestedRenderPassRecorders.Last().Get();
	}

	void Dx12CmdRecorder::EndRenderPass(GpuRenderPassRecorder* InRenderPassRecorder)
	{
		if (CurrentRenderPassDesc)
		{
			for (const GpuRenderTargetInfo& RenderTargetInfo : CurrentRenderPassDesc->ColorRenderTargets)
			{
				if (RenderTargetInfo.ResolveTarget)
				{
					ResolveDx12MsaaTarget(CmdList.GetReference(), this, static_cast<Dx12TextureView*>(RenderTargetInfo.View), static_cast<Dx12TextureView*>(RenderTargetInfo.ResolveTarget));
				}
			}
			CurrentRenderPassDesc.Reset();
		}

		if (CurrentTimestampWrites)
		{
			Dx12QuerySet* DxQuerySet = static_cast<Dx12QuerySet*>(CurrentTimestampWrites->QuerySet);
			CmdList->EndQuery(DxQuerySet->GetHeap(), D3D12_QUERY_TYPE_TIMESTAMP, CurrentTimestampWrites->EndOfPassWriteIndex);

			uint32 FirstQuery = FMath::Min(CurrentTimestampWrites->BeginningOfPassWriteIndex, CurrentTimestampWrites->EndOfPassWriteIndex);
			uint32 Count = FMath::Abs((int32)CurrentTimestampWrites->EndOfPassWriteIndex - (int32)CurrentTimestampWrites->BeginningOfPassWriteIndex) + 1;
			CmdList->ResolveQueryData(DxQuerySet->GetHeap(), D3D12_QUERY_TYPE_TIMESTAMP, FirstQuery, Count,
				DxQuerySet->GetReadbackBuffer(), FirstQuery * sizeof(uint64));
			CurrentTimestampWrites.Reset();
		}
		EndCaptureEvent();
	}

	void Dx12CmdRecorder::BeginCaptureEvent(const FString& EventName)
	{
#if GPU_API_CAPTURE && USE_PIX
		PIXBeginEvent(CmdList.GetReference(), PIX_COLOR_DEFAULT, *EventName);
#endif
	}

	void Dx12CmdRecorder::EndCaptureEvent()
	{
#if GPU_API_CAPTURE && USE_PIX
		PIXEndEvent(CmdList.GetReference());
#endif
	}

	void Dx12CmdRecorder::Barriers(const TArray<GpuBarrierInfo>& BarrierInfos)
	{
		if (BarrierInfos.IsEmpty())
		{
			return;
		}

		TArray<CD3DX12_RESOURCE_BARRIER> DxBarriers;
		for (const GpuBarrierInfo& BarrierInfo : BarrierInfos)
		{
			D3D12_RESOURCE_STATES AfterDx12State = MapResourceState(BarrierInfo.NewState);

			if (BarrierInfo.Resource->GetType() == GpuResourceType::Texture)
			{
				Dx12Texture* DxTex = static_cast<Dx12Texture*>(BarrierInfo.Resource);
				uint32 NumMips = DxTex->GetNumMips();
				uint32 ArraySize = DxTex->GetArrayLayerCount();
				bool bNeedUAVBarrier = false;
				for (uint32 Mip = 0; Mip < NumMips; Mip++)
				{
					for (uint32 ArraySlice = 0; ArraySlice < ArraySize; ArraySlice++)
					{
						D3D12_RESOURCE_STATES BeforeDx12State = MapResourceState(GetLocalTextureSubResourceState(DxTex, Mip, ArraySlice));
						if (BeforeDx12State == AfterDx12State)
						{
							if (BarrierInfo.NewState == GpuResourceState::UnorderedAccess) bNeedUAVBarrier = true;
							continue;
						}
						uint32 Subresource = D3D12CalcSubresource(Mip, ArraySlice, 0, NumMips, ArraySize);
						DxBarriers.Add(CD3DX12_RESOURCE_BARRIER::Transition(DxTex->GetResource(), BeforeDx12State, AfterDx12State, Subresource));
					}
				}
				if (bNeedUAVBarrier) DxBarriers.Add(CD3DX12_RESOURCE_BARRIER::UAV(DxTex->GetResource()));
				SetLocalTextureAllSubResourceStates(DxTex, BarrierInfo.NewState);
			}
			else if (BarrierInfo.Resource->GetType() == GpuResourceType::TextureView)
			{
				GpuTextureView* View = static_cast<GpuTextureView*>(BarrierInfo.Resource);
				Dx12Texture* DxTex = static_cast<Dx12Texture*>(View->GetTexture());
				uint32 NumMips = DxTex->GetNumMips();
				uint32 ArraySize = DxTex->GetArrayLayerCount();
				bool bNeedUAVBarrier = false;
				for (uint32 i = 0; i < View->GetMipLevelCount(); i++)
				{
					uint32 Mip = View->GetBaseMipLevel() + i;
					for (uint32 ArraySlice = 0; ArraySlice < ArraySize; ArraySlice++)
					{
						D3D12_RESOURCE_STATES BeforeDx12State = MapResourceState(GetLocalTextureSubResourceState(DxTex, Mip, ArraySlice));
						if (BeforeDx12State == AfterDx12State)
						{
							if (BarrierInfo.NewState == GpuResourceState::UnorderedAccess) bNeedUAVBarrier = true;
							continue;
						}
						uint32 Subresource = D3D12CalcSubresource(Mip, ArraySlice, 0, NumMips, ArraySize);
						DxBarriers.Add(CD3DX12_RESOURCE_BARRIER::Transition(DxTex->GetResource(), BeforeDx12State, AfterDx12State, Subresource));
						SetLocalTextureSubResourceState(DxTex, Mip, ArraySlice, BarrierInfo.NewState);
					}
				}
				if (bNeedUAVBarrier) DxBarriers.Add(CD3DX12_RESOURCE_BARRIER::UAV(DxTex->GetResource()));
			}
			else if (BarrierInfo.Resource->GetType() == GpuResourceType::Buffer)
			{
				Dx12Buffer* DxBuffer = static_cast<Dx12Buffer*>(BarrierInfo.Resource);
				GpuResourceState OldState = GetLocalBufferState(DxBuffer);
				if (OldState == BarrierInfo.NewState)
				{
					if (BarrierInfo.NewState == GpuResourceState::UnorderedAccess)
						DxBarriers.Add(CD3DX12_RESOURCE_BARRIER::UAV(DxBuffer->GetAllocation().GetResource()));
				}
				else
				{
					D3D12_RESOURCE_STATES BeforeDx12State = MapResourceState(OldState);
					DxBarriers.Add(CD3DX12_RESOURCE_BARRIER::Transition(DxBuffer->GetAllocation().GetResource(), BeforeDx12State, AfterDx12State));
				}
				SetLocalBufferState(DxBuffer, BarrierInfo.NewState);
			}
			else
			{
				check(false);
			}
		}

		if (DxBarriers.Num() > 0)
		{
			CmdList->ResourceBarrier(DxBarriers.Num(), DxBarriers.GetData());
		}
	}

	void Dx12CmdRecorder::CopyBufferToTexture(GpuBuffer* InBuffer, GpuTexture* InTexture, uint32 ArrayLayer, uint32 MipLevel)
	{
		Dx12Texture* Texture = static_cast<Dx12Texture*>(InTexture);
		Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(InBuffer);

		const uint32 Subresource = D3D12CalcSubresource(MipLevel, ArrayLayer, 0, InTexture->GetResourceDesc().NumMips, Texture->GetResource()->GetDesc().DepthOrArraySize);
        D3D12_RESOURCE_DESC Desc = Texture->GetResource()->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout{};
		GDevice->GetCopyableFootprints(&Desc, Subresource, 1, 0, &Layout, nullptr, nullptr, nullptr);

		const CommonAllocationData& AllocationData = Buffer->GetAllocation().GetAllocationData().Get<CommonAllocationData>();
        CD3DX12_TEXTURE_COPY_LOCATION DestLoc{ Texture->GetResource(), Subresource };
        CD3DX12_TEXTURE_COPY_LOCATION SrcLoc{ AllocationData.UnderlyResource, Layout };
		CmdList->CopyTextureRegion(&DestLoc, 0, 0, 0, &SrcLoc, nullptr);
	}

	void Dx12CmdRecorder::CopyTextureToBuffer(GpuTexture* InTexture, GpuBuffer* InBuffer)
	{
        Dx12Texture* Texture = static_cast<Dx12Texture*>(InTexture);
        Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(InBuffer);

        D3D12_RESOURCE_DESC Desc = Texture->GetResource()->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout{};
        GDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Layout, nullptr, nullptr, nullptr);

        const CommonAllocationData& AllocationData = Buffer->GetAllocation().GetAllocationData().Get<CommonAllocationData>();
        CD3DX12_TEXTURE_COPY_LOCATION DestLoc{ AllocationData.UnderlyResource, Layout };
        CD3DX12_TEXTURE_COPY_LOCATION SrcLoc{ Texture->GetResource() };
		CmdList->CopyTextureRegion(&DestLoc, 0, 0, 0, &SrcLoc, nullptr);
	}

	void Dx12CmdRecorder::CopyBufferToBuffer(GpuBuffer* SrcBuffer, uint32 SrcOffset, GpuBuffer* DestBuffer, uint32 DestOffset, uint32 Size)
	{
		//TODO:Is it necessary to support the sub-allocated constant buffer?
		Dx12Buffer* SrcDxBuffer = static_cast<Dx12Buffer*>(SrcBuffer);
		Dx12Buffer* DestDxBuffer = static_cast<Dx12Buffer*>(DestBuffer);
		CmdList->CopyBufferRegion(DestDxBuffer->GetAllocation().GetResource(), DestOffset, SrcDxBuffer->GetAllocation().GetResource(), SrcOffset, Size);
	}

	void Dx12CmdRecorder::Reset()
	{
		check(IsFree());
		DxCheck(CommandAllocator->Reset());
		DxCheck(CmdList->Reset(CommandAllocator, nullptr));
		DxCheck(Fence->Signal(NotSubmitted));
		RequestedRenderPassRecorders.Empty();
		StateCache.Clear();
		BindDescriptorHeap();
		IsUnsubmittedAtFrameEnd = false;
		IsEnd = false;
	}

	void Dx12CmdRecorder::SetName(const FString& Name)
	{
		CmdList->SetName(*Name);
		CommandAllocator->SetName(*Name);
	}

	void Dx12CmdRecorder::BindDescriptorHeap()
	{
		ID3D12DescriptorHeap* Heaps[] = {
			Gpu_CbvSrvUavAllocator->GetDescriptorHeap(),
			Gpu_SamplerAllocator->GetDescriptorHeap()
		};
		CmdList->SetDescriptorHeaps(UE_ARRAY_COUNT(Heaps), Heaps);
	}

}
