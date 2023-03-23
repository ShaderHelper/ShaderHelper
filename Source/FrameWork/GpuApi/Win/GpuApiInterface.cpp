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

	void StartFrame()
	{
		uint32 FrameResourceIndex = GetCurFrameSourceIndex();
		GCommandListContext->BindFrameResource(FrameResourceIndex);
	}

	void EndFrame()
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

	void* MapGpuTexture(TRefCountPtr<GpuTextureResource> InGpuTexture, TextureMapMode InMapMode)
	{

	}

	void UnMapGpuTexture(TRefCountPtr<GpuTextureResource> InGpuTexture)
	{

	}
}
}