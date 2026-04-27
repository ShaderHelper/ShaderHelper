#include "CommonHeader.h"
#include "MeshPassNode.h"
#include "App/App.h"
#include "AssetObject/Pins/Pins.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "Editor/ShaderHelperEditor.h"
#include "Editor/PreviewViewPort.h"
#include "Renderer/RenderRenderComp.h"
#include "RenderResource/RenderPass/BlitPass.h"
#include "UI/Widgets/Graph/SGraphPanel.h"
#include "UI/Widgets/Graph/SGraphNode.h"
#include "UI/Widgets/Graph/SGraphPin.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "UI/Widgets/Property/PropertyData/PropertyArrayItem.h"
#include "UI/Widgets/Property/PropertyData/PropertyObjectItem.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuResourceHelper.h"

#include <Widgets/Views/SListView.h>
#include <Widgets/SViewport.h>
#include <Widgets/Input/SComboBox.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Layout/SBox.h>

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<MeshPassNode>("MeshPass Node")
		.BaseClass<GraphNode>()
		.Data<&MeshPassNode::ColorRTFormats, MetaInfo::Property>(LOCALIZATION("ColorRTFormats"))
	)
	REFLECTION_REGISTER(AddClass<MeshPassNodeOp>()
		.BaseClass<ShObjectOp>()
	)

	REGISTER_NODE_TO_GRAPH(MeshPassNode, "Render")

	MetaType* MeshPassNodeOp::SupportType()
	{
		return GetMetaType<MeshPassNode>();
	}

	MeshPassNode::MeshPassNode()
	{
		NodeWidth = 170.0f;
		ObjectName = FText::FromString(TEXT("MeshPass"));
		ColorRTFormats = { MeshPassColorFormat::B8G8R8A8_UNORM };
		Preview = MakeShared<PreviewViewPort>();
	}

	MeshPassNode::~MeshPassNode() = default;

	void MeshPassNode::Init()
	{
		RebuildOutputPins();
	}

	GpuFormat MeshPassNode::ToGpuFormat(MeshPassColorFormat F)
	{
		switch (F)
		{
		case MeshPassColorFormat::B8G8R8A8_UNORM:     return GpuFormat::B8G8R8A8_UNORM;
		case MeshPassColorFormat::R16G16B16A16_FLOAT: return GpuFormat::R16G16B16A16_FLOAT;
		case MeshPassColorFormat::R32G32B32A32_FLOAT: return GpuFormat::R32G32B32A32_FLOAT;
		case MeshPassColorFormat::R32_FLOAT:          return GpuFormat::R32_FLOAT;
		case MeshPassColorFormat::R16_FLOAT:          return GpuFormat::R16_FLOAT;
		case MeshPassColorFormat::R32G32_FLOAT:       return GpuFormat::R32G32_FLOAT;
		}
		AUX::Unreachable();
	}

	GpuFormat MeshPassNode::ToGpuFormat(MeshPassDepthFormat F)
	{
		if (F == MeshPassDepthFormat::D32_FLOAT) return GpuFormat::D32_FLOAT;
		AUX::Unreachable();
	}

	TArray<FString> MeshPassNode::GetPreviewOutputNames() const
	{
		TArray<FString> Result;
		for (int32 i = 0; i < ColorRTFormats.Num(); ++i)
		{
			Result.Add(ColorPinName(i));
		}
		if (DepthFormat != MeshPassDepthFormat::None)
		{
			Result.Add(TEXT("Depth"));
		}
		return Result;
	}

	void MeshPassNode::NormalizePreviewOutputName()
	{
		const TArray<FString> Options = GetPreviewOutputNames();
		if (Options.Contains(PreviewOutputName))
		{
			return;
		}
		PreviewOutputName = Options.Num() > 0 ? Options[0] : FString();
	}

	void MeshPassNode::RebuildOutputPins()
	{
		// Preserve existing color/depth pins that still fit by matching name.
		TMap<FString, ObjectPtr<GraphPin>> Existing;
		for (auto& P : Pins)
		{
			Existing.Add(P->ObjectName.ToString(), P);
		}

		TArray<ObjectPtr<GraphPin>> NewPins;
		for (int32 i = 0; i < ColorRTFormats.Num(); ++i)
		{
			FString Name = ColorPinName(i);
			ObjectPtr<GraphPin> P;
			if (auto* Found = Existing.Find(Name))
			{
				if (DynamicCast<GpuTexturePin>(Found->Get()))
				{
					P = *Found;
				}
			}
			if (!P)
			{
				P = NewShObject<GpuTexturePin>(this);
				P->ObjectName = FText::FromString(Name);
				P->Direction = PinDirection::Output;
			}
			NewPins.Add(MoveTemp(P));
		}
		if (DepthFormat != MeshPassDepthFormat::None)
		{
			FString Name = TEXT("Depth");
			ObjectPtr<GraphPin> P;
			if (auto* Found = Existing.Find(Name))
			{
				if (DynamicCast<GpuTexturePin>(Found->Get())) P = *Found;
			}
			if (!P)
			{
				P = NewShObject<GpuTexturePin>(this);
				P->ObjectName = FText::FromString(Name);
				P->Direction = PinDirection::Output;
			}
			NewPins.Add(MoveTemp(P));
		}

		// Break links on pins that are being removed.
		for (auto& OldP : Pins)
		{
			bool Kept = NewPins.ContainsByPredicate([&](const ObjectPtr<GraphPin>& NP) { return NP.Get() == OldP.Get(); });
			if (!Kept)
			{
				TArray<ObserverObjectPtr<GraphPin>> Targets;
				OutPinToInPin.MultiFind(OldP, Targets);
				for (auto& T : Targets)
				{
					OutPinToInPin.Remove(OldP, T);
					if (T.IsValid())
					{
						T->SourcePin.Reset();
						T->Refuse();
					}
				}
			}
		}

		Pins = MoveTemp(NewPins);
	}

	MeshRenderObject* MeshPassNode::AddMeshRenderObject(MeshSceneObject* InMeshSceneObject)
	{
		auto MRO = NewShObject<MeshRenderObject>(this);
		MRO->MeshSceneObjectRef = InMeshSceneObject;
		if (InMeshSceneObject)
		{
			MRO->ObjectName = InMeshSceneObject->ObjectName;
		}
		MeshRenderObject* Result = MRO.Get();
		MeshRenderObjects.Add(MoveTemp(MRO));
		if (auto* OM = GetOuterMost()) OM->MarkDirty();
		return Result;
	}

	void MeshPassNode::RemoveMeshRenderObject(MeshRenderObject* InObject)
	{
		if (!InObject) return;
		auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (ShEditor->GetCurPropertyObject() == InObject)
		{
			ShEditor->ShowProperty(this);
		}
		// Break any links from override pins before removal.
		for (auto& OP : InObject->OverridePins)
		{
			if (OP->SourcePin.IsValid())
			{
				if (GraphPin* Src = OP->GetSourcePin())
				{
					GraphNode* SrcNode = static_cast<GraphNode*>(Src->GetOuter());
					SrcNode->OutPinToInPin.Remove(Src, OP.Get());
				}
				OP->SourcePin.Reset();
				OP->Refuse();
			}
		}
		MeshRenderObjects.RemoveAll([InObject](const ObjectPtr<MeshRenderObject>& E) { return E.Get() == InObject; });
		if (auto* OM = GetOuterMost()) OM->MarkDirty();
	}

	void MeshPassNode::OnRenderTargetsChanged()
	{
		NormalizePreviewOutputName();
		RebuildOutputPins();
		RefreshNodeWidget();
	}

	void MeshPassNode::RefreshNodeWidget()
	{
		// Tell the graph panel to refresh this node's widget (so pin layout updates).
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (auto* Panel = ShEditor->GetGraphPanel())
		{
			if (auto* NodeWidget = Panel->GetNode(this)) NodeWidget->RebuildPins();
		}
	}

	void MeshPassNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);

		Ar << ColorRTFormats;
		Ar << DepthFormat;
		Ar << PreviewOutputName;
		if (Ar.IsLoading())
		{
			NormalizePreviewOutputName();
		}

		Ar << CameraRef;
		Ar << RTSize;

		SerializePolymorphicObjectArray(Ar, MeshRenderObjects, this);
	}

	ExecRet MeshPassNode::Exec(GraphExecContext& Context)
	{
		auto& Ctx = static_cast<RenderExecContext&>(Context);
		if (!Ctx.RG || !Ctx.FinalRT.IsValid())
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" execution failed because render graph or final render target is invalid."), *ObjectName.ToString());
			return { true, true };
		}
		if (!CameraRef.IsValid() || !CameraRef.Get())
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" execution failed because no camera is assigned."), *ObjectName.ToString());
			return { true, true };
		}

		uint32 Width  = (RTSize.X > 0) ? RTSize.X : Ctx.FinalRT->GetWidth();
		uint32 Height = (RTSize.Y > 0) ? RTSize.Y : Ctx.FinalRT->GetHeight();
		if (Width == 0 || Height == 0)
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" execution failed because render target size is invalid (%u x %u)."), *ObjectName.ToString(), Width, Height);
			return { true, true };
		}

		NormalizePreviewOutputName();

		// (Re)create RTs if size/formats changed.
		bool bNeedRebuild = CachedRTSize.X != Width || CachedRTSize.Y != Height
			|| CachedColorFormats != ColorRTFormats || CachedDepthFormat != DepthFormat
			|| CachedColorRTs.Num() != ColorRTFormats.Num();

		if (bNeedRebuild)
		{
			CachedColorRTs.Reset();
			CachedDepthRT.SafeRelease();
			for (int32 i = 0; i < ColorRTFormats.Num(); ++i)
			{
				GpuTextureDesc D{
					.Width = Width, .Height = Height,
					.Format = ToGpuFormat(ColorRTFormats[i]),
					.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::ShaderResource,
					.ClearValues = Vector4f(0,0,0,1),
				};
				CachedColorRTs.Add(GGpuRhi->CreateTexture(D, GpuResourceState::RenderTargetWrite));
			}
			if (DepthFormat != MeshPassDepthFormat::None)
			{
				GpuTextureDesc D{
					.Width = Width, .Height = Height,
					.Format = ToGpuFormat(DepthFormat),
					.Usage = GpuTextureUsage::DepthStencil | GpuTextureUsage::ShaderResource,
				};
				CachedDepthRT = GGpuRhi->CreateTexture(D, GpuResourceState::DepthStencilWrite);
			}
			CachedRTSize = FW::Vector2u(Width, Height);
			CachedColorFormats = ColorRTFormats;
			CachedDepthFormat = DepthFormat;
		}

		TRefCountPtr<GpuTexture> PreviewSourceTex;
		if (PreviewOutputName == TEXT("Depth"))
		{
			PreviewSourceTex = CachedDepthRT;
		}
		else if (PreviewOutputName.StartsWith(TEXT("Color")))
		{
			const int32 PreviewColorIndex = FCString::Atoi(*PreviewOutputName.RightChop(5));
			if (CachedColorRTs.IsValidIndex(PreviewColorIndex))
			{
				PreviewSourceTex = CachedColorRTs[PreviewColorIndex];
			}
		}

		if (PreviewSourceTex.IsValid())
		{
			if (!PreviewRT.IsValid() || PreviewRT->GetWidth() != Width || PreviewRT->GetHeight() != Height)
			{
				GpuTextureDesc PreviewDesc{
					.Width = Width,
					.Height = Height,
					.Format = GpuFormat::B8G8R8A8_UNORM,
					.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared,
					.ClearValues = Vector4f(0, 0, 0, 1),
				};
				PreviewRT = GGpuRhi->CreateTexture(PreviewDesc, GpuResourceState::RenderTargetWrite);
				GGpuRhi->SetResourceName("MeshPassPreviewRT", PreviewRT);
				Preview->SetViewPortRenderTexture(PreviewRT);
			}
		}
		else if (PreviewRT.IsValid())
		{
			PreviewRT.SafeRelease();
			Preview->Clear();
		}

		// Build Pass Desc.
		GpuRenderPassDesc PassDesc;
		for (int32 i = 0; i < CachedColorRTs.Num(); ++i)
		{
			PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
				CachedColorRTs[i]->GetDefaultView(),
				RenderTargetLoadAction::Clear,
				RenderTargetStoreAction::Store,
				Vector4f(0,0,0,1)
			});
		}
		if (CachedDepthRT.IsValid())
		{
			PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{
				CachedDepthRT->GetDefaultView(),
				RenderTargetLoadAction::Clear,
				RenderTargetStoreAction::Store,
				1.0f
			};
		}

		// Ensure render resources for each MRO with the current format key.
		TArray<GpuFormat> ColorFormatKey;
		for (auto F : ColorRTFormats) ColorFormatKey.Add(ToGpuFormat(F));
		GpuFormat DepthFormatKey = DepthFormat != MeshPassDepthFormat::None ? ToGpuFormat(DepthFormat) : GpuFormat::NUM;
		const uint32 SampleCount = 1;

		for (auto& MRO : MeshRenderObjects)
		{
			MRO->EnsureRenderResources(ColorFormatKey, DepthFormatKey, SampleCount);
		}

		const float Aspect = (float)Width / (float)Height;
		const Camera Cam = CameraRef->ToCamera(Aspect);

		// Capture MRO pointers; ObjectPtrs ensure lifetime thru lambda.
		TArray<ObjectPtr<MeshRenderObject>> MROsCopy = MeshRenderObjects;

		BindingContext Bindings; // empty; MRO calls SetBindGroups directly, but we don't use RG barriers for bound textures.

		Ctx.RG->AddRenderPass(ObjectName.ToString(), PassDesc, Bindings,
			[MROsCopy, Cam](GpuRenderPassRecorder* Rec, BindingContext&) {
				for (const auto& MRO : MROsCopy)
				{
					FMatrix44f ModelMat = FMatrix44f::Identity;
					if (MRO->MeshSceneObjectRef.IsValid() && MRO->MeshSceneObjectRef.Get())
					{
						ModelMat = MRO->MeshSceneObjectRef->GetWorldMatrix();
					}
					MRO->Draw(Rec, Cam, ModelMat);
				}
			}
		);

		if (PreviewRT.IsValid() && PreviewSourceTex.IsValid())
		{
			BlitPassInput PreviewInput;
			PreviewInput.InputView = PreviewSourceTex->GetDefaultView();
			PreviewInput.InputTexSampler = GpuResourceHelper::GetSampler({});
			PreviewInput.OutputView = PreviewRT->GetDefaultView();
			PreviewInput.LoadAction = RenderTargetLoadAction::Clear;
			AddBlitPass(*Ctx.RG, PreviewInput);
		}

		// Publish outputs.
		for (int32 i = 0; i < CachedColorRTs.Num(); ++i)
		{
			if (auto* Pin = static_cast<GpuTexturePin*>(GetPin(ColorPinName(i))))
			{
				Pin->SetValue(CachedColorRTs[i]);
			}
		}
		if (CachedDepthRT.IsValid())
		{
			if (auto* Pin = static_cast<GpuTexturePin*>(GetPin(TEXT("Depth"))))
			{
				Pin->SetValue(CachedDepthRT);
			}
		}

		return {};
	}

	TSharedPtr<SWidget> MeshPassNode::ExtraNodeWidget(SGraphNode* OwnerWidget)
	{
		NormalizePreviewOutputName();

		// Mark override pins as non-layout so default node pin pass ignores them.
		for (auto& MRO : MeshRenderObjects)
		{
			for (auto& OP : MRO->OverridePins)
			{
				// Append to node's Pins array so SGraphPanel::GetGraphPin(Guid) can find them.
				if (!Pins.Contains(OP)) Pins.Add(OP);
			}
		}

		struct FMeshRenderObjectListItem
		{
			MeshRenderObject* Object = nullptr;
		};
		using FMeshRenderObjectListItemPtr = TSharedPtr<FMeshRenderObjectListItem>;

		class SMeshRenderObjectList : public SCompoundWidget
		{
		public:
			SLATE_BEGIN_ARGS(SMeshRenderObjectList){}
				SLATE_ARGUMENT(MeshPassNode*, Node)
				SLATE_ARGUMENT(SGraphNode*, OwnerWidget)
			SLATE_END_ARGS()

			void Construct(const FArguments& InArgs)
			{
				Node = InArgs._Node;
				OwnerWidget = InArgs._OwnerWidget;

				for (const auto& MROPtr : Node->MeshRenderObjects)
				{
					auto Item = MakeShared<FMeshRenderObjectListItem>();
					Item->Object = MROPtr.Get();
					Items.Add(Item);
				}

				ChildSlot
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SAssignNew(ListView, SListView<FMeshRenderObjectListItemPtr>)
						.AllowOverscroll(EAllowOverscroll::No)
						.ListItemsSource(&Items)
						.SelectionMode(ESelectionMode::Single)
						.OnGenerateRow(this, &SMeshRenderObjectList::GenerateRowForItem)
						.OnSelectionChanged(this, &SMeshRenderObjectList::OnSelectionChanged)
						.OnContextMenuOpening(this, &SMeshRenderObjectList::CreateContextMenu)
					]
					+ SOverlay::Slot()
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("Debug.Border"))
						.BorderBackgroundColor_Lambda([this] {
							if (bRecognizedDragDrop)
							{
								return FAppStyle::Get().GetBrush("Brushes.Highlight")->TintColor.GetSpecifiedColor();
							}
							return FAppStyle::Get().GetBrush("Brushes.Hover")->TintColor.GetSpecifiedColor();
						})
						.Padding(0.0f)
						.Visibility_Lambda([this] {
							return bRecognizedDragDrop || bHovered ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
						})
						[
							SNullWidget::NullWidget
						]
					]
				];

				for (const auto& Item : Items)
				{
					if (Item->Object && static_cast<ShaderHelperEditor*>(GApp->GetEditor())->GetCurPropertyObject() == Item->Object)
					{
						ListView->SetSelection(Item, ESelectInfo::Direct);
						break;
					}
				}
			}

			void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
			{
				bHovered = true;
				SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);
			}

			void OnMouseLeave(const FPointerEvent& MouseEvent) override
			{
				bHovered = false;
				SCompoundWidget::OnMouseLeave(MouseEvent);
			}

			TSharedRef<ITableRow> GenerateRowForItem(FMeshRenderObjectListItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
			{
				using FMeshRenderObjectTableRow = STableRow<FMeshRenderObjectListItemPtr>;
				MeshRenderObject* Object = Item->Object;
				TSharedRef<FMeshRenderObjectTableRow> Row = SNew(FMeshRenderObjectTableRow, OwnerTable);

				TSharedPtr<SHorizontalBox> OverridePinBox;
				Row->SetContent(
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("NoBrush"))
						.Padding(0.0f)
						.OnMouseButtonDown_Lambda([this, Row, Item](const FGeometry&, const FPointerEvent& MouseEvent) {
							if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Row->IsSelected())
							{
								OnSelectionChanged(Item, ESelectInfo::OnMouseClick);
							}
							return FReply::Unhandled();
						})
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SAssignNew(OverridePinBox, SHorizontalBox)
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							.VAlign(VAlign_Center)
							.Padding(4.0f, 0.0f, 0.0f, 0.0f)
							[
								SNew(STextBlock)
								.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
								.Text_Lambda([Object] {
									if (!Object)
									{
										return FText::GetEmpty();
									}
									if (Object->MeshSceneObjectRef.IsValid() && Object->MeshSceneObjectRef.Get())
									{
										return Object->MeshSceneObjectRef->ObjectName;
									}
									return Object->ObjectName;
								})
							]
						]
				);

				if (Object && OwnerWidget && OverridePinBox)
				{
					for (const auto& OP : Object->OverridePins)
					{
						auto PinIcon = SNew(SGraphPin, OwnerWidget).PinData(OP.Get());
						OwnerWidget->Pins.Add(&*PinIcon);
						OverridePinBox->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(2,0)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								PinIcon
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(2,0,0,0)
							[
								SNew(STextBlock).Text(OP->ObjectName)
							]
						];
					}
				}

				return Row;
			}

			virtual bool SupportsKeyboardFocus() const override { return true; }

			FReply OnKeyDown(const FGeometry&, const FKeyEvent& InKeyEvent) override
			{
				if (InKeyEvent.GetKey() == EKeys::Delete && DeleteSelected())
				{
					return FReply::Handled();
				}
				return FReply::Unhandled();
			}

			void OnDragEnter(FGeometry const&, FDragDropEvent const& E) override
			{
				auto Op = E.GetOperation();
				if (Op && Op->IsOfType<ShObjectDragDropOp>())
				{
					ShObject* Obj = StaticCastSharedPtr<ShObjectDragDropOp>(Op)->Object;
					bRecognizedDragDrop = dynamic_cast<MeshSceneObject*>(Obj) != nullptr;
					if (bRecognizedDragDrop)
					{
						Op->SetCursorOverride(EMouseCursor::GrabHand);
					}
					else
					{
						Op->SetCursorOverride(EMouseCursor::SlashedCircle);
					}
				}
			}

			void OnDragLeave(const FDragDropEvent& E) override
			{
				bRecognizedDragDrop = false;
				if (auto Op = E.GetOperation())
				{
					Op->SetCursorOverride(TOptional<EMouseCursor::Type>());
				}
			}

			FReply OnDrop(const FGeometry&, const FDragDropEvent& E) override
			{
				bRecognizedDragDrop = false;
				auto Op = E.GetOperation();
				if (Op && Op->IsOfType<ShObjectDragDropOp>())
				{
					ShObject* Obj = StaticCastSharedPtr<ShObjectDragDropOp>(Op)->Object;
					if (auto* MSO = dynamic_cast<MeshSceneObject*>(Obj))
					{
						Node->AddMeshRenderObject(MSO);
						Node->RefreshNodeWidget();
						Op->SetCursorOverride(TOptional<EMouseCursor::Type>());
						return FReply::Handled();
					}
				}
				return FReply::Unhandled();
			}

			void OnSelectionChanged(FMeshRenderObjectListItemPtr SelectedItem, ESelectInfo::Type SelectInfo)
			{
				if (SelectInfo != ESelectInfo::Direct)
				{
					FSlateApplication::Get().SetKeyboardFocus(AsShared());
				}
				if (SelectedItem && SelectedItem->Object)
				{
					GApp->GetEditor()->ShowProperty(SelectedItem->Object);
				}
			}

			TSharedPtr<SWidget> CreateContextMenu()
			{
				if (!HasSelectedObject())
				{
					return SNullWidget::NullWidget;
				}

				FMenuBuilder MenuBuilder(true, nullptr);
				MenuBuilder.AddMenuEntry(
					LOCALIZATION("Delete"),
					FText::GetEmpty(),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([this] { DeleteSelected(); })));
				return MenuBuilder.MakeWidget();
			}

		private:
			bool HasSelectedObject() const
			{
				const TArray<FMeshRenderObjectListItemPtr> SelectedItems = ListView->GetSelectedItems();
				return SelectedItems.Num() > 0 && SelectedItems[0]->Object;
			}

			bool DeleteSelected()
			{
				const TArray<FMeshRenderObjectListItemPtr> SelectedItems = ListView->GetSelectedItems();
				if (SelectedItems.Num() == 0 || !SelectedItems[0]->Object)
				{
					return false;
				}

				Node->RemoveMeshRenderObject(SelectedItems[0]->Object);
				Node->RefreshNodeWidget();
				return true;
			}

			MeshPassNode* Node = nullptr;
			SGraphNode* OwnerWidget = nullptr;
			bool bRecognizedDragDrop = false;
			bool bHovered = false;
			TArray<FMeshRenderObjectListItemPtr> Items;
			TSharedPtr<SListView<FMeshRenderObjectListItemPtr>> ListView;
		};

		TSharedRef<SMeshRenderObjectList> RowsList = SNew(SMeshRenderObjectList)
			.Node(this)
			.OwnerWidget(OwnerWidget);

		auto RefreshPreviewOptions = [this]() {
			PreviewOptions.Reset();
			for (const FString& Name : GetPreviewOutputNames())
			{
				PreviewOptions.Add(MakeShared<FString>(Name));
			}
		};
		RefreshPreviewOptions();

		return SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 4, 0)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Camera")))
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SPropertyObject)
						.ObjectMetaType(GetMetaType<CameraSceneObject>())
						.Padding(FMargin(6.0f, 3.0f))
						.GetObject([this]() -> ShObject* {
							return CameraRef.Get();
						})
						.SetObject([this](ShObject* Object) {
							CameraRef = static_cast<CameraSceneObject*>(Object);
							if (auto* OM = GetOuterMost()) OM->MarkDirty();
							static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(SBorder).BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
					[
						SNew(SBox).MinDesiredHeight(60.0f)
						[
							RowsList
						]
						
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SComboBox<TSharedPtr<FString>>)
							.OptionsSource(&PreviewOptions)
							.OnComboBoxOpening_Lambda([this, RefreshPreviewOptions] {
							NormalizePreviewOutputName();
							RefreshPreviewOptions();
						})
						.OnSelectionChanged_Lambda([this](TSharedPtr<FString> InItem, ESelectInfo::Type) {
							if (InItem)
							{
								PreviewOutputName = *InItem;
								if (auto* OM = GetOuterMost()) OM->MarkDirty();
							}
						})
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem) {
							return SNew(STextBlock).Text(FText::FromString(InItem ? *InItem : FString()));
						})
						[
							SNew(STextBlock)
							.Text_Lambda([this] {
								return FText::FromString(PreviewOutputName.IsEmpty() ? TEXT("(None)") : PreviewOutputName);
							})
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						
						SNew(SViewport)
							.ViewportInterface(Preview).ViewportSize(FVector2D{ 100, 100 })
					]
				];
	}

	TArray<TSharedRef<PropertyData>> MeshPassNode::GeneratePropertyDatas()
	{
		TArray<TSharedRef<PropertyData>> Result;

		auto RTCat = MakeShared<PropertyCategory>(this, LOCALIZATION("MeshPassRenderTargets").ToString());
		{
			if (const MetaMemberData* ColorRTFormatsMetaData = GetMetaType<MeshPassNode>()->GetMetaMemberData(LOCALIZATION("ColorRTFormats")))
			{
				RTCat->AddChilds(FW::GeneratePropertyDatas(this, ColorRTFormatsMetaData, this, true));
			}

			{
				auto EnumItem = MakePropertyEnumItem<MeshPassDepthFormat>(
					this,
					LOCALIZATION("MeshPassDepthFormat"),
					DepthFormat,
					[this](MeshPassDepthFormat Value) {
						DepthFormat = Value;
					});
				RTCat->AddChild(EnumItem);
			}
		}
		Result.Add(RTCat);

		// RT Size (two scalar items).
		{
			auto SizeCat = MakeShared<PropertyCategory>(this, LOCALIZATION("MeshPassRTSizeAuto").ToString());
			auto WItem = MakeShared<PropertyScalarItem<uint32>>(this, TEXT("Width"), &RTSize.X);
			auto HItem = MakeShared<PropertyScalarItem<uint32>>(this, TEXT("Height"), &RTSize.Y);
			SizeCat->AddChild(WItem);
			SizeCat->AddChild(HItem);
			Result.Add(SizeCat);
		}

		return Result;
	}

	bool MeshPassNode::CanChangeProperty(PropertyData* InProperty)
	{
		if (InProperty->IsOfType<PropertyArrayItem>() && InProperty->GetDisplayName().EqualTo(LOCALIZATION("ColorRTFormats")))
		{
			const int32 NewNum = static_cast<PropertyArrayItem*>(InProperty)->GetPendingNum();
			if (NewNum != INDEX_NONE && NewNum > 8)
			{
				auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
				MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, ShEditor->GetMainWindow(), LOCALIZATION("MeshPassColorRTFormatsMaxCountTip"));
				return false;
			}
		}

		return true;
	}

	void MeshPassNode::PostPropertyChanged(PropertyData* InProperty)
	{
		GraphNode::PostPropertyChanged(InProperty);

		for (PropertyData* CurProperty = InProperty; CurProperty; CurProperty = CurProperty->GetParent())
		{
			if (CurProperty->GetDisplayName().EqualTo(LOCALIZATION("ColorRTFormats"))
				|| CurProperty->GetDisplayName().EqualTo(LOCALIZATION("MeshPassDepthFormat")))
			{
				OnRenderTargetsChanged();
				break;
			}
		}
	}
}
