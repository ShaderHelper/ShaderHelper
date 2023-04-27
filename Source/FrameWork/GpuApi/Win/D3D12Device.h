#pragma once
#include "D3D12Common.h"

namespace FRAMEWORK
{

	inline bool GCanGpuCapture = false;
	inline TRefCountPtr<ID3D12Device> GDevice;
	inline TRefCountPtr<IDXGIFactory2> GDxgiFactory;
	inline TRefCountPtr<ID3D12CommandQueue> GGraphicsQueue;
	inline TRefCountPtr<ID3D12Fence> CpuSyncGpuFence;
	inline HANDLE CpuSyncGpuEvent;
	inline constexpr uint32 AllowableLag = 2;
	inline constexpr uint32 FrameSourceNum = AllowableLag;
	inline uint64 CurCpuFrame;
	inline uint64 CurGpuFrame;

    extern void InitDx12Core();
	inline uint32 GetCurFrameSourceIndex() {
		return CurCpuFrame % FrameSourceNum;
	}
 	
}
