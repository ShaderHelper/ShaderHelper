#include "CommonHeader.h"
#include "PrintBuffer.h"
#include "Shared/Print.h"
#include "GpuApi/GpuResourceHelper.h"

namespace FW
{
	PrintBuffer::PrintBuffer()
	{
		TArray<uint8> Datas;
		Datas.SetNumZeroed(sizeof(HLSL::Printer));
		InternalBuffer = GGpuRhi->CreateBuffer({
			.ByteSize = sizeof(HLSL::Printer),
			.Usage = GpuBufferUsage::RWStorage,
			.Stride = sizeof(HLSL::Printer),
			.InitialData = Datas
		});
	}

	TArray<FString> PrintBuffer::GetPrintStrings()
	{
	
		HLSL::Printer* Printer = (HLSL::Printer*)GGpuRhi->MapGpuBuffer(InternalBuffer, GpuResourceMapMode::Read_Only);
		GGpuRhi->UnMapGpuBuffer(InternalBuffer);

		return {};
	}

	void PrintBuffer::Clear()
	{
		auto CmdRecorder = GGpuRhi->BeginRecording();
		{
			GpuResourceHelper::ClearRWResource(CmdRecorder, InternalBuffer);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });
	}
}