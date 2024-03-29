#include "CommonHeader.h"
#include "GpuApi/GpuApiInterface.h"
#include "GpuApi/GpuApiValidation.h"
#include "Dx12Device.h"
#include "Dx12CommandList.h"
#include "Dx12Texture.h"
#include "Dx12Shader.h"
#include "Dx12PSO.h"
#include "Dx12Map.h"
#include "Dx12Allocation.h"
#include "Dx12RS.h"

namespace FRAMEWORK
{
namespace GpuApi
{
	void InitApiEnv()
	{	
		InitDx12Core();
		InitCommandListContext();
		InitBufferAllocator();
	}

	void BeginFrame()
	{
		uint32 FrameResourceIndex = GetCurFrameSourceIndex();
		GTempUniformBufferAllocator[FrameResourceIndex]->Flush();
        GDeferredReleaseManager->AllocateOneFrame();
	}

	void EndFrame()
	{
		//If there are recorded commands between last Submit() and EndFrame()
		if (!GCommandListContext->IsClose())
		{
			Submit();
		}

		check(CurCpuFrame >= CurGpuFrame);
		CurCpuFrame++;
		DxCheck(GGraphicsQueue->Signal(CpuSyncGpuFence, CurCpuFrame));
		
		CurGpuFrame = CpuSyncGpuFence->GetCompletedValue();
		const uint64 CurLag = CurCpuFrame - CurGpuFrame;
		if (CurLag > AllowableLag) {
			//Cpu is waiting for gpu to catch up a frame.
			DxCheck(CpuSyncGpuFence->SetEventOnCompletion(CurGpuFrame + 1, CpuSyncGpuEvent));
			WaitForSingleObject(CpuSyncGpuEvent, INFINITE);
			CurGpuFrame = CurGpuFrame + 1;
		}

        GDeferredReleaseManager->ReleaseCompletedResources();
	}

	TRefCountPtr<GpuTexture> CreateTexture(const GpuTextureDesc& InTexDesc)
	{
		return AUX::StaticCastRefCountPtr<GpuTexture>(CreateDx12Texture2D(InTexDesc));
	}

	void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch)
	{
		Dx12Texture* Texture = static_cast<Dx12Texture*>(InGpuTexture);
		void* Data{};
		if (InMapMode == GpuResourceMapMode::Write_Only) {
			const uint32 BufferSize = (uint32)GetRequiredIntermediateSize(Texture->GetResource(), 0, 1);
			if (!Texture->UploadBuffer) {
				Texture->UploadBuffer = CreateDx12Buffer(BufferSize, GpuBufferUsage::Dynamic);
			}
			Data = Texture->UploadBuffer->GetAllocation().GetCpuAddr();
			Texture->bIsMappingForWriting = true;
		}
		else if (InMapMode == GpuResourceMapMode::Read_Only) {
			const uint32 BufferSize = (uint32)GetRequiredIntermediateSize(Texture->GetResource(), 0, 1);
			if (!Texture->ReadBackBuffer) {
				Texture->ReadBackBuffer = CreateDx12Buffer(BufferSize, GpuBufferUsage::Staging);
			}
			Data = Texture->ReadBackBuffer->GetAllocation().GetCpuAddr();
			
			ScopedBarrier Barrier{ Texture, D3D12_RESOURCE_STATE_COPY_SOURCE };
			ID3D12GraphicsCommandList* CommandListHandle = GCommandListContext->GetCommandListHandle();

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout{};
			D3D12_RESOURCE_DESC Desc = Texture->GetResource()->GetDesc();
			GDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Layout, nullptr, nullptr, nullptr);

			const CommonAllocationData& AllocationData = Texture->ReadBackBuffer->GetAllocation().GetAllocationData().Get<CommonAllocationData>();
			CD3DX12_TEXTURE_COPY_LOCATION DestLoc{ AllocationData.UnderlyResource, Layout};
			CD3DX12_TEXTURE_COPY_LOCATION SrcLoc{ Texture->GetResource() };
			CommandListHandle->CopyTextureRegion(&DestLoc, 0, 0, 0, &SrcLoc, nullptr);

			//To make sure ReadBackBuffer finished copying, so cpu can read the mapped data.
			FlushGpu();
		}
		OutRowPitch = Align(InGpuTexture->GetWidth() * GetTextureFormatByteSize(InGpuTexture->GetFormat()), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		return Data;
	}

	void UnMapGpuTexture(GpuTexture* InGpuTexture)
	{
		Dx12Texture* Texture = static_cast<Dx12Texture*>(InGpuTexture);
		if (Texture->bIsMappingForWriting) {
			ScopedBarrier Barrier{ Texture, D3D12_RESOURCE_STATE_COPY_DEST };
			ID3D12GraphicsCommandList* CommandListHandle = GCommandListContext->GetCommandListHandle();

			CD3DX12_TEXTURE_COPY_LOCATION DestLoc{ Texture->GetResource() };

			D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout{};
			D3D12_RESOURCE_DESC Desc = Texture->GetResource()->GetDesc();
			GDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Layout, nullptr, nullptr, nullptr);

			const CommonAllocationData& AllocationData = Texture->UploadBuffer->GetAllocation().GetAllocationData().Get<CommonAllocationData>();
			CD3DX12_TEXTURE_COPY_LOCATION SrcLoc{ AllocationData.UnderlyResource, Layout };
			CommandListHandle->CopyTextureRegion(&DestLoc, 0, 0, 0, &SrcLoc, nullptr);
				
			Texture->bIsMappingForWriting = false;
		}
	}

	void* MapGpuBuffer(GpuBuffer* InGpuBuffer, GpuResourceMapMode InMapMode)
	{
		GpuBufferUsage Usage = InGpuBuffer->GetUsage();
		Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(InGpuBuffer);
		void* Data = nullptr;

		if (EnumHasAnyFlags(Usage, GpuBufferUsage::Dynamic))
		{
			check(InMapMode == GpuResourceMapMode::Write_Only);
			Data = Buffer->GetAllocation().GetCpuAddr();
		}
		else
		{

		}

		return Data;
	}

	void UnMapGpuBuffer(GpuBuffer* InGpuBuffer)
	{

	}

	TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName, FString EntryPoint)
	{
		return AUX::StaticCastRefCountPtr<GpuShader>(CreateDx12Shader(InType, MoveTemp(InSourceText), MoveTemp(InShaderName), MoveTemp(EntryPoint)));
	}

	TRefCountPtr<GpuShader> CreateShaderFromFile(FString FileName, ShaderType InType, FString EntryPoint, FString ExtraDeclaration)
	{
		return AUX::StaticCastRefCountPtr<GpuShader>(CreateDx12Shader(MoveTemp(FileName), InType, MoveTemp(ExtraDeclaration), MoveTemp(EntryPoint)));
	}

	TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc& InBindGroupDesc)
	{
		check(ValidateCreateBindGroup(InBindGroupDesc));
		return AUX::StaticCastRefCountPtr<GpuBindGroup>(CreateDx12BindGroup(InBindGroupDesc));
	}

	TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc& InBindGroupLayoutDesc)
	{
        check(ValidateCreateBindGroupLayout(InBindGroupLayoutDesc));
		return AUX::StaticCastRefCountPtr<GpuBindGroupLayout>(CreateDx12BindGroupLayout(InBindGroupLayoutDesc));
	}

	TRefCountPtr<GpuBuffer> CreateBuffer(uint32 ByteSize, GpuBufferUsage Usage)
	{
		return AUX::StaticCastRefCountPtr<GpuBuffer>(CreateDx12Buffer(ByteSize, Usage));
	}

	TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc& InSamplerDesc)
	{
		return AUX::StaticCastRefCountPtr<GpuSampler>(CreateDx12Sampler(InSamplerDesc));
	}

    bool CrossCompileShader(GpuShader* InShader, FString& OutErrorInfo)
    {
        return GShaderCompiler.Compile(static_cast<Dx12Shader*>(InShader), OutErrorInfo);
    }

	TRefCountPtr<GpuPipelineState> CreateRenderPipelineState(const GpuPipelineStateDesc& InPipelineStateDesc)
	{
		check(ValidateCreateRenderPipelineState(InPipelineStateDesc));
        return AUX::StaticCastRefCountPtr<GpuPipelineState>(CreateDx12Pso(InPipelineStateDesc));
	}

	void SetRenderPipelineState(GpuPipelineState* InPipelineState)
	{
		GCommandListContext->SetPipeline(static_cast<Dx12Pso*>(InPipelineState));
	}

	void SetVertexBuffer(GpuBuffer* InVertexBuffer)
	{
		Dx12Buffer* Vb = static_cast<Dx12Buffer*>(InVertexBuffer);
		GCommandListContext->SetVertexBuffer(Vb);
	}

	void SetViewPort(const GpuViewPortDesc& InViewPortDesc)
	{
		D3D12_VIEWPORT ViewPort{};
		ViewPort.Width = InViewPortDesc.Width;
		ViewPort.Height = InViewPortDesc.Height;
		ViewPort.MinDepth = InViewPortDesc.ZMin;
		ViewPort.MaxDepth = InViewPortDesc.ZMax;
		ViewPort.TopLeftX = InViewPortDesc.TopLeftX;
		ViewPort.TopLeftY = InViewPortDesc.TopLeftY;

		D3D12_RECT ScissorRect = CD3DX12_RECT(0, 0, (LONG)InViewPortDesc.Width, (LONG)InViewPortDesc.Height);
		GCommandListContext->SetViewPort(MoveTemp(ViewPort), MoveTemp(ScissorRect));
	}

	void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
		check(ValidateSetBindGroups(BindGroup0, BindGroup1, BindGroup2, BindGroup3));

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

		GCommandListContext->SetRootSignature(Dx12RootSignatureManager::GetRootSignature(RsDesc));
		GCommandListContext->SetBindGroups(Dx12BindGroup0, Dx12BindGroup1, Dx12BindGroup2, Dx12BindGroup3);
	}

	void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount)
	{
		GCommandListContext->PrepareDrawingEnv();
		GCommandListContext->GetCommandListHandle()->DrawInstanced(VertexCount, InstanceCount, StartVertexLocation, StartInstanceLocation);
	}

	void Submit()
	{
		GCommandListContext->SubmitCommandList();
		GCommandListContext->ClearBinding();
	}

	void FlushGpu()
	{
		Submit();
		DxCheck(GGraphicsQueue->Signal(CpuSyncGpuFence, CurCpuFrame + 114514));
		DxCheck(CpuSyncGpuFence->SetEventOnCompletion(CurCpuFrame + 114514, CpuSyncGpuEvent));
		WaitForSingleObject(CpuSyncGpuEvent, INFINITE);
		CpuSyncGpuFence->Signal(CurCpuFrame);
		CurGpuFrame = CurCpuFrame;

        GDeferredReleaseManager->ReleaseCompletedResources();
	}

	void BeginGpuCapture(const FString& SavedFileName)
	{
#if USE_PIX
		if (GCanGpuCapture) {
			PIXCaptureParameters params = {};
			FString FileName = FString::Format(TEXT("{0}.wpix"), { SavedFileName});
			params.GpuCaptureParameters.FileName = *FileName;
			FlushGpu();
			PIXBeginCapture(PIX_CAPTURE_GPU, &params);
		}
#endif
	}

	void EndGpuCapture()
	{
#if USE_PIX
		if (GCanGpuCapture) {
			FlushGpu();
			PIXEndCapture(false);
            FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Successfully captured the current frame."), TEXT("Message:"));
		}
#endif
	}

	void BeginCaptureEvent(const FString& EventName)
	{
#if USE_PIX
		PIXBeginEvent(GCommandListContext->GetCommandListHandle(), PIX_COLOR_DEFAULT, *EventName);
#endif
	}

	void EndCpatureEvent()
	{
#if USE_PIX
		PIXEndEvent(GCommandListContext->GetCommandListHandle());
#endif
	}

	void* GetSharedHandle(GpuTexture* InGpuTexture)
	{
		return static_cast<Dx12Texture*>(InGpuTexture)->GetSharedHandle();
	}

    void BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName)
    {
        GpuApi::BeginCaptureEvent(PassName);
        
        //To follow metal api design, we don't keep previous the bindings when beginning a new render pass.
        GCommandListContext->ClearBinding();
        
        TArray<Dx12Texture*> RTs;
        TArray<TOptional<Vector4f>> ClearColorValues;
        
        for(int32 i = 0; i < PassDesc.ColorRenderTargets.Num(); i++)
        {
            Dx12Texture* Rt = static_cast<Dx12Texture*>(PassDesc.ColorRenderTargets[i].Texture);
            RTs.Add(Rt);
            ClearColorValues.Add(PassDesc.ColorRenderTargets[i].ClearColor);
        }
        
        GCommandListContext->SetRenderTargets(MoveTemp(RTs), MoveTemp(ClearColorValues));    
    }

    void EndRenderPass()
    {
        GpuApi::EndCpatureEvent();
    }
        
}
}
