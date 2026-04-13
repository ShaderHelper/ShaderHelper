#include "CommonHeader.h"
#include "SMaterialPreviewer.h"
#include "Renderer/MaterialPreviewRenderer.h"

namespace SH
{
	using namespace FW;

	SMaterialPreviewer::~SMaterialPreviewer()
	{
		if (Preview.IsValid())
		{
			Preview->ResizeHandler.Remove(ResizeHandlerHandle);
			Preview->MouseDownHandler.Unbind();
			Preview->MouseUpHandler.Unbind();
			Preview->MouseMoveHandler.Unbind();
			Preview->MouseWheelHandler.Unbind();
		}

		if (MaterialAsset)
		{
			MaterialAsset->OnMaterialChanged.Remove(MaterialChangedHandle);
		}
	}

	void SMaterialPreviewer::Construct(const FArguments& InArgs)
	{
		MouseButtonDownHandler = InArgs._OnMouseButtonDown;
		MaterialAsset = InArgs._InMaterial;
		Preview = MakeShared<PreviewViewPort>();
		Renderer = MakeUnique<MaterialPreviewRenderer>(MaterialAsset.Get());
		for (auto Value : magic_enum::enum_values<MaterialPreviewPrimitive>())
		{
			PreviewPrimitiveOptions.Add(MakeShared<MaterialPreviewPrimitive>(Value));
		}

		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2, 0)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0, 0, 6, 0)
					[
						SNew(STextBlock)
						.Text(LOCALIZATION("PreviewPrimitive"))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SComboBox<TSharedPtr<MaterialPreviewPrimitive>>)
						.OptionsSource(&PreviewPrimitiveOptions)
						.OnSelectionChanged_Lambda([this](TSharedPtr<MaterialPreviewPrimitive> InItem, ESelectInfo::Type) {
							if (InItem)
							{
								if (PreviewPrimitive == *InItem)
								{
									return;
								}

								PreviewPrimitive = *InItem;
								Render();
							}
						})
						.OnGenerateWidget_Lambda([this](TSharedPtr<MaterialPreviewPrimitive> InItem) {
							const FString PrimitiveName = InItem ? ANSI_TO_TCHAR(magic_enum::enum_name(*InItem).data()) : TEXT("");
							return SNew(STextBlock)
								.Text(PrimitiveName.IsEmpty() ? FText::GetEmpty() : LOCALIZATION(PrimitiveName));
						})
						[
							SNew(STextBlock)
							.Text_Lambda([this] {
								return LOCALIZATION(magic_enum::enum_name(PreviewPrimitive).data());
							})
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SViewport)
						.ViewportInterface(Preview)
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					.Padding(8)
					[
						SAssignNew(ErrorText, STextBlock)
						.Text_Lambda([this]{ return Renderer->GetErrorReason(); })
						.ColorAndOpacity(FLinearColor::Red)
						.AutoWrapText(true)
						.WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
						.Clipping(EWidgetClipping::ClipToBounds)
						.Visibility(EVisibility::Collapsed)
					]
				]
			]
		];

		ResizeHandlerHandle = Preview->ResizeHandler.AddLambda([this](const Vector2f&) {
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
			OrbitYaw += MouseDelta.x * 0.01f;
			OrbitPitch = FMath::Clamp(OrbitPitch - MouseDelta.y * 0.01f, -1.4f, 1.4f);
			Render();
			return FReply::Handled();
		});

		Preview->MouseWheelHandler.BindLambda([this](const FGeometry&, const FPointerEvent& MouseEvent) {
			const float ZoomFactor = FMath::Pow(0.9f, MouseEvent.GetWheelDelta());
			CameraDistance = FMath::Clamp(CameraDistance * ZoomFactor, 1.0f, 10.0f);
			Render();
			return FReply::Handled();
		});

		if (MaterialAsset)
		{
			MaterialChangedHandle = MaterialAsset->OnMaterialChanged.AddLambda([this] {
				Renderer->SetMaterial(MaterialAsset.Get());
				Render();
			});
		}

		Render();
	}

	FReply SMaterialPreviewer::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseButtonDownHandler.IsBound())
		{
			return MouseButtonDownHandler.Execute(MyGeometry, MouseEvent);
		}
		return FReply::Handled();
	}

	void SMaterialPreviewer::OpenMaterialPreviewer(AssetPtr<Material> InMaterial, const FPointerEventHandler& InOnMouseButtonDown, TSharedPtr<SWindow> InParentWindow)
	{
		if (TSharedPtr<SMaterialPreviewer> ExistingPreviewer = Instance.Pin())
		{
			if (TSharedPtr<SWindow> ExistingWindow = FSlateApplication::Get().FindWidgetWindow(ExistingPreviewer.ToSharedRef()))
			{
				ExistingWindow->RequestDestroyWindow();
			}
			Instance.Reset();
		}

		TSharedRef<SWindow> PreviewWindow = SNew(SWindow)
			.Title(LOCALIZATION("MaterialPreview"))
			.ClientSize(FVector2D(512, 512))
			.AutoCenter(EAutoCenter::PreferredWorkArea)
			.SupportsMinimize(true)
			.SupportsMaximize(true);

		TSharedRef<SMaterialPreviewer> Previewer = SNew(SMaterialPreviewer)
			.InMaterial(InMaterial)
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

	void SMaterialPreviewer::Render()
	{
		const FIntPoint ViewportSize = Preview->GetSize();
		Renderer->SetPreviewPrimitive(PreviewPrimitive);
		Renderer->SetOrbit(OrbitYaw, OrbitPitch);
		Renderer->SetCameraDistance(CameraDistance);
		if (Renderer->Render(ViewportSize.X, ViewportSize.Y))
		{
			Preview->SetViewPortRenderTexture(Renderer->GetRenderTarget());
			ErrorText->SetVisibility(EVisibility::Collapsed);
		}
		else
		{
			Renderer->RenderErrorColor(ViewportSize.X, ViewportSize.Y);
			Preview->SetViewPortRenderTexture(Renderer->GetRenderTarget());
			ErrorText->SetVisibility(EVisibility::HitTestInvisible);
		}
	}
}
