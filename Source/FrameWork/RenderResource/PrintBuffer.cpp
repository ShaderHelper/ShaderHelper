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
		TArray<FString> PrintStrings;
		HLSL::Printer* Printer = (HLSL::Printer*)GGpuRhi->MapGpuBuffer(InternalBuffer, GpuResourceMapMode::Read_Only);
		GGpuRhi->UnMapGpuBuffer(InternalBuffer);
		uint32 ByteOffset = 4;
		while (ByteOffset < Printer->ByteSize)
		{
			FString PrintStr{ (char*)Printer + ByteOffset };
			ByteOffset += PrintStr.Len() + 1;
			uint8 ArgNum = *((uint8*)Printer + ByteOffset);
			ByteOffset += 1;
			TArray<FStringFormatArg> Args;
			while (ArgNum--)
			{
				HLSL::TypeTag Tag = static_cast<HLSL::TypeTag>(*((uint8*)Printer + ByteOffset));
				ByteOffset += 1;
				void* ArgValue = (uint8*)Printer + ByteOffset;
				if (Tag == HLSL::Print_float)
				{
					float Arg = *(float*)ArgValue;
					Args.Add(Arg);
					ByteOffset += sizeof(float);
				}
				else if (Tag == HLSL::Print_float2)
				{
					Vector2f Arg = *(Vector2f*)ArgValue;
					Args.Add(Arg.ToString());
					ByteOffset += sizeof(Vector2f);
				}
				else if (Tag == HLSL::Print_float3)
				{
					Vector3f Arg = *(Vector3f*)ArgValue;
					Args.Add(Arg.ToString());
					ByteOffset += sizeof(Vector3f);
				}
				else if (Tag == HLSL::Print_float4)
				{
					Vector4f Arg = *(Vector4f*)ArgValue;
					Args.Add(Arg.ToString());
					ByteOffset += sizeof(Vector4f);
				}
				else if (Tag == HLSL::Print_int)
				{
					int Arg = *(int*)ArgValue;
					Args.Add(Arg);
					ByteOffset += sizeof(int);
				}
				else if (Tag == HLSL::Print_int2)
				{
					Vector2i Arg = *(Vector2i*)ArgValue;
					Args.Add(Arg.ToString());
					ByteOffset += sizeof(Vector2i);
				}
				else if (Tag == HLSL::Print_int3)
				{
					Vector3i Arg = *(Vector3i*)ArgValue;
					Args.Add(Arg.ToString());
					ByteOffset += sizeof(Vector3i);
				}
				else if (Tag == HLSL::Print_int4)
				{
					Vector4i Arg = *(Vector4i*)ArgValue;
					Args.Add(Arg.ToString());
					ByteOffset += sizeof(Vector4i);
				}
				else if (Tag == HLSL::Print_uint)
				{
					uint32 Arg = *(uint32*)ArgValue;
					Args.Add(Arg);
					ByteOffset += sizeof(uint32);
				}
				else if (Tag == HLSL::Print_uint2)
				{
					Vector2u Arg = *(Vector2u*)ArgValue;
					Args.Add(Arg.ToString());
					ByteOffset += sizeof(Vector2u);
				}
				else if (Tag == HLSL::Print_uint3)
				{
					Vector3u Arg = *(Vector3u*)ArgValue;
					Args.Add(Arg.ToString());
					ByteOffset += sizeof(Vector3u);
				}
				else if (Tag == HLSL::Print_uint4)
				{
					Vector4u Arg = *(Vector4u*)ArgValue;
					Args.Add(Arg.ToString());
					ByteOffset += sizeof(Vector4u);
				}
			}
			PrintStrings.Add(FString::Format(*PrintStr, Args));
		}

		return PrintStrings;
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