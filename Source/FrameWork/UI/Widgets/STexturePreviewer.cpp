#include "CommonHeader.h"
#include "STexturePreviewer.h"
#include "AssetObject/TextureCube.h"
#include "Common/Path/PathHelper.h"
#include "Editor/PreviewViewPort.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "GpuApi/GpuRhi.h"
#include "RenderResource/Camera.h"
#include "RenderResource/Mesh.h"
#include "RenderResource/RenderPass/BlitPass.h"
#include "RenderResource/UniformBuffer.h"
#include "Renderer/RenderGraph.h"
#include "Common/Util/Reflection.h"

namespace FW
{
	class CubemapPreviewScene
	{
	public:
		explicit CubemapPreviewScene(PreviewViewPort* InPreview, TextureCube* InTextureCube)
			: Preview(InPreview)
			, TextureCubeAsset(InTextureCube)
		{
			ViewCamera.Position = { 0.0f, 0.0f, -1.6f };
			ViewCamera.VerticalFov = 2.0f * FMath::Atan(1.0f / 1.4f);
			ViewCamera.NearPlane = 0.1f;
			ViewCamera.FarPlane = 100.0f;

			CubeBuffers = UploadMesh(CreateCube());

			UniformBufferBuilder PreviewUbBuilder{ UniformBufferUsage::Persistant };
			PreviewUbBuilder.AddMatrix4x4f(TEXT("Transform"))
							.AddUint(TEXT("ChannelFilter"));
			PreviewUniformBuffer = PreviewUbBuilder.Build();

			BindGroupLayout = GpuBindGroupLayoutBuilder{0}
				.AddUniformBuffer(TEXT("PreviewUb"), PreviewUbBuilder)
				.AddTextureCube(TEXT("PreviewCube"), BindingShaderStage::Pixel)
				.AddSampler(TEXT("PreviewCubeSampler"), BindingShaderStage::Pixel)
				.Build();

			PreviewBindGroup = GpuBindGroupBuilder{BindGroupLayout}
				.SetUniformBuffer(TEXT("PreviewUb"), PreviewUniformBuffer->GetGpuResource())
				.SetTexture(TEXT("PreviewCube"), TextureCubeAsset->GetGpuData()->GetDefaultView())
				.SetSampler(TEXT("PreviewCubeSampler"), GpuResourceHelper::GetSampler({ .Filter = SamplerFilter::Bilinear }))
				.Build();

			const FString ShaderPath = PathHelper::ShaderDir() / TEXT("TextureCubePreview.hlsl");

			TRefCountPtr<GpuShader> Vs = GGpuRhi->CreateShaderFromFile({
				.FileName = ShaderPath,
				.Type = ShaderType::VertexShader,
				.EntryPoint = TEXT("MainVS"),
				.ExtraDecl = BindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL),
			});

			TRefCountPtr<GpuShader> Ps = GGpuRhi->CreateShaderFromFile({
				.FileName = ShaderPath,
				.Type = ShaderType::PixelShader,
				.EntryPoint = TEXT("MainPS"),
				.ExtraDecl = BindGroupLayout->GetCodegenDeclaration(GpuShaderLanguage::HLSL),
			});

			FString ErrorInfo;
			FString WarnInfo;
			GGpuRhi->CompileShader(Vs, ErrorInfo, WarnInfo);
			check(ErrorInfo.IsEmpty());
			GGpuRhi->CompileShader(Ps, ErrorInfo, WarnInfo);
			check(ErrorInfo.IsEmpty());

			GpuRenderPipelineStateDesc PipelineDesc{
				.Vs = Vs,
				.Ps = Ps,
				.Targets = {
					{ .TargetFormat = GpuFormat::B8G8R8A8_UNORM }
				},
				.BindGroupLayout0 = BindGroupLayout,
				.VertexLayout = {
					{
						.ByteStride = sizeof(MeshVertex),
						.Attributes = {
							{
								.Location = 0,
								.SemanticName = TEXT("POSITION"),
								.Format = GpuFormat::R32G32B32_FLOAT,
								.ByteOffset = offsetof(MeshVertex, Position),
							},
							{
								.Location = 1,
								.SemanticName = TEXT("NORMAL"),
								.Format = GpuFormat::R32G32B32_FLOAT,
								.ByteOffset = offsetof(MeshVertex, Normal),
							},
						}
					}
				},
				.RasterizerState = {
					.FillMode = RasterizerFillMode::Solid,
					.CullMode = RasterizerCullMode::Back,
				},
			};

			Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(PipelineDesc);

			Preview->ResizeHandler.AddLambda([this](const Vector2f&) {
				Render();
			});

			Preview->MouseDownHandler.BindLambda([this](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
				if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					bDragging = true;
					LastMousePos = (Vector2f)MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
				}
				return FReply::Unhandled();
			});

			Preview->MouseUpHandler.BindLambda([this](const FGeometry&, const FPointerEvent& MouseEvent) {
				if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					bDragging = false;
					return FReply::Handled();
				}
				return FReply::Unhandled();
			});

			Preview->MouseMoveHandler.BindLambda([this](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
				if (!bDragging || !MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
				{
					return FReply::Unhandled();
				}

				const Vector2f CurrentMousePos = (Vector2f)MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
				const Vector2f MouseDelta = CurrentMousePos - LastMousePos;
				LastMousePos = CurrentMousePos;
				ModelYaw -= MouseDelta.x * 0.01f;
				ModelPitch = FMath::Clamp(ModelPitch + MouseDelta.y * 0.01f, -1.4f, 1.4f);
				Render();
				return FReply::Handled();
			});
		}

		void SetChannelFilter(TextureChannelFilter InChannelFilter)
		{
			ChannelFilter = InChannelFilter;
		}

		void Render()
		{
			ResizeRenderTargetIfNeeded();

			const FMatrix44f ModelMatrix = RotationMatrix(ModelYaw, ModelPitch);
			const FMatrix44f Transform = ModelMatrix * ViewCamera.GetViewProjectionMatrix();
			PreviewUniformBuffer->GetMember<FMatrix44f>(TEXT("Transform")) = Transform;
			PreviewUniformBuffer->GetMember<uint32>(TEXT("ChannelFilter")) = static_cast<uint32>(ChannelFilter);

			GpuRenderPassDesc PassDesc;
			PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ RenderTarget->GetDefaultView(), RenderTargetLoadAction::Clear, RenderTargetStoreAction::Store });

			auto CmdRecorder = GGpuRhi->BeginRecording(TEXT("TextureCubePreview"));
			{
				auto PassRecorder = CmdRecorder->BeginRenderPass(PassDesc, TEXT("TextureCubePreview"));
				{
					PassRecorder->SetRenderPipelineState(Pipeline);
					PassRecorder->SetBindGroups(PreviewBindGroup, nullptr, nullptr, nullptr);
					PassRecorder->SetVertexBuffer(0, CubeBuffers.VertexBuffer);
					PassRecorder->SetIndexBuffer(CubeBuffers.IndexBuffer);
					PassRecorder->DrawIndexed(0, CubeBuffers.IndexCount);
				}
				CmdRecorder->EndRenderPass(PassRecorder);
			}
			GGpuRhi->EndRecording(CmdRecorder);
			GGpuRhi->Submit({ CmdRecorder });

			Preview->SetViewPortRenderTexture(RenderTarget);
		}

		void ResizeRenderTargetIfNeeded()
		{
			const FIntPoint ViewportSize = Preview->GetSize();
			if (RenderTarget.IsValid()
				&& RenderTarget->GetWidth() == static_cast<uint32>(ViewportSize.X)
				&& RenderTarget->GetHeight() == static_cast<uint32>(ViewportSize.Y))
			{
				return;
			}

			GpuTextureDesc Desc{
				.Width = static_cast<uint32>(ViewportSize.X),
				.Height = static_cast<uint32>(ViewportSize.Y),
				.Format = GpuTextureFormat::B8G8R8A8_UNORM,
				.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
				.ClearValues = Vector4f(0.08f, 0.08f, 0.08f, 1.0f),
			};
			RenderTarget = GGpuRhi->CreateTexture(Desc, GpuResourceState::RenderTargetWrite);
			ViewCamera.AspectRatio = (float)RenderTarget->GetWidth() / (float)RenderTarget->GetHeight();
		}

	private:
		PreviewViewPort* Preview = nullptr;
		TextureCube* TextureCubeAsset = nullptr;
		TextureChannelFilter ChannelFilter = TextureChannelFilter::None;
		Camera ViewCamera;
		MeshBuffers CubeBuffers;
		TUniquePtr<UniformBuffer> PreviewUniformBuffer;
		TRefCountPtr<GpuBindGroupLayout> BindGroupLayout;
		TRefCountPtr<GpuBindGroup> PreviewBindGroup;
		TRefCountPtr<GpuRenderPipelineState> Pipeline;
		TRefCountPtr<GpuTexture> RenderTarget;
		bool bDragging = false;
		Vector2f LastMousePos = { 0.0f, 0.0f };
		float ModelYaw = 0.0f;
		float ModelPitch = 0.0f;
	};


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
		TextureAsset = InArgs._InTexture;
		Preview = MakeShared<PreviewViewPort>();
		const bool bIsTextureCube = DynamicCast<TextureCube>(TextureAsset.Get()) != nullptr;

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
					.IsEnabled(bIsTextureCube)
					.IsChecked_Lambda([this] {
						return bView3D ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
					})
					.OnCheckStateChanged_Lambda([this](ECheckBoxState InState) {
						bView3D = (InState == ECheckBoxState::Checked);
						RefreshPreview();
					})
					[
						SNew(STextBlock).Text(LOCALIZATION("TexturePreview3DView"))
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

		RefreshPreview();
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
				if (!CubemapPreviewSceneInstance)
				{
					CubemapPreviewSceneInstance = MakeUnique<CubemapPreviewScene>(Preview.Get(), TexCube);
				}
				PreviewTexture = nullptr;
				CubemapPreviewSceneInstance->SetChannelFilter(ChannelFilter);
				CubemapPreviewSceneInstance->Render();
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
