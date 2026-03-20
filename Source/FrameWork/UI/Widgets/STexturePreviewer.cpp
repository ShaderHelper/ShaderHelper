#include "CommonHeader.h"
#include "STexturePreviewer.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "GpuApi/GpuRhi.h"
#include "RenderResource/RenderPass/BlitPass.h"
#include "Renderer/RenderGraph.h"
#include "Common/Util/Reflection.h"

namespace FW
{

	class STexturePreviewCanvas : public SPanel
	{
	public:
		SLATE_BEGIN_ARGS(STexturePreviewCanvas) {}
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()

		STexturePreviewCanvas() : ChildSlot(this) {}

		void Construct(const FArguments& InArgs)
		{
			ChildSlot[InArgs._Content.Widget];
		}

		virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override
		{
			if (ChildSlot.GetWidget() != SNullWidget::NullWidget)
			{
				FVector2D PanelSize = AllottedGeometry.GetLocalSize();
				FVector2D ChildSize = ChildSlot.GetWidget()->GetDesiredSize();
				FVector2D CenterOffset = (PanelSize - ChildSize * (1.0f + ZoomLevel)) * 0.5f;
				FVector2D Offset = CenterOffset - ViewOffset * (1.0f + ZoomLevel);

				ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(
					ChildSlot.GetWidget(), ChildSize,
					FSlateLayoutTransform{1.0f + ZoomLevel, Offset}
				));
			}
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(256.0f, 256.0f);
		}

		virtual FChildren* GetChildren() override
		{
			return &ChildSlot;
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
			{
				MousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
				return FReply::Handled().CaptureMouse(AsShared());
			}
			return FReply::Unhandled();
		}

		virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
			{
				return FReply::Handled().ReleaseMouseCapture();
			}
			return FReply::Unhandled();
		}

		virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
			{
				FVector2D CurMousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
				FVector2D DeltaPos = CurMousePos - MousePos;
				MousePos = CurMousePos;

				ViewOffset -= DeltaPos / (1.0f + ZoomLevel);
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

		virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			ZoomLevel += MouseEvent.GetWheelDelta() * 0.1f;
			ZoomLevel = FMath::Clamp(ZoomLevel, -0.5f, 5.0f);
			return FReply::Handled();
		}

	private:
		FSimpleSlot ChildSlot;
		float ZoomLevel = 0.0f;
		FVector2D ViewOffset = FVector2D::ZeroVector;
		FVector2D MousePos = FVector2D::ZeroVector;
	};

	void STexturePreviewer::Construct(const FArguments& InArgs)
	{
		MouseButtonDownHandler = InArgs._OnMouseButtonDown;
		Preview = MakeShared<PreviewViewPort>();

		TSharedPtr<SHorizontalBox> ChannelToolbar;

		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4)
			[
				SAssignNew(Toolbar, SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew(ChannelToolbar, SHorizontalBox)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2, 0)
				[
					SNew(SCheckBox)
					.IsEnabled_Lambda([this] {
						return DynamicCast<TextureCube>(TextureAsset.Get()) != nullptr;
					})
					.IsChecked_Lambda([this] {
						return bView3D ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {
						bView3D = (InState == ECheckBoxState::Checked);
						RefreshPreview();
					})
					[
						SNew(STextBlock).Text(FText::FromString("3D View"))
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
				[
					SNew(STexturePreviewCanvas)
					.Clipping(EWidgetClipping::ClipToBounds)
					.Content()
					[
						SNew(SViewport)
						.ViewportInterface(Preview)
					]
				]
			]
		];

		auto AddChannelButton = [&](const FString& Label, TextureChannelFilter Filter)
		{
			ChannelToolbar->AddSlot()
			.AutoWidth()
			.Padding(2, 0)
			[
				SNew(SShToggleButton)
				.Text(FText::FromString(Label))
				.IsChecked_Lambda([this, Filter] {
					return ChannelFilter == Filter ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this, Filter](ECheckBoxState InState) {
					ChannelFilter = (InState == ECheckBoxState::Checked) ? Filter : TextureChannelFilter::None;
					RefreshPreview();
				})
			];
		};

		AddChannelButton("R", TextureChannelFilter::R);
		AddChannelButton("G", TextureChannelFilter::G);
		AddChannelButton("B", TextureChannelFilter::B);
		AddChannelButton("A", TextureChannelFilter::A);

		if (InArgs._InTexture)
		{
			SetTexture(InArgs._InTexture);
		}
	}

	void STexturePreviewer::RefreshPreview()
	{
		Texture2D* Tex2d = DynamicCast<Texture2D>(TextureAsset.Get());
		TextureCube* TexCube = DynamicCast<TextureCube>(TextureAsset.Get());

		auto UpdateFilteredPreview = [this](GpuTexture* SourceTexture)
		{
			GpuTextureDesc Desc{
				SourceTexture->GetWidth(),
				SourceTexture->GetHeight(),
				SourceTexture->GetFormat(),
				GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared
			};
			PreviewTexture = GGpuRhi->CreateTexture(MoveTemp(Desc));

			RenderGraph Graph;
			{
				BlitPassInput Input;
				Input.InputView = SourceTexture->GetDefaultView();
				Input.InputTexSampler = GpuResourceHelper::GetSampler({});
				Input.OutputView = PreviewTexture->GetDefaultView();
				Input.VariantDefinitions.insert(FString::Printf(TEXT("CHANNEL_FILTER_%s"),
					ANSI_TO_TCHAR(magic_enum::enum_name(ChannelFilter).data())));
				AddBlitPass(Graph, MoveTemp(Input));
			}
			Graph.Execute();

			Preview->SetViewPortRenderTexture(PreviewTexture);
		};

		if (Tex2d)
		{
			UpdateFilteredPreview(Tex2d->GetGpuData());
		}
		else if (TexCube)
		{
			if (bView3D)
			{
				UpdateFilteredPreview(TexCube->GetPreviewTexture());
			}
			else
			{
				uint32 S = TexCube->GetSize();
				uint32 BytePerPixel = GetTextureFormatByteSize(TexCube->GetFormat());
				uint32 OutputW = 4 * S;
				uint32 OutputH = 3 * S;

				// Cross layout (4x3 grid):
				//       [+Y]
				// [-X]  [+Z]  [+X]  [-Z]
				//       [-Y]
				struct FaceLayout { uint32 Col; uint32 Row; };
				const FaceLayout Layouts[6] = { {2,1}, {0,1}, {1,0}, {1,2}, {1,1}, {3,1} };

				TArray<uint8> CompositeData;
				CompositeData.SetNumZeroed(OutputW * OutputH * BytePerPixel);

				const auto& Faces = TexCube->GetFaceData();
				for (int32 Face = 0; Face < 6; Face++)
				{
					uint32 OffX = Layouts[Face].Col * S;
					uint32 OffY = Layouts[Face].Row * S;

					for (uint32 Row = 0; Row < S; Row++)
					{
						const uint8* Src = Faces[Face].GetData() + Row * S * BytePerPixel;
						uint8* Dst = CompositeData.GetData() + ((OffY + Row) * OutputW + OffX) * BytePerPixel;
						FMemory::Memcpy(Dst, Src, S * BytePerPixel);
					}
				}

				GpuTextureDesc Desc{ OutputW, OutputH, TexCube->GetFormat(), GpuTextureUsage::ShaderResource | GpuTextureUsage::Shared, CompositeData };
				TRefCountPtr<GpuTexture> CompositeTexture = GGpuRhi->CreateTexture(MoveTemp(Desc));
				UpdateFilteredPreview(CompositeTexture);
			}
		}
		else
		{
			PreviewTexture = nullptr;
			Preview->Clear();
		}
	}

	FReply STexturePreviewer::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseButtonDownHandler.IsBound())
		{
			return MouseButtonDownHandler.Execute(MyGeometry, MouseEvent);
		}
		return FReply::Handled();
	}

	void STexturePreviewer::SetTexture(AssetPtr<AssetObject> InTexture)
	{
		TextureAsset = InTexture;
		ChannelFilter = TextureChannelFilter::None;
		RefreshPreview();
	}

	void STexturePreviewer::OpenTexturePreviewer(AssetPtr<AssetObject> InTexture, const FPointerEventHandler& InOnMouseButtonDown, TSharedPtr<SWindow> InParentWindow)
	{
		if (TSharedPtr<STexturePreviewer> ExistingPreviewer = Instance.Pin())
		{
			if (TSharedPtr<SWindow> ExistingWindow = FSlateApplication::Get().FindWidgetWindow(ExistingPreviewer.ToSharedRef()))
			{
				ExistingWindow->RequestDestroyWindow();
			}
			Instance.Reset();
		}

		TSharedRef<SWindow> PreviewWindow = SNew(SWindow)
			.Title(LOCALIZATION("TexturePreview"))
			.ClientSize(FVector2D(512, 512))
			.AutoCenter(EAutoCenter::PreferredWorkArea)
			.SupportsMinimize(true)
			.SupportsMaximize(true);

		TSharedRef<STexturePreviewer> Previewer = SNew(STexturePreviewer)
			.InTexture(InTexture)
			.OnMouseButtonDown(InOnMouseButtonDown);

		PreviewWindow->SetContent(Previewer);

		if (InParentWindow.IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(PreviewWindow, InParentWindow.ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(PreviewWindow);
		}

		Instance = Previewer;
	}
}
