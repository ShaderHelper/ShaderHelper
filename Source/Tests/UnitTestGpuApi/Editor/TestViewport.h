#pragma once
#include "Editor/PreviewViewPort.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuResource.h"

namespace UNITTEST_GPUAPI
{
	class TestViewport : public FW::PreviewViewPort
	{
	public:
		TestViewport(const FW::Vector2f& InSize, FW::GpuTextureFormat InFormat);

		void Resize(const FW::Vector2f& InSize);
		FW::GpuTexture* GetRenderTarget() const { return RenderTarget.GetReference(); }
		FW::GpuTextureFormat GetRenderTargetFormat() const { return RenderTargetFormat; }

	private:
		void CreateRenderTarget(const FW::Vector2f& InSize);

	public:
		TFunction<void(TestViewport*)> RenderFunc;

	private:
		TRefCountPtr<FW::GpuTexture> RenderTarget;
		FW::GpuTextureFormat RenderTargetFormat;
	};
}
