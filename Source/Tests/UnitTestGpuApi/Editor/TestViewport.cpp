#include "CommonHeader.h"
#include "TestViewport.h"

using namespace FW;

namespace UNITTEST_GPUAPI
{
	TestViewport::TestViewport(const Vector2f& InSize, FW::GpuTextureFormat InFormat)
		: RenderTargetFormat(FW::GpuTextureFormat::B8G8R8A8_UNORM)
	{
		CreateRenderTarget(InSize);
		ResizeHandler.AddLambda([this](const Vector2f& InNewSize) {
			Resize(InNewSize);
		});
	}

	void TestViewport::Resize(const Vector2f& InSize)
	{
		if (InSize.X > 0 && InSize.Y > 0)
		{
			CreateRenderTarget(InSize);
			if (RenderFunc)
			{
				RenderFunc(this);
			}
		}
	}

	void TestViewport::CreateRenderTarget(const Vector2f& InSize)
	{
		SizeX = static_cast<int32>(InSize.X);
		SizeY = static_cast<int32>(InSize.Y);

		GpuTextureDesc Desc{
			static_cast<uint32>(SizeX),
			static_cast<uint32>(SizeY),
			RenderTargetFormat,
			GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
		};
		RenderTarget = GGpuRhi->CreateTexture(Desc, GpuResourceState::RenderTargetWrite);
		SetViewPortRenderTexture(RenderTarget);
	}
}
