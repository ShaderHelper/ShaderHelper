#pragma once
namespace SH
{
	class DebuggableObject
	{
	public:
		virtual TRefCountPtr<FW::GpuTexture> OnStartDebugging() = 0;
		virtual void OnFinalizePixel(const FW::Vector2u& PixelCoord) = 0;
		virtual void OnEndDebuggging() = 0;
	};
}
