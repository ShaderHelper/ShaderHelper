#include "CommonHeader.h"
#include "D3D12Device.h"
namespace FRAMEWORK
{
	TUniquePtr<Dx12Device> Dx12Device::CreateDevice()
	{
		bool bEnableDebugLayer = FParse::Param(FCommandLine::Get(), TEXT("DxDebug"));
	
		uint32 FactoryFlag = 0;
		if (bEnableDebugLayer) {
			FactoryFlag = DXGI_CREATE_FACTORY_DEBUG;
		}
		
		TUniquePtr<Dx12Device> device{ new Dx12Device() };
		if (!device->Init()) {
			return TUniquePtr<Dx12Device>{nullptr};
		}
		return device;
	}

	bool Dx12Device::Init()
	{

	}

}