#include "CommonHeader.h"
#include "TextureCubeEditor.h"
#include "AssetObject/TextureCube.h"
#include "AssetObject/Texture2D.h"
#include "Editor/PreviewViewPort.h"
#include "AssetManager/AssetImporter/AssetImporter.h"

#include <Widgets/SViewport.h>
#include <DesktopPlatformModule.h>

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<TextureCubeOp>()
		.BaseClass<ShAssetOp>()
	)

	MetaType* TextureCubeOp::SupportType()
	{
		return GetMetaType<TextureCube>();
	}

	struct FaceImageData
	{
		TUniquePtr<Texture2D> Texture;
		FString FilePath;
	};
	 
	static bool LoadFaceImage(const FString& InFilePath, FaceImageData& OutData)
	{
		AssetImporter* Importer = GetAssetImporter(InFilePath);
		if (!Importer)
		{
			return false;
		}

		TUniquePtr<AssetObject> Asset = Importer->CreateAssetObject(InFilePath);
		if (!Asset)
		{
			return false;
		}

		OutData.Texture = TUniquePtr<Texture2D>(static_cast<Texture2D*>(Asset.Release()));
		OutData.FilePath = InFilePath;
		return true;
	}

	class SCubemapCreationDialog : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SCubemapCreationDialog) {}
			SLATE_ARGUMENT(SWindow*, ParentWindow)
			SLATE_ARGUMENT(bool*, bConfirmed)
			SLATE_ARGUMENT(FaceImageData*, FaceImages)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ParentWindow = InArgs._ParentWindow;
			bConfirmed = InArgs._bConfirmed;
			FaceImages = InArgs._FaceImages;

			for (int32 i = 0; i < 6; i++)
			{
				FaceViewports[i] = MakeShared<PreviewViewPort>();
			}

			static const TCHAR* FaceNames[] = {
				TEXT("FaceRightX"), TEXT("FaceLeftX"),
				TEXT("FaceTopY"), TEXT("FaceBottomY"),
				TEXT("FaceFrontZ"), TEXT("FaceBackZ")
			};

			TSharedPtr<SVerticalBox> FaceGrid;

			ChildSlot
			[
				SNew(SBorder)
				.Padding(16.f)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(FaceGrid, SVerticalBox)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 12, 0, 0)
					.HAlign(HAlign_Right)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCALIZATION("Ok"))
							.IsEnabled_Lambda([this]{ return AllFacesValid(); })
							.OnClicked_Lambda([this] {
								*bConfirmed = true;
								ParentWindow->RequestDestroyWindow();
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(8, 0, 0, 0)
						[
							SNew(SButton)
							.Text(LOCALIZATION("Cancel"))
							.OnClicked_Lambda([this] {
								*bConfirmed = false;
								ParentWindow->RequestDestroyWindow();
								return FReply::Handled();
							})
						]
					]
				]
			];

			for (int32 i = 0; i < 6; i += 2)
			{
				FaceGrid->AddSlot()
				.AutoHeight()
				.Padding(0, 2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						CreateFaceRow(i, FaceNames[i])
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(8, 0, 0, 0)
					[
						CreateFaceRow(i + 1, FaceNames[i + 1])
					]
				];
			}
		}

	private:
		TSharedRef<SWidget> CreateFaceRow(int32 FaceIndex, const TCHAR* FaceName)
		{
			static const float PreviewSize = 64.0f;

			return SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 4)
				[
					SNew(STextBlock)
						.Text(LOCALIZATION(FaceName))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(PreviewSize)
						.HeightOverride(PreviewSize)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
							[
								SNew(SViewport)
								.Visibility_Lambda([this, FaceIndex] {
									return FaceImages[FaceIndex].Texture ? EVisibility::Visible : EVisibility::Collapsed;
								})
								.ViewportInterface(FaceViewports[FaceIndex])
								.ViewportSize(FVector2D(PreviewSize, PreviewSize))
							]
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					.Padding(8, 0)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([this, FaceIndex] {
								if (FaceImages[FaceIndex].FilePath.IsEmpty())
								{
									return LOCALIZATION("None");
								}
								return FText::FromString(FPaths::GetCleanFilename(FaceImages[FaceIndex].FilePath));
							})
							.ToolTipText_Lambda([this, FaceIndex] {
								return FText::FromString(FaceImages[FaceIndex].FilePath);
							})
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0, 4, 0, 0)
						[
							SNew(SButton)
								.Text(LOCALIZATION("SelectImage"))
							.OnClicked_Lambda([this, FaceIndex] {
								SelectFaceImage(FaceIndex);
								return FReply::Handled();
							})
						]
					]
				];
		}

		void SelectFaceImage(int32 FaceIndex)
		{
			IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
			void* WindowHandle = ParentWindow->GetNativeWindow().IsValid() ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;

			TArray<FString> OpenedFileNames;
			FString Filter = TEXT("Image files (*.png;*.jpg;*.jpeg;*.tga)|*.png;*.jpg;*.jpeg;*.tga");
			if (DesktopPlatform->OpenFileDialog(WindowHandle, LOCALIZATION("SelectFaceImage").ToString(), TEXT(""), TEXT(""), Filter, EFileDialogFlags::None, OpenedFileNames))
			{
				if (OpenedFileNames.Num() > 0)
				{
					FaceImageData ImageData;
					if (LoadFaceImage(OpenedFileNames[0], ImageData))
					{
						FaceImages[FaceIndex] = MoveTemp(ImageData);
						FaceViewports[FaceIndex]->SetViewPortRenderTexture(FaceImages[FaceIndex].Texture->GetGpuData());
					}
				}
			}
		}

		bool AllFacesValid() const
		{
			for (int32 i = 0; i < 6; i++)
			{
				if (!FaceImages[i].Texture) return false;
				if (FaceImages[i].Texture->GetWidth() != FaceImages[i].Texture->GetHeight()) return false;
			}
			uint32 Size = FaceImages[0].Texture->GetWidth();
			for (int32 i = 1; i < 6; i++)
			{
				if (FaceImages[i].Texture->GetWidth() != Size) return false;
			}
			return true;
		}

		SWindow* ParentWindow;
		bool* bConfirmed;
		FaceImageData* FaceImages;
		TSharedPtr<PreviewViewPort> FaceViewports[6];
	};

	bool TextureCubeOp::OnCreate(AssetObject* InAsset)
	{
		TextureCube* CubeAsset = static_cast<TextureCube*>(InAsset);

		FaceImageData FaceImages[6];
		bool bConfirmed = false;

		TSharedRef<SWindow> ModalWindow = SNew(SWindow)
			.Title(LOCALIZATION("CreateCubemap"))
			.ClientSize(FVector2D(480, 350))
			.SizingRule(ESizingRule::FixedSize)
			.AutoCenter(EAutoCenter::PreferredWorkArea)
			.SupportsMinimize(false)
			.SupportsMaximize(false);

		TSharedRef<SCubemapCreationDialog> Dialog = SNew(SCubemapCreationDialog)
			.ParentWindow(&*ModalWindow)
			.bConfirmed(&bConfirmed)
			.FaceImages(FaceImages);

		ModalWindow->SetContent(Dialog);
		FSlateApplication::Get().AddModalWindow(ModalWindow, FSlateApplication::Get().GetActiveTopLevelRegularWindow());

		if (bConfirmed)
		{
			uint32 FaceSize = FaceImages[0].Texture->GetWidth();
			GpuTextureFormat FaceFormat = FaceImages[0].Texture->GetFormat();

			TArray<TArray<uint8>> AllFaceData;
			AllFaceData.SetNum(6);
			for (int32 i = 0; i < 6; i++)
			{
				AllFaceData[i] = FaceImages[i].Texture->GetRawData();
			}

			CubeAsset->SetData(FaceSize, FaceFormat, MoveTemp(AllFaceData));
		}
		return bConfirmed;
	}
}
