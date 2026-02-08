#include "CommonHeader.h"
#include "PreviewViewPort.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	uint32 MapTextureFormatToGLInternalFormat(GpuTextureFormat InTexFormat)
	{
		switch (InTexFormat)
		{
		case GpuTextureFormat::R8_UNORM:              return 0x8229; // GL_R8
		case GpuTextureFormat::R8G8B8A8_UNORM:        return 0x8058; // GL_RGBA8
		case GpuTextureFormat::B8G8R8A8_UNORM:        return 0x8058; // GL_RGBA8
		case GpuTextureFormat::B8G8R8A8_UNORM_SRGB:   return 0x8C43; // GL_SRGB8_ALPHA8
		case GpuTextureFormat::R10G10B10A2_UNORM:     return 0x8059; // GL_RGB10_A2
		case GpuTextureFormat::R16G16B16A16_UNORM:    return 0x805B; // GL_RGBA16
		case GpuTextureFormat::R16G16B16A16_UINT:     return 0x8D76; // GL_RGBA16UI
		case GpuTextureFormat::R32G32B32A32_UINT:     return 0x8D70; // GL_RGBA32UI
		case GpuTextureFormat::R16G16B16A16_FLOAT:    return 0x881A; // GL_RGBA16F
		case GpuTextureFormat::R32G32B32A32_FLOAT:    return 0x8814; // GL_RGBA32F
		case GpuTextureFormat::R11G11B10_FLOAT:       return 0x8C3A; // GL_R11F_G11F_B10F
		case GpuTextureFormat::R16_FLOAT:             return 0x822D; // GL_R16F
		case GpuTextureFormat::R32_FLOAT:             return 0x822E; // GL_R32F
		default:
			AUX::Unreachable();
		}
	}

	void PreviewViewPort::SetViewPortRenderTexture(GpuTexture* InGpuTex)
	{
		FSlateRenderer* UIRenderer = FSlateApplication::Get().GetRenderer();
		void* SharedHandle = GGpuRhi->GetSharedHandle(InGpuTex);
		FSlateUpdatableTexture* UpdatableTexture;
		GpuRhiBackendType BackendType = GetGpuRhiBackendType();
#if PLATFORM_WINDOWS
		if (BackendType == GpuRhiBackendType::Vulkan)
		{
			UpdatableTexture = UIRenderer->CreateSharedHandleTextureFromVulkan(SharedHandle, InGpuTex->GetWidth(), InGpuTex->GetHeight(),
				MapTextureFormatToGLInternalFormat(InGpuTex->GetFormat()), InGpuTex->GetAllocationSize());
		}
		else
		{
			UpdatableTexture = UIRenderer->CreateSharedHandleTextureFromDX12(SharedHandle);
		}
#elif PLATFORM_MAC
		UpdatableTexture = UIRenderer->CreateSharedHandleTextureFromMetal(SharedHandle);
#endif
		ViewPortRT = MakeShareable(UpdatableTexture);
	}

	//void PreviewViewPort::UpdateViewPortRenderTexture(GpuTexture* InGpuTex)
	//{
	//	check((SizeX == InGpuTex->GetWidth()) && (SizeY == InGpuTex->GetHeight()));
	//	uint32 PaddedRowPitch;
	//	uint8* PaddedData = (uint8*)GGpuRhi->MapGpuTexture(InGpuTex, GpuResourceMapMode::Read_Only, PaddedRowPitch);
	//	uint32 UnpaddedSize = InGpuTex->GetWidth() * InGpuTex->GetHeight() * GetTextureFormatByteSize(InGpuTex->GetFormat());
 //       
 //       tarray<uint8> unpaddeddata;
 //       unpaddeddata.setnumuninitialized(unpaddedsize);
	//	uint32 unpaddedrowpitch = ingputex->getwidth() * gettextureformatbytesize(ingputex->getformat());
	//	for (uint32 row = 0; row < ingputex->getheight(); ++row)
	//	{
 //           uint8* DataPtr = UnpaddedData.GetData();
	//		FMemory::Memcpy(DataPtr + Row * UnpaddedRowPitch, PaddedData + Row * PaddedRowPitch, UnpaddedRowPitch);
	//	}
	//	
	//	//fslated3dtexture::updatetexture() updates texture with the unpadded data.
	//	viewportrt->updatetexture(unpaddeddata);
	//	gpuapi::unmapgputexture(ingputex);
	//}

	void PreviewViewPort::OnDrawViewport(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled)
	{
		FIntPoint DrawSize = FIntPoint(FMath::RoundToInt(AllottedGeometry.GetDrawSize().X), FMath::RoundToInt(AllottedGeometry.GetDrawSize().Y));
		if (GetSize() != DrawSize && DrawSize.X > 0 && DrawSize.Y > 0)
		{
			SizeX = DrawSize.X;
			SizeY = DrawSize.Y;
			if (ResizeHandler.IsBound())
			{
				ResizeHandler.Broadcast(Vector2f{ (float)SizeX, (float)SizeY });
			}
		}

	}

}
