#pragma once
namespace SH
{
	class DebuggableObject
	{
	public:
		virtual TRefCountPtr<FW::GpuTexture> OnStartDebugging() = 0;
		virtual void OnEndDebuggging() = 0;
	};
}
