#pragma once
#include "D3D12Common.h"

namespace FRAMEWORK
{
	extern void InitDx12Core();

	inline TRefCountPtr<ID3D12Device> GDevice;
	inline TRefCountPtr<IDXGIFactory2> GDxgiFactory;
	inline TRefCountPtr<ID3D12CommandQueue> GGraphicsQueue;
	inline TRefCountPtr<ID3D12Fence> CpuSyncGpuFence;
	inline HANDLE CpuSyncGpuEvent;
	inline constexpr uint32 AllowableLag = 2;
	inline constexpr uint32 FrameSourceNum = AllowableLag + 1;
	inline uint64 CurCpuFrame;
	inline uint64 CurGpuFrame;
 	
}