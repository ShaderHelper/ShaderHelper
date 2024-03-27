#include "CommonHeader.h"
#include "Dx12Common.h"
#include "Dx12Device.h"

namespace FRAMEWORK
{

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

		SH_LOG(LogDx12, Fatal, TEXT("DxError(%s) encountered during calling %s.(%s-%u)"), *ErrorCodeText, ANSI_TO_TCHAR(Code), ANSI_TO_TCHAR(Filename), Line);
		std::_Exit(0);
	}

}