#pragma once
#include "GpuApi/GpuRhi.h"

namespace FW
{
	struct ShaderAssertInfo
	{
		FString AssertString;
		int Line{};
	};

	struct ShaderPrintInfo
	{
		FString PrintStr;
		int Line{};
	};

	class FRAMEWORK_API PrintBuffer
	{
	public:
		PrintBuffer();

	public:
		TArray<ShaderPrintInfo> GetPrintStrings(ShaderAssertInfo& OutAssertInfo);
		void Clear();
		GpuBuffer* GetResource() const { return InternalBuffer; }

	private:
		TRefCountPtr<GpuBuffer> InternalBuffer;
	};
}
