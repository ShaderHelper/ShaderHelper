#include "CommonHeader.h"
#include "GpuApi/GpuApiInterface.h"
#include "D3D12Device.h"
#include "D3D12CommandList.h"
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
		uint32 FrameResourceIndex = CurCpuFrame % FrameSourceNum;
		GCommandListContext->Reset(FrameResourceIndex);
	}
}
}