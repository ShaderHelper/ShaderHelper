#include "CommonHeader.h"
#include "SDebuggerViewport.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "Editor/ShaderHelperEditor.h"
#include "App/App.h"
#include "RenderResource/Shader/DebuggerGridShader.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"

#include <Widgets/SViewport.h>

using namespace FW;

namespace SH
{
	void SDebuggerViewport::Construct( const FArguments& InArgs )
	{
		ViewPort = MakeShared<PreviewViewPort>();
		
		auto InfoTip = SNew(SBorder)
		.Padding(4)
		.BorderImage(FAppStyle::Get().GetBrush("ToolTip.Background"))
		.BorderBackgroundColor(FLinearColor{1,1,1,0.2f})
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
					float Value{};
					if(DebuggerTex) {
						uint32 DataIndex = PixelCoord.x + PixelCoord.y * DebuggerTex->GetWidth();
						Value = TexDatas[DataIndex].x;
					}
					return FText::FromString(FString::Printf(TEXT("R Channel: %f"), Value));
				})
			]
			+SVerticalBox::Slot()
			.Padding(0, 0, 0, 2)
			[
				SNew(STextBlock).Text_Lambda([this]{
					float Value{};
					if(DebuggerTex) {
						uint32 DataIndex = PixelCoord.x + PixelCoord.y * DebuggerTex->GetWidth();
						Value = TexDatas[DataIndex].y;
					}
					return FText::FromString(FString::Printf(TEXT("G Channel: %f"), Value));
				})
			]
			+SVerticalBox::Slot()
			.Padding(0, 0, 0, 2)
			[
				SNew(STextBlock).Text_Lambda([this]{
					float Value{};
					if(DebuggerTex) {
						uint32 DataIndex = PixelCoord.x + PixelCoord.y * DebuggerTex->GetWidth();
						Value = TexDatas[DataIndex].z;
					}
					return FText::FromString(FString::Printf(TEXT("B Channel: %f"), Value));
				})
			]
			+SVerticalBox::Slot()
			.Padding(0, 0, 0, 2)
			[
				SNew(STextBlock).Text_Lambda([this]{
					float Value{};
					if(DebuggerTex) {
						uint32 DataIndex = PixelCoord.x + PixelCoord.y * DebuggerTex->GetWidth();
						Value = TexDatas[DataIndex].w;
					}
					return FText::FromString(FString::Printf(TEXT("A Channel: %f"), Value));
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
					FVector2D ViewportSize = FVector2D(ViewPort->GetSize().X, ViewPort->GetSize().Y) / DpiScale;
					FVector2D InfoTipSize = InfoTip->GetCachedGeometry().GetLocalSize();
					FVector2D MousePos = FVector2D{ MouseLoc.x, MouseLoc.y } / DpiScale;

					FVector2D Offset = FVector2D(8, 8);
					FVector2D Pos = MousePos + Offset;

					if (Pos.X + InfoTipSize.X > ViewportSize.X)
						Pos.X = MousePos.X - InfoTipSize.X - Offset.X;
					if (Pos.Y + InfoTipSize.Y > ViewportSize.Y)
						Pos.Y = MousePos.Y - InfoTipSize.Y - Offset.Y;

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
		
		Vector2f CurMousePos = (Vector2f)(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) * DpiScale);
		if(MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
		{
			Offset += (MouseLoc - CurMousePos);
			float MaxWOffset = (float)ViewPort->GetSize().X * (int(Zoom) - 1);
			float MaxHOffset = (float)ViewPort->GetSize().Y * (int(Zoom) - 1);
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
		
		Vector2f CurMousePos = (Vector2f)(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) * DpiScale);
		Vector2f ContentPosBefore = (CurMousePos + Offset) / float(Zoom);
		Zoom = FMath::Clamp(int32(float(Zoom) + MouseEvent.GetWheelDelta()), 1, 128);
		Offset = ContentPosBefore * float(Zoom) - CurMousePos;
		float MaxWOffset = (float)ViewPort->GetSize().X * (int(Zoom) - 1);
		float MaxHOffset = (float)ViewPort->GetSize().Y * (int(Zoom) - 1);
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

	void SDebuggerViewport::SetDebugTarget(TRefCountPtr<GpuTexture> InTarget, bool GlobalValidation)
	{
		bFinalizePixel = false;
		Zoom = 1;
		Offset = 0.0f;
		DpiScale = FSlateApplication::Get().FindWidgetWindow(AsShared())->GetNativeWindow()->GetDPIScaleFactor();
		
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
				const uint8* Pixel = SrcRow + x * GetTextureFormatByteSize(InTarget->GetFormat());
				if(InTarget->GetFormat() == GpuTextureFormat::B8G8R8A8_UNORM)
				{
					TexDatas[y * Width + x] = {Pixel[2] / 255.0f, Pixel[1] / 255.0f, Pixel[0] / 255.0f, Pixel[3] / 255.0f};
				}
				else if (InTarget->GetFormat() == GpuTextureFormat::R32G32B32A32_FLOAT)
				{
					TexDatas[y * Width + x] = { *((float*)Pixel), *((float*)Pixel + 1), *((float*)Pixel + 2), *((float*)Pixel + 3) };
				}
			}
		}
		GGpuRhi->UnMapGpuTexture(InTarget);

		if (GlobalValidation)
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			auto Invocation = ShEditor->GetDebuggaleObject()->GetInvocationState();
			if (std::holds_alternative<PixelState>(Invocation))
			{
				SShaderEditorBox* ShaderEditor = ShEditor->GetShaderEditor(ShEditor->GetDebuggaleObject()->GetShaderAsset());
				std::optional<Vector2u> ErrorCoord;
				try
				{
					ErrorCoord = ShaderEditor->ValidatePixel(Invocation);
				}
				catch (const std::runtime_error& e)
				{
					MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, GApp->GetEditor()->GetMainWindow(), FText::FromString(UTF8_TO_TCHAR(e.what())));
					ShEditor->EndDebugging();
					return;
				}

				if (ErrorCoord)
				{
					PixelCoord = ErrorCoord.value();
					MouseLoc = PixelCoord * Zoom - Offset;
					Draw();
					bFinalizePixel = true;
					ShEditor->GetDebuggaleObject()->OnFinalizePixel(PixelCoord);
				}
				else
				{
					MessageDialog::Open(MessageDialog::Ok, MessageDialog::Happy, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("ValidationTip"));
					ShEditor->EndDebugging();
				}
			}
		}
	}
}
