#include "CommonHeader.h"
#include "Dx12Common.h"
#include "Dx12Device.h"

namespace FW
{
#if GPU_API_DEBUG
	FString GetDredInfo()
	{
		FString DredInfo;
		TRefCountPtr<ID3D12DeviceRemovedExtendedData1> DredData;
		if (SUCCEEDED(GDevice->QueryInterface(IID_PPV_ARGS(DredData.GetInitReference()))))
		{
			D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 DredAutoBreadcrumbsOutput;
			if (SUCCEEDED(DredData->GetAutoBreadcrumbsOutput1(&DredAutoBreadcrumbsOutput)))
			{
				const D3D12_AUTO_BREADCRUMB_NODE1* HeadAutoBreadcrumbNode = DredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;
				//TODO, pCommandlistdebugname always is nullptr and pLastBreadcrumbValue 0.
			}
			D3D12_DRED_PAGE_FAULT_OUTPUT DreaPageFaultOutput;
			if (SUCCEEDED(DredData->GetPageFaultAllocationOutput(&DreaPageFaultOutput)))
			{

			}
		}
		return DredInfo;
	}
#endif

	void OutputDxError(HRESULT hr, const ANSICHAR* Code, const ANSICHAR* Filename, uint32 Line)
	{
		FString ErrorCodeText = GetErrorText(hr);
		if (hr == E_OUTOFMEMORY)
		{
			TRefCountPtr<IDXGIAdapter3> Adapter3;
			if (SUCCEEDED(Adapter->QueryInterface(IID_PPV_ARGS(Adapter3.GetInitReference()))))
			{
				DXGI_QUERY_VIDEO_MEMORY_INFO LocalGpuMemInfo{}, NonLocalGpuMemInfo{};
				DxCheck(Adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &LocalGpuMemInfo));
				DxCheck(Adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &NonLocalGpuMemInfo));
				FString LocalGpuMemDebugInfo = FString::Printf(TEXT("Local gpu memory: budget=%.2f mb usage=%.2f mb."), LocalGpuMemInfo.Budget / (1024.0f * 1024), LocalGpuMemInfo.CurrentUsage / (1024.0f * 1024));
				FString NonLocalGpuMemDebugInfo = FString::Printf(TEXT("Non-local gpu memory: budget=%.2f mb usage=%.2f mb."), NonLocalGpuMemInfo.Budget / (1024.0f * 1024), NonLocalGpuMemInfo.CurrentUsage / (1024.0f * 1024));
				

				FString MessageInfo = FString::Printf(TEXT("Out of gpu memory\n"
					"----------------------------------------------------------------------\n"
					"%s\n%s\n "
					"----------------------------------------------------------------------\n"
					"Please make sure you have the enough memory."), 
					*LocalGpuMemDebugInfo, *NonLocalGpuMemDebugInfo);
				FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *MessageInfo, TEXT("Error:"));
			}
			else
			{
				FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Out of gpu memory. Please make sure you have the enough memory."), TEXT("Error:"));
			}
		}

		if (hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			hr = GDevice->GetDeviceRemovedReason();
			if (GDred)
			{
#if GPU_API_DEBUG
				GetDredInfo();
#endif
			}
			SH_LOG(LogDx12, Error, TEXT("DxError(%s) with reason(%s) encountered during calling %s.(%s-%u)"), *ErrorCodeText, GetErrorText(hr), ANSI_TO_TCHAR(Code), ANSI_TO_TCHAR(Filename), Line);
		}
		else
		{
			SH_LOG(LogDx12, Error, TEXT("DxError(%s) encountered during calling %s.(%s-%u)"), *ErrorCodeText, ANSI_TO_TCHAR(Code), ANSI_TO_TCHAR(Filename), Line);
		}
	}

}