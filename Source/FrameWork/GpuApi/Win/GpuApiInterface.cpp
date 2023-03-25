#include "CommonHeader.h"
#include "GpuApi/GpuApiInterface.h"
#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12Texture.h"

namespace FRAMEWORK
{
namespace GpuApi
{
	void InitApiEnv()
	{
		InitDx12Core();
		InitFrameResource();
	}

	void FlushGpu()
	{
		
	}

	void StartRenderFrame()
	{
		uint32 FrameResourceIndex = GetCurFrameSourceIndex();
		GCommandListContext->BindFrameResource(FrameResourceIndex);
	}

	void EndRenderFrame()
	{
		check(CurCpuFrame >= CurCpuFrame);
		CurCpuFrame++;
		DxCheck(GGraphicsQueue->Signal(CpuSyncGpuFence, CurCpuFrame));
		const uint64 CurLag = CurCpuFrame - CurGpuFrame;
		if (CurLag > AllowableLag) {
			//Cpu is waiting for gpu to catch up a frame.
			const uint64 FenceValue = CpuSyncGpuFence->GetCompletedValue();
			if (FenceValue < CurGpuFrame + 1) {
				DxCheck(CpuSyncGpuFence->SetEventOnCompletion(CurGpuFrame + 1, CpuSyncGpuEvent));
				WaitForSingleObject(CpuSyncGpuEvent, INFINITE);
			}
			CurGpuFrame++;
		}

		//FrameResource
		uint32 FrameResourceIndex = GetCurFrameSourceIndex();
		GCommandListContext->ResetFrameResource(FrameResourceIndex);
	}

	TRefCountPtr<GpuTextureResource> CreateGpuTexture(const GpuTextureDesc& InTexDesc)
	{
		return AUX::StaticCastRefCountPtr<GpuTextureResource>(CreateDx12Texture(InTexDesc));
	}

	void* MapGpuTexture(TRefCountPtr<GpuTextureResource> InGpuTexture, GpuResourceMapMode InMapMode)
	{
		TRefCountPtr<Dx12Texture> Texture = AUX::StaticCastRefCountPtr<Dx12Texture>(InGpuTexture);
		void* Data{};
		if (InMapMode == GpuResourceMapMode::WRITE_ONLY) {
			const uint64 UploadBufferSize = GetRequiredIntermediateSize(Texture->GetResource(), 0, 1);
			Texture->UploadBuffer = new Dx12UploadBuffer(UploadBufferSize);
			Data = Texture->UploadBuffer->Map();
			Texture->bIsMappingForWriting = true;
		}
		return Data;
	}

	void UnMapGpuTexture(TRefCountPtr<GpuTextureResource> InGpuTexture)
	{
		TRefCountPtr<Dx12Texture> Texture = AUX::StaticCastRefCountPtr<Dx12Texture>(InGpuTexture);
		if (Texture->bIsMappingForWriting) {
			Texture->UploadBuffer->Unmap();
			ScopedBarrier Barrier{ Texture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST };
			ID3D12GraphicsCommandList* CommandListHandle = GCommandListContext->GetCommandListHandle();

			CD3DX12_TEXTURE_COPY_LOCATION DestLoc{ Texture->GetResource() };
			CD3DX12_TEXTURE_COPY_LOCATION SrcLoc{ Texture->UploadBuffer->GetResource() };
			CommandListHandle->CopyTextureRegion(&DestLoc, 0, 0, 0, &SrcLoc,nullptr);
				
			Texture->bIsMappingForWriting = false;
		}
	}
}
}