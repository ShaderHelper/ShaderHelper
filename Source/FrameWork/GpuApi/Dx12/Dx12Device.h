#pragma once
#include "Dx12Common.h"

namespace FW
{

	inline bool GDred;
	inline bool GCanPIXCapture;
	inline ID3D12Device* GDevice;
	inline ID3D12CompatibilityDevice* GCompatDevice;
	inline IDXGIFactory2* GDxgiFactory;
	inline IDXGIAdapter1* Adapter;
	inline ID3D12CommandQueue* GGraphicsQueue;
	inline ID3D12Fence* CpuSyncGpuFence;
	inline HANDLE CpuSyncGpuEvent;
	inline constexpr uint32 AllowableLag = 2;
	inline constexpr uint32 FrameSourceNum = AllowableLag + 1;
	inline uint64 CurCpuFrame;
	inline uint64 CurGpuFrame;
	inline D3D_SHADER_MODEL GMaxShaderModel;

    extern void InitDx12Core();
	inline uint32 GetCurFrameSourceIndex() {
		return CurCpuFrame % FrameSourceNum;
	}
 	
}
