#include "CommonHeader.h"
#include "Texture3dEditor.h"
#include "AssetObject/Texture3D.h"
#include "AssetObject/Texture2D.h"
#include "Editor/PreviewViewPort.h"
#include "AssetManager/AssetImporter/AssetImporter.h"
#include "Editor/ShaderHelperEditor.h"
#include "App/App.h"
#include "UI/Widgets/STexturePreviewer.h"

#include <Widgets/SViewport.h>
#include <DesktopPlatformModule.h>
#include <Widgets/Input/SSpinBox.h>

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<Texture3dOp>()
		.BaseClass<ShAssetOp>()
	)

	MetaType* Texture3dOp::SupportType()
	{
		return GetMetaType<Texture3D>();
	}

	class STexture3dCreationDialog : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(STexture3dCreationDialog) {}
			SLATE_ARGUMENT(SWindow*, ParentWindow)
			SLATE_ARGUMENT(bool*, bConfirmed)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ParentWindow = InArgs._ParentWindow;
			bConfirmed = InArgs._bConfirmed;

			SourceViewport = MakeShared<PreviewViewPort>();

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
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(128.f)
							.HeightOverride(128.f)
							[
								SNew(SBorder)
								.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
								[
									SNew(SViewport)
									.Visibility_Lambda([this] {
										return SourceTexture.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
									})
									.ViewportInterface(SourceViewport)
									.ViewportSize(FVector2D(128.f, 128.f))
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
								.Text_Lambda([this] {
									if (SourceFilePath.IsEmpty())
									{
										return LOCALIZATION("None");
									}
									return FText::FromString(FPaths::GetCleanFilename(SourceFilePath));
								})
								.ToolTipText_Lambda([this] {
									return FText::FromString(SourceFilePath);
								})
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0, 4, 0, 0)
							[
								SNew(SButton)
								.Text(LOCALIZATION("SelectImage"))
								.OnClicked_Lambda([this] {
									SelectSourceImage();
									return FReply::Handled();
								})
							]
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 12, 0, 0)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCALIZATION("Columns"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(8, 0, 0, 0)
						[
							SNew(SSpinBox<int32>)
							.MinValue(1)
							.MaxValue(256)
							.Value_Lambda([this] { return Columns; })
							.OnValueChanged_Lambda([this](int32 Val) { Columns = Val; })
							.MinDesiredWidth(80)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(16, 0, 0, 0)
						[
							SNew(STextBlock)
							.Text(LOCALIZATION("Rows"))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(8, 0, 0, 0)
						[
							SNew(SSpinBox<int32>)
							.MinValue(1)
							.MaxValue(256)
							.Value_Lambda([this] { return Rows; })
							.OnValueChanged_Lambda([this](int32 Val) { Rows = Val; })
							.MinDesiredWidth(80)
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 8, 0, 0)
					[
						SNew(STextBlock)
						.Text_Lambda([this] {
							if (!SourceTexture.IsValid())
							{
								return FText();
							}
							uint32 SliceWidth = SourceTexture->GetWidth() / Columns;
							uint32 SliceHeight = SourceTexture->GetHeight() / Rows;
							uint32 DepthVal = Rows * Columns;
							return FText::FromString(FString::Printf(TEXT("%ux%ux%u"), SliceWidth, SliceHeight, DepthVal));
						})
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 8, 0, 0)
					.HAlign(HAlign_Right)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCALIZATION("Ok"))
							.IsEnabled_Lambda([this] { return IsValid(); })
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
		}

		TUniquePtr<Texture2D> SourceTexture;
		int32 Rows = 1;
		int32 Columns = 1;

	private:
		void SelectSourceImage()
		{
			IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
			void* WindowHandle = ParentWindow->GetNativeWindow().IsValid() ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;

			TArray<FString> OpenedFileNames;
			FString Filter = TEXT("Image files (*.png;*.jpg;*.jpeg;*.tga)|*.png;*.jpg;*.jpeg;*.tga");
			if (DesktopPlatform->OpenFileDialog(WindowHandle, LOCALIZATION("SelectSourceImage").ToString(), TEXT(""), TEXT(""), Filter, EFileDialogFlags::None, OpenedFileNames))
			{
				if (OpenedFileNames.Num() > 0)
				{
					AssetImporter* Importer = GetAssetImporter(OpenedFileNames[0]);
					if (Importer)
					{
						TUniquePtr<AssetObject> Asset = Importer->CreateAssetObject(OpenedFileNames[0]);
						if (Asset)
						{
							SourceTexture = TUniquePtr<Texture2D>(static_cast<Texture2D*>(Asset.Release()));
							SourceFilePath = OpenedFileNames[0];
							SourceViewport->SetViewPortRenderTexture(SourceTexture->GetPreviewTexture());
						}
					}
				}
			}
		}

		bool IsValid() const
		{
			if (!SourceTexture.IsValid()) return false;
			if (Rows < 1 || Columns < 1) return false;
			if (SourceTexture->GetWidth() % Columns != 0) return false;
			if (SourceTexture->GetHeight() % Rows != 0) return false;
			return true;
		}

		SWindow* ParentWindow;
		bool* bConfirmed;
		FString SourceFilePath;
		TSharedPtr<PreviewViewPort> SourceViewport;
	};

	bool Texture3dOp::OnCreate(AssetObject* InAsset)
	{
		Texture3D* Tex3dAsset = static_cast<Texture3D*>(InAsset);

		bool bConfirmed = false;

		TSharedRef<SWindow> ModalWindow = SNew(SWindow)
			.Title(LOCALIZATION("CreateTexture3D"))
			.ClientSize(FVector2D(330, 250))
			.SizingRule(ESizingRule::FixedSize)
			.AutoCenter(EAutoCenter::PreferredWorkArea)
			.SupportsMinimize(false)
			.SupportsMaximize(false);

		TSharedRef<STexture3dCreationDialog> Dialog = SNew(STexture3dCreationDialog)
			.ParentWindow(&*ModalWindow)
			.bConfirmed(&bConfirmed);

		ModalWindow->SetContent(Dialog);
		FSlateApplication::Get().AddModalWindow(ModalWindow, FSlateApplication::Get().GetActiveTopLevelRegularWindow());

		if (bConfirmed)
		{
			Texture2D* SrcTex = Dialog->SourceTexture.Get();
			int32 SrcRows = Dialog->Rows;
			int32 SrcColumns = Dialog->Columns;

			uint32 SliceWidth = SrcTex->GetWidth() / SrcColumns;
			uint32 SliceHeight = SrcTex->GetHeight() / SrcRows;
			uint32 Depth = SrcRows * SrcColumns;
			GpuFormat SrcFormat = SrcTex->GetFormat();
			uint32 BytesPerTexel = GetFormatByteSize(SrcFormat);

			const TArray<uint8>& SrcData = SrcTex->GetRawData();
			uint32 SrcRowPitch = SrcTex->GetWidth() * BytesPerTexel;
			uint32 SliceRowPitch = SliceWidth * BytesPerTexel;
			uint32 SliceByteSize = SliceWidth * SliceHeight * BytesPerTexel;

			TArray<uint8> VolumeData;
			VolumeData.SetNum(SliceByteSize * Depth);

			for (int32 Row = 0; Row < SrcRows; Row++)
			{
				for (int32 Col = 0; Col < SrcColumns; Col++)
				{
					int32 SliceIndex = Row * SrcColumns + Col;
					uint8* DstSlice = VolumeData.GetData() + SliceIndex * SliceByteSize;

					for (uint32 Y = 0; Y < SliceHeight; Y++)
					{
						uint32 SrcX = Col * SliceWidth;
						uint32 SrcY = Row * SliceHeight + Y;
						const uint8* SrcRow = SrcData.GetData() + SrcY * SrcRowPitch + SrcX * BytesPerTexel;
						uint8* DstRow = DstSlice + Y * SliceRowPitch;
						FMemory::Memcpy(DstRow, SrcRow, SliceRowPitch);
					}
				}
			}

			Tex3dAsset->SetData(SliceWidth, SliceHeight, Depth, SrcFormat, MoveTemp(VolumeData));
		}
		return bConfirmed;
	}

	void Texture3dOp::OnOpen(const FString& InAssetPath)
	{
		AssetPtr<Texture3D> Asset = TSingleton<AssetManager>::Get().LoadAssetByPath<Texture3D>(InAssetPath);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ShowProperty(Asset.Get());
		STexturePreviewer::OpenTexturePreviewer(Asset, FPointerEventHandler::CreateLambda([=](const FGeometry&, const FPointerEvent&) {
			ShEditor->ShowProperty(Asset.Get());
			return FReply::Handled();
		}), ShEditor->GetMainWindow());
	}
}
