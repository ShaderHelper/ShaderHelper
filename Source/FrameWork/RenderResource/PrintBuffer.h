#pragma once
#include "GpuApi/GpuRhi.h"

DECLARE_LOG_CATEGORY_EXTERN(LogShaderPrint, Log, All);
inline DEFINE_LOG_CATEGORY(LogShaderPrint);

namespace FW
{
	class FRAMEWORK_API PrintBuffer
	{
	public:
		PrintBuffer();

	public:
		TArray<FString> GetPrintStrings();
		void Clear();
		GpuBuffer* GetResource() const { return InternalBuffer; }

	private:
		TRefCountPtr<GpuBuffer> InternalBuffer;
	};
}