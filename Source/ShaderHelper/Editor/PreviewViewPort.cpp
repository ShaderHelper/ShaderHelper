#include "CommonHeader.h"
#include "PreviewViewPort.h"
#include "GpuApi/GpuApiInterface.h"

namespace SH
{
	void PreviewViewPort::SetViewPortRenderTexture(GpuTexture* InGpuTex)
	{
		FSlateRenderer* UIRenderer = FSlateApplication::Get().GetRenderer();
		void* SharedHandle = GpuApi::GetSharedHandle(InGpuTex);
		//May need sync here.
        FSlateUpdatableTexture* UpdatableTexture = UIRenderer->CreateSharedHandleTexture2(SharedHandle);
		ViewPortRT = MakeShareable(UpdatableTexture);
	}

	//void PreviewViewPort::UpdateViewPortRenderTexture(GpuTexture* InGpuTex)
	//{
	//	check((SizeX == InGpuTex->GetWidth()) && (SizeY == InGpuTex->GetHeight()));
	//	uint32 PaddedRowPitch;
	//	uint8* PaddedData = (uint8*)GpuApi::MapGpuTexture(InGpuTex, GpuResourceMapMode::Read_Only, PaddedRowPitch);
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
			if (OnViewportResize.IsBound())
			{
				OnViewportResize.Broadcast(Vector2f{ (float)SizeX, (float)SizeY });
			}
		}

	}

}
