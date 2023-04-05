#include "CommonHeader.h"
#include "PreviewViewPort.h"
#include "GpuApi/GpuApiInterface.h"
#include <Textures/SlateUpdatableTexture.h>

namespace SH
{
	void PreviewViewPort::SetViewPortRenderTexture(GpuTexture* InGpuTex)
	{
		FSlateRenderer* UIRenderer = FSlateApplication::Get().GetRenderer();
		void* SharedHandle = GpuApi::GetSharedHandle(InGpuTex);
		FSlateShaderResource* UpdatableTexture = UIRenderer->CreateSharedHandleTexture(SharedHandle)->GetSlateResource();
		ViewPortRT = MakeShareable(UpdatableTexture);
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
