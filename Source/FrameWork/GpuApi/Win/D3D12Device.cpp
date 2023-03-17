#include "CommonHeader.h"
#include "D3D12Device.h"
namespace FRAMEWORK
{
	void InitDx12Core()
	{
		bool bEnableDebugLayer = FParse::Param(FCommandLine::Get(), TEXT("DxDebug"));

		DxCheck(CreateDXGIFactory2(bEnableDebugLayer ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(GDxgiFactory.GetInitReference())));
		TRefCountPtr<IDXGIAdapter1> Adapter;
		GDxgiFactory->EnumAdapters1(0, Adapter.GetInitReference());
		
		DXGI_ADAPTER_DESC1 Desc = { };
		Adapter->GetDesc1(&Desc);
		SH_LOG(LogDx12, Display, TEXT("Adapter: %s"), Desc.Description);

		DxCheck(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(GDevice.GetInitReference())));

		if (bEnableDebugLayer) {
			TRefCountPtr<ID3D12Debug> Debug;
			DxCheck(D3D12GetDebugInterface(IID_PPV_ARGS(Debug.GetInitReference())));
			Debug->EnableDebugLayer();
	
			TRefCountPtr<ID3D12InfoQueue> InfoQueue;
			DxCheck(GDevice->QueryInterface(IID_PPV_ARGS(InfoQueue.GetInitReference())));

			D3D12_MESSAGE_ID DisabledMessages[] =
			{
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
				D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,
			};

			D3D12_INFO_QUEUE_FILTER filter{};
			filter.DenyList.NumIDs = UE_ARRAY_COUNT(DisabledMessages);
			filter.DenyList.pIDList = DisabledMessages;
			InfoQueue->AddStorageFilterEntries(&filter);

			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING,true);
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		}

		DxCheck(GDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(CpuSyncGpuFence.GetInitReference())));
		CpuSyncGpuEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		D3D12_COMMAND_QUEUE_DESC QueueDesc{};
		QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		DxCheck(GDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(GGraphicsQueue.GetInitReference())));
	}

}