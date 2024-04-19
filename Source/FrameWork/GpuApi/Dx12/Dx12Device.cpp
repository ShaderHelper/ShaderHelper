#include "CommonHeader.h"
#include "Dx12Device.h"
#include <dxgidebug.h>
#include "GpuApi/GpuFeature.h"

namespace FRAMEWORK
{
	static D3D_SHADER_MODEL GetMaxShaderModel(ID3D12Device* InDevice)
	{
		D3D_SHADER_MODEL ShaderModelsToCheck[] =
		{
			D3D_SHADER_MODEL_6_6,
			D3D_SHADER_MODEL_6_5,
			D3D_SHADER_MODEL_6_4,
			D3D_SHADER_MODEL_6_3,
			D3D_SHADER_MODEL_6_2,
			D3D_SHADER_MODEL_6_1,
			D3D_SHADER_MODEL_6_0,
		};

		D3D12_FEATURE_DATA_SHADER_MODEL DataShaderModel;
		for (auto ShaderModelToCheck : ShaderModelsToCheck)
		{
			DataShaderModel.HighestShaderModel = ShaderModelToCheck;
			if (SUCCEEDED(GDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &DataShaderModel, sizeof(DataShaderModel))))
			{
				return DataShaderModel.HighestShaderModel;
			}
		}

		return D3D_SHADER_MODEL_6_0;
	}

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

#if GPU_API_DEBUG
		bool bEnableDred = FParse::Param(FCommandLine::Get(), TEXT("DxDred"));
		bool bEnableGBV = FParse::Param(FCommandLine::Get(), TEXT("DxGBV"));
		bool bEnableDebugLayer = FParse::Param(FCommandLine::Get(), TEXT("DxDebugLayer"));
		bEnableDebugLayer |= bEnableGBV;

		TRefCountPtr<ID3D12Debug> Debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(Debug.GetInitReference())))) {
			if (bEnableDebugLayer) {

				Debug->EnableDebugLayer();
				if (bEnableGBV)
				{
					TRefCountPtr<ID3D12Debug1> Debug1;
					DxCheck(Debug->QueryInterface(IID_PPV_ARGS(Debug1.GetInitReference())));
					Debug1->SetEnableGPUBasedValidation(true);
				}

				TRefCountPtr<IDXGIInfoQueue> DxgiInfoQueue;
				if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(DxgiInfoQueue.GetInitReference()))))
				{
					DXGI_INFO_QUEUE_MESSAGE_CATEGORY DisabledCategory[] =
					{
						DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_CREATION,
					};

					DXGI_INFO_QUEUE_FILTER Filter{};
					Filter.DenyList.NumCategories = UE_ARRAY_COUNT(DisabledCategory);
					Filter.DenyList.pCategoryList = DisabledCategory;
					DxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &Filter);

					DxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_DXGI, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
					DxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_DXGI, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
				}
			}

			if (bEnableDred || bEnableDebugLayer)
			{
				TRefCountPtr<ID3D12Debug5> Debug5;
				if (SUCCEEDED(Debug->QueryInterface(IID_PPV_ARGS(Debug5.GetInitReference()))))
				{
					Debug5->SetEnableAutoName(true);
				}
			}
		}

		if (bEnableDred)
		{
			HMODULE D3d12DllHandle = (HMODULE)FPlatformProcess::GetDllHandle(TEXT("d3d12.dll"));
			if (D3d12DllHandle)
			{
				PFN_D3D12_GET_INTERFACE GetInterfacePtr = (PFN_D3D12_GET_INTERFACE)(void*)GetProcAddress(D3d12DllHandle, "D3D12GetInterface");
				if (GetInterfacePtr)
				{
					TRefCountPtr<ID3D12DeviceRemovedExtendedDataSettings2> DredSettings;
					if (SUCCEEDED(GetInterfacePtr(CLSID_D3D12DeviceRemovedExtendedData, IID_PPV_ARGS(DredSettings.GetInitReference()))))
					{
						DredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
						DredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
						DredSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
						DredSettings->UseMarkersOnlyAutoBreadcrumbs(true);
						GDred = true;
					}
				}
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

		DxCheck(D3D12CreateDevice(Adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&GDevice)));
		GMaxShaderModel = GetMaxShaderModel(GDevice);
		if (GMaxShaderModel >= D3D_SHADER_MODEL_6_2)
		{
			GpuFeature::Support16bitType = true;
		}

#if GPU_API_DEBUG
		if (bEnableDebugLayer) {
			TRefCountPtr<ID3D12InfoQueue> D3dInfoQueue;
			if (SUCCEEDED(GDevice->QueryInterface(IID_PPV_ARGS(D3dInfoQueue.GetInitReference()))))
			{
				D3D12_MESSAGE_ID DisabledMessageId[] =
				{
					D3D12_MESSAGE_ID_CREATERESOURCE_STATE_IGNORED,
				};
				D3D12_MESSAGE_CATEGORY DisabledCategory[] =
				{
					D3D12_MESSAGE_CATEGORY_STATE_CREATION,
				};
				D3D12_INFO_QUEUE_FILTER Filter{};
				Filter.DenyList.NumCategories = UE_ARRAY_COUNT(DisabledCategory);
				Filter.DenyList.pCategoryList = DisabledCategory;
				Filter.DenyList.NumIDs = UE_ARRAY_COUNT(DisabledMessageId);
				Filter.DenyList.pIDList = DisabledMessageId;
				D3dInfoQueue->AddStorageFilterEntries(&Filter);

				D3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
				D3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			
			}
		}
#endif

		DxCheck(GDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&CpuSyncGpuFence)));
		CpuSyncGpuFence->SetName(TEXT("CpuSyncGpuFence"));
		CpuSyncGpuEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		D3D12_COMMAND_QUEUE_DESC QueueDesc{};
		QueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		QueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		DxCheck(GDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&GGraphicsQueue)));
		GGraphicsQueue->SetName(TEXT("Dx12GlobalGraphicsQueue"));
	}

}