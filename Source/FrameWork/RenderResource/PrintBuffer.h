#pragma once
#include "GpuApi/GpuRhi.h"

namespace FW
{
	struct ShaderAssertInfo
	{
		FString AssertString;
		int LineNumber;
	};

	class FRAMEWORK_API PrintBuffer
	{
	public:
		PrintBuffer();

	public:
		TArray<FString> GetPrintStrings(ShaderAssertInfo& OutAssertInfo);
		void Clear();
		GpuBuffer* GetResource() const { return InternalBuffer; }

	private:
		TRefCountPtr<GpuBuffer> InternalBuffer;
	};
}
