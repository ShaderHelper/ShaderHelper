#pragma once
#include "D3D12Common.h"
namespace FRAMEWORK
{
	class Dx12Device
	{
	protected:
		Dx12Device() = default;
	public:
		static TUniquePtr<Dx12Device> CreateDevice();
	private:
		bool Init();

	private:
		TRefCountPtr<IDXGIFactory2> DxgiFactory;
		TRefCountPtr<ID3D12Device> RawDevice;
	};

	inline TUniquePtr<Dx12Device> GDx12Device;
}