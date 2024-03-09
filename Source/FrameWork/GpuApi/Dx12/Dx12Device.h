#pragma once
#include "Dx12Common.h"

namespace FRAMEWORK
{

	inline bool GCanGpuCapture;
	inline ID3D12Device* GDevice;
	inline IDXGIFactory2* GDxgiFactory;
	inline ID3D12CommandQueue* GGraphicsQueue;
	inline ID3D12Fence* CpuSyncGpuFence;
	inline HANDLE CpuSyncGpuEvent;
	inline constexpr uint32 AllowableLag = 2;
	inline constexpr uint32 FrameSourceNum = AllowableLag + 1;
	inline uint64 CurCpuFrame;
	inline uint64 CurGpuFrame;

    extern void InitDx12Core();
	inline uint32 GetCurFrameSourceIndex() {
		return CurCpuFrame % FrameSourceNum;
	}
 	
}
