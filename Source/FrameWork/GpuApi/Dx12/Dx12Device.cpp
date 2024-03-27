#include "CommonHeader.h"
#include "Dx12Device.h"
namespace FRAMEWORK
{
	void InitDx12Core()
	{
#if USE_PIX
		// Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
		// This may happen if the application is launched through the PIX UI. 
		if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
		{
			if (LoadLibrary(GetLatestWinPixGpuCapturerPath().c_str())) {
				GCanGpuCapture = true;
			}
		}
#endif

#if GPU_API_DEBUG_LAYER
		bool bEnableDebugLayer = FParse::Param(FCommandLine::Get(), TEXT("DxDebug"));
		if (bEnableDebugLayer) {
			TRefCountPtr<ID3D12Debug> Debug;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(Debug.GetInitReference())))) {
				Debug->EnableDebugLayer();
			}
		}
		DxCheck(CreateDXGIFactory2(bEnableDebugLayer ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&GDxgiFactory)));
#else
		DxCheck(CreateDXGIFactory2(0, IID_PPV_ARGS(GDxgiFactory.GetInitReference())));
#endif

		GDxgiFactory->EnumAdapters1(0, &Adapter);
		
		DXGI_ADAPTER_DESC1 Desc = { };
		Adapter->GetDesc1(&Desc);
		SH_LOG(LogDx12, Display, TEXT("Adapter: %s"), Desc.Description);

		DxCheck(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&GDevice)));

#if GPU_API_DEBUG_LAYER
		if (bEnableDebugLayer) {
			TRefCountPtr<ID3D12InfoQueue> InfoQueue;
			if (SUCCEEDED(GDevice->QueryInterface(IID_PPV_ARGS(InfoQueue.GetInitReference()))))
			{
				D3D12_MESSAGE_ID DisabledMessages[] =
				{
					D3D12_MESSAGE_ID_CREATERESOURCE_STATE_IGNORED,
				};
				D3D12_INFO_QUEUE_FILTER Filter{};
				Filter.DenyList.NumIDs = UE_ARRAY_COUNT(DisabledMessages);
				Filter.DenyList.pIDList = DisabledMessages;
				InfoQueue->AddStorageFilterEntries(&Filter);
				
				InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			}
		}
#endif

		DxCheck(GDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&CpuSyncGpuFence)));
		CpuSyncGpuEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		D3D12_COMMAND_QUEUE_DESC QueueDesc{};
		QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		DxCheck(GDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&GGraphicsQueue)));
#if GPU_API_DEBUG_LAYER
		GGraphicsQueue->SetName(TEXT("GpuApi-dx12 global graphics queue"));
#endif
	}

}