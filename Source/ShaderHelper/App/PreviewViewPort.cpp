#include "CommonHeader.h"
#include "PreviewViewPort.h"
#include "GpuApi/GpuApiInterface.h"

namespace SH
{
	void PreviewViewPort::SetViewPortRenderTexture(GpuTexture* InGpuTex)
	{
		FSlateRenderer* UIRenderer = FSlateApplication::Get().GetRenderer();
		void* SharedHandle = GpuApi::GetSharedHandle(InGpuTex);
		FSlateUpdatableTexture* UpdatableTexture = UIRenderer->CreateSharedHandleTexture(SharedHandle);
		ViewPortRT = MakeShareable(UpdatableTexture);
	}

	void PreviewViewPort::UpdateViewPortRenderTexture(GpuTexture* InGpuTex)
	{
		check((SizeX == InGpuTex->GetWidth()) && (SizeY == InGpuTex->GetHeight()));
		uint32 PaddedRowPitch;
		uint8* PaddedData = (uint8*)GpuApi::MapGpuTexture(InGpuTex, GpuResourceMapMode::Read_Only, PaddedRowPitch);
		uint32 UnpaddedSize = InGpuTex->GetWidth() * InGpuTex->GetHeight() * GetTextureFormatByteSize(InGpuTex->GetFormat());
        
        TArray<uint8> UnpaddedData;
        UnpaddedData.Reserve(UnpaddedSize);
		uint32 UnpaddedRowPitch = InGpuTex->GetWidth() * GetTextureFormatByteSize(InGpuTex->GetFormat());
		for (uint32 Row = 0; Row < InGpuTex->GetHeight(); ++Row)
		{
            uint8* DataPtr = UnpaddedData.GetData();
			FMemory::Memcpy(DataPtr + Row * UnpaddedRowPitch, PaddedData + Row * PaddedRowPitch, UnpaddedRowPitch);
		}
		
		//FSlateD3DTexture::UpdateTexture() updates texture with the unpadded data.
		ViewPortRT->UpdateTexture(UnpaddedData);
		GpuApi::UnMapGpuTexture(InGpuTex);
	}

	void PreviewViewPort::OnDrawViewport(const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled)
	{
		FIntPoint DrawSize = FIntPoint(FMath::RoundToInt(AllottedGeometry.GetDrawSize().X), FMath::RoundToInt(AllottedGeometry.GetDrawSize().Y));
		if (GetSize() != DrawSize && DrawSize.X > 0 && DrawSize.Y > 0)
		{
			SizeX = DrawSize.X;
			SizeY = DrawSize.Y;
			if (OnViewportResize.IsBound())
			{
				OnViewportResize.Broadcast();
			}
		}

	}

}
