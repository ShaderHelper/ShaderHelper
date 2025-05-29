#include "CommonHeader.h"
#include "SDebuggerViewport.h"
#include <Widgets/SViewport.h>
#include "UI/Styles/FShaderHelperStyle.h"
#include "Editor/ShaderHelperEditor.h"
#include "App/App.h"
#include "RenderResource/Shader/DebuggerGridShader.h"

using namespace FW;

namespace SH
{
	void SDebuggerViewport::Construct( const FArguments& InArgs )
	{
		ViewPort = MakeShared<PreviewViewPort>();
		
		auto InfoTip = SNew(SBorder)
		.Padding(4)
		.BorderImage(FAppStyle::Get().GetBrush("ToolTip.Background"))
		.BorderBackgroundColor(FLinearColor{1,1,1,0.2})
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.Padding(0, 0, 0, 8)
			[
				SNew(STextBlock).Text_Lambda([this]{
					return FText::FromString(FString::Printf(TEXT("Pixel Coord: %d %d"), PixelCoord.x, PixelCoord.y));
				})
			]
			+SVerticalBox::Slot()
			.Padding(0, 0, 0, 2)
			[
				SNew(STextBlock).Text_Lambda([this]{
					uint32 Value{};
					if(DebuggerTex) {
						uint32 DataIndex = PixelCoord.x + PixelCoord.y * DebuggerTex->GetWidth();
						Value = TexDatas[DataIndex].z;
					}
					return FText::FromString(FString::Printf(TEXT("R Channel: %d"), Value));
				})
			]
			+SVerticalBox::Slot()
			.Padding(0, 0, 0, 2)
			[
				SNew(STextBlock).Text_Lambda([this]{
					uint32 Value{};
					if(DebuggerTex) {
						uint32 DataIndex = PixelCoord.x + PixelCoord.y * DebuggerTex->GetWidth();
						Value = TexDatas[DataIndex].y;
					}
					return FText::FromString(FString::Printf(TEXT("G Channel: %d"), Value));
				})
			]
			+SVerticalBox::Slot()
			.Padding(0, 0, 0, 2)
			[
				SNew(STextBlock).Text_Lambda([this]{
					uint32 Value{};
					if(DebuggerTex) {
						uint32 DataIndex = PixelCoord.x + PixelCoord.y * DebuggerTex->GetWidth();
						Value = TexDatas[DataIndex].x;
					}
					return FText::FromString(FString::Printf(TEXT("B Channel: %d"), Value));
				})
			]
		];

		
		ChildSlot
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SViewport)
				.ViewportInterface(ViewPort)
			]
			+SOverlay::Slot()
			[
				SNew(SCanvas)
				+SCanvas::Slot()
				.Size_Lambda([InfoTip]{
					return InfoTip->GetDesiredSize();
				})
				.Position_Lambda([this, InfoTip]{
					FVector2D ViewportSize = FVector2D(ViewPort->GetSize().X, ViewPort->GetSize().Y);
					FVector2D InfoTipSize = InfoTip->GetDesiredSize();
					FVector2D MousePos(MouseLoc.x, MouseLoc.y);

					FVector2D Pos = MousePos + FVector2D(8, 8);

					if (Pos.X + InfoTipSize.X > ViewportSize.X)
						Pos.X = MousePos.X - InfoTipSize.X - 8;
					if (Pos.Y + InfoTipSize.Y > ViewportSize.Y)
						Pos.Y = MousePos.Y - InfoTipSize.Y - 8;

					return Pos;
				})
				[
					InfoTip
				]
			]
		];
		
	 }

	FReply SDebuggerViewport::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if(bFinalizePixel)
		{
			return FReply::Unhandled();
		}
		
		Vector2f CurMousePos = (Vector2f)(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) * MyGeometry.Scale);
		if(MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
		{
			Offset += (MouseLoc - CurMousePos);
			float MaxWOffset = ViewPort->GetSize().X * (int(Zoom) - 1);
			float MaxHOffset = ViewPort->GetSize().Y * (int(Zoom) - 1);
			Offset.x = FMath::Clamp(Offset.x, 0.0f, MaxWOffset);
			Offset.y = FMath::Clamp(Offset.y, 0.0f, MaxHOffset);
		}
		MouseLoc = CurMousePos;
		PixelCoord.x = uint32(MouseLoc.x + Offset.x) / Zoom;
		PixelCoord.y = uint32(MouseLoc.y + Offset.y) / Zoom;
		return FReply::Handled();
	}

	FReply SDebuggerViewport::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if(bFinalizePixel)
		{
			return FReply::Unhandled();
		}
		
		Vector2f CurMousePos = (Vector2f)(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) * MyGeometry.Scale);
		Vector2f ContentPosBefore = (CurMousePos + Offset) / float(Zoom);
		Zoom = FMath::Clamp(int32(float(Zoom) + MouseEvent.GetWheelDelta()), 1, 128);
		Offset = ContentPosBefore * float(Zoom) - CurMousePos;
		float MaxWOffset = ViewPort->GetSize().X * (int(Zoom) - 1);
		float MaxHOffset = ViewPort->GetSize().Y * (int(Zoom) - 1);
		Offset.x = FMath::Clamp(Offset.x, 0.0f, MaxWOffset);
		Offset.y = FMath::Clamp(Offset.y, 0.0f, MaxHOffset);
		return FReply::Handled();
	}

	FReply SDebuggerViewport::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if(bFinalizePixel)
		{
			return FReply::Unhandled();
		}
		
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if(MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			ShEditor->EndDebugging();
			return FReply::Handled().ReleaseMouseLock();
		}
		else if(MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			bFinalizePixel = true;
			ShEditor->GetDebuggaleObject()->OnFinalizePixel(PixelCoord);
			return FReply::Handled().ReleaseMouseLock();
		}
		return FReply::Handled();
	}

	void SDebuggerViewport::Draw() const
	{
		if(!DebuggerTex || bFinalizePixel)
		{
			return;
		}
		
		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ DebuggerTex, RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store });
		
		DebuggerGridShader* PassShader = GetShader<DebuggerGridShader>();
		
		BindingContext Bindings;
		DebuggerGridShader::Parameters ShaderParameter{
			.InputTex = RawTex,
			.Offset = Offset,
			.Zoom = Zoom,
			.MouseLoc = MouseLoc,
		};
		Bindings.SetShaderBindGroup(PassShader->GetBindGroup(ShaderParameter));
		Bindings.SetShaderBindGroupLayout(PassShader->GetBindGroupLayout());
		
		GpuRenderPipelineStateDesc PipelineDesc{
			.Vs = PassShader->GetVertexShader(),
			.Ps = PassShader->GetPixelShader(),
			.Targets = {
				{ .TargetFormat = DebuggerTex->GetFormat() }
			}
		};
		Bindings.ApplyBindGroupLayout(PipelineDesc);
		
		TRefCountPtr<GpuRenderPipelineState> Pipeline = GGpuRhi->CreateRenderPipelineState(PipelineDesc);
		
		auto CmdRecorder = GGpuRhi->BeginRecording();
		{
			auto PassRecorder = CmdRecorder->BeginRenderPass(MoveTemp(PassDesc), "DebuggerGrid");
			{
				Bindings.ApplyBindGroup(PassRecorder);
				PassRecorder->SetRenderPipelineState(Pipeline);
				PassRecorder->DrawPrimitive(0, 3, 0, 1);
			}
			CmdRecorder->EndRenderPass(PassRecorder);
		}
		GGpuRhi->EndRecording(CmdRecorder);
		GGpuRhi->Submit({ CmdRecorder });
	}

	int32 SDebuggerViewport::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
	{
		Draw();
		return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	void SDebuggerViewport::SetDebugTarget(TRefCountPtr<GpuTexture> InTarget)
	{
		check(InTarget->GetFormat() == GpuTextureFormat::B8G8R8A8_UNORM);

		bFinalizePixel = false;
		Zoom = 1;
		Offset = 0.0f;
		
		uint32 Width = InTarget->GetWidth();
		uint32 Height = InTarget->GetHeight();
		
		DebuggerTex = GGpuRhi->CreateTexture({
			.Width = Width,
			.Height = Height,
			.Format = InTarget->GetFormat(),
			.Usage =  GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared
		});
		RawTex = InTarget;
		ViewPort->SetViewPortRenderTexture(DebuggerTex);
		
		uint32 PaddedRowPitch;
		uint8* PaddedData = (uint8*)GGpuRhi->MapGpuTexture(InTarget, GpuResourceMapMode::Read_Only, PaddedRowPitch);
		TexDatas.SetNum(Width * Height);
		for (uint32 y = 0; y < Height; ++y)
		{
			const uint8* SrcRow = PaddedData + y * PaddedRowPitch;
			for (uint32 x = 0; x < Width; ++x)
			{
				const uint8* Pixel = SrcRow + x * 4;
				TexDatas[y * Width + x] = Vector3u(Pixel[0], Pixel[1], Pixel[2]);
			}
		}
		GGpuRhi->UnMapGpuTexture(InTarget);
	}
}
