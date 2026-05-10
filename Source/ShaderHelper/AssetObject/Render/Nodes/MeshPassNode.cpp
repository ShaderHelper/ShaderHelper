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
	REFLECTION_REGISTER(AddClass<MeshPassColorRT>()
		.Data<&MeshPassColorRT::ClearValue, MetaInfo::Property>(LOCALIZATION("Clear"))
		.Data<&MeshPassColorRT::Format, MetaInfo::Property>(LOCALIZATION("Format"))
	)
	REFLECTION_REGISTER(AddClass<MeshPassNode>("MeshPass Node")
		.BaseClass<GraphNode>()
		.Data<&MeshPassNode::ColorRTs, MetaInfo::Property>(LOCALIZATION("ColorRT"))
	)
	REFLECTION_REGISTER(AddClass<MeshPassNodeOp>()
		.BaseClass<ShObjectOp>()
	)

	REGISTER_NODE_TO_GRAPH(MeshPassNode, "Render")

	MetaType* MeshPassNodeOp::SupportType()
	{
		return GetMetaType<MeshPassNode>();
	}

	void MeshPassNodeOp::OnCancelSelect(ShObject* InObject)
	{
		ShPropertyOp::OnCancelSelect(InObject);

		auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		auto* Node = static_cast<MeshPassNode*>(InObject);
		auto* RenderAsset = static_cast<Render*>(Node->GetOuterMost());
		if (RenderAsset->SelectedMeshRenderObject)
		{
			MeshRenderObject* SelectedMeshRenderObject = static_cast<MeshRenderObject*>(RenderAsset->SelectedMeshRenderObject.Get());
			if (Node->MeshRenderObjects.ContainsByPredicate([SelectedMeshRenderObject](const ObjectPtr<MeshRenderObject>& Object) { return Object.Get() == SelectedMeshRenderObject; }))
			{
				RenderAsset->SelectedMeshRenderObject.Reset();
				if (!ShEditor->IsInDebugging() && ShEditor->GetDebuggaleObject() == SelectedMeshRenderObject)
				{
					ShEditor->SetDebuggableObject(nullptr);
				}
			}
		}
	}

	void MeshPassNodeOp::OnSelect(ShObject* InObject)
	{
		ShPropertyOp::OnSelect(InObject);

		auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		auto* Node = static_cast<MeshPassNode*>(InObject);
		auto* RenderAsset = static_cast<Render*>(Node->GetOuterMost());
		if (RenderAsset->SelectedMeshRenderObject)
		{
			MeshRenderObject* SelectedMeshRenderObject = static_cast<MeshRenderObject*>(RenderAsset->SelectedMeshRenderObject.Get());
			if (Node->MeshRenderObjects.ContainsByPredicate([SelectedMeshRenderObject](const ObjectPtr<MeshRenderObject>& Object) { return Object.Get() == SelectedMeshRenderObject; }))
			{
				RenderAsset->SelectedMeshRenderObject.Reset();
				if (!ShEditor->IsInDebugging() && ShEditor->GetDebuggaleObject() == SelectedMeshRenderObject)
				{
					ShEditor->SetDebuggableObject(nullptr);
				}
			}
		}
	}

	MeshPassNode::MeshPassNode()
	{
		NodeWidth = 170.0f;
		ObjectName = FText::FromString(TEXT("MeshPass"));
		ColorRTs = { MeshPassColorRT{} };
		Preview = MakeShared<PreviewViewPort>();
	}

	MeshPassNode::~MeshPassNode() = default;

	void MeshPassNode::Init()
	{
		RebuildPins();
	}

	void MeshPassNode::PostLoad()
	{
		GraphNode::PostLoad();
		NormalizePreviewOutputName();
		RebuildPins();
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
		for (int32 i = 0; i < ColorRTs.Num(); ++i)
		{
			Result.Add(ColorPinName(i));
		}
		if (bDepthEnabled)
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

	void MeshPassNode::RebuildPins()
	{
		auto FindReusablePin = [this](const FString& Name, PinDirection Direction) -> ObjectPtr<GraphPin>
		{
			for (const ObjectPtr<GraphPin>& ExistingPin : Pins)
			{
				if (ExistingPin
					&& ExistingPin->Direction == Direction
					&& ExistingPin->ObjectName.ToString() == Name
					&& DynamicCast<GpuTexturePin>(ExistingPin.Get()))
				{
					return ExistingPin;
				}
			}
			return nullptr;
		};

		auto MakeTexturePin = [this, &FindReusablePin](const FString& Name, PinDirection Direction) -> ObjectPtr<GraphPin>
		{
			ObjectPtr<GraphPin> Pin = FindReusablePin(Name, Direction);
			if (!Pin)
			{
				Pin = NewShObject<GpuTexturePin>(this);
			}
			Pin->ObjectName = FText::FromString(Name);
			Pin->Direction = Direction;
			return Pin;
		};

		TArray<ObjectPtr<GraphPin>> NewPins;
		for (int32 i = 0; i < ColorRTs.Num(); ++i)
		{
			NewPins.Add(MakeTexturePin(ColorPinName(i), PinDirection::Input));
			NewPins.Add(MakeTexturePin(ColorPinName(i), PinDirection::Output));
		}
		if (bDepthEnabled)
		{
			NewPins.Add(MakeTexturePin(TEXT("Depth"), PinDirection::Input));
			NewPins.Add(MakeTexturePin(TEXT("Depth"), PinDirection::Output));
		}

		// Break links on pins that are being removed.
		Graph* OwnerGraph = static_cast<Graph*>(GetOuterMost());
		for (auto& OldP : Pins)
		{
			bool Kept = NewPins.ContainsByPredicate([&](const ObjectPtr<GraphPin>& NP) { return NP.Get() == OldP.Get(); });
			if (!Kept)
			{
				if (OldP->Direction == PinDirection::Output)
				{
					TArray<ObserverObjectPtr<GraphPin>> Targets;
					OutPinToInPin.MultiFind(OldP, Targets);
					for (auto& Target : Targets)
					{
						if (Target.IsValid())
						{
							OwnerGraph->RemoveLink(OldP.Get(), Target.Get());
						}
					}
				}
				else if (OldP->SourcePin.IsValid())
				{
					OwnerGraph->RemoveLink(OldP->GetSourcePin(), OldP.Get());
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
		if (ShEditor->GetDebuggaleObject() == InObject)
		{
			if (ShEditor->IsInDebugging())
			{
				ShEditor->EndDebugging();
			}
			ShEditor->SetDebuggableObject(nullptr);
		}
		if (auto* RenderAsset = static_cast<Render*>(GetOuterMost()); RenderAsset->SelectedMeshRenderObject.Get() == InObject)
		{
			RenderAsset->SelectedMeshRenderObject.Reset();
		}
		if (ShEditor->GetCurPropertyObject() == InObject)
		{
			ShEditor->ShowProperty(this);
		}
		// Break any links from override pins before removal.
		Graph* OwnerGraph = static_cast<Graph*>(GetOuterMost());
		for (auto& OP : InObject->OverridePins)
		{
			if (OP->SourcePin.IsValid())
			{
				OwnerGraph->RemoveLink(OP->GetSourcePin(), OP.Get());
			}
		}
		MeshRenderObjects.RemoveAll([InObject](const ObjectPtr<MeshRenderObject>& E) { return E.Get() == InObject; });
		if (auto* OM = GetOuterMost()) OM->MarkDirty();
	}

	void MeshPassNode::OnRenderTargetsChanged()
	{
		NormalizePreviewOutputName();
		RebuildPins();
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

	DebugTargetInfo MeshPassNode::MakeDebugTargetInfo(TRefCountPtr<GpuTexture> CoverageMask) const
	{
		DebugTargetInfo Target;
		for (int32 i = 0; i < LastActiveColorRTs.Num(); ++i)
		{
			if (LastActiveColorRTs[i])
			{
				Target.Outputs.Add({ ColorPinName(i), LastActiveColorRTs[i] });
			}
		}
		if (LastActiveDepthRT)
		{
			Target.Outputs.Add({ TEXT("Depth"), LastActiveDepthRT });
		}
		for (int32 Index = 0; Index < Target.Outputs.Num(); ++Index)
		{
			if (Target.Outputs[Index].Name == PreviewOutputName)
			{
				Target.SelectedOutputIndex = Index;
				break;
			}
		}
		Target.CoverageMask = CoverageMask;
		Target.Normalize();
		return Target;
	}

	void MeshPassNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);

		Ar << ColorRTs;
		Ar << DepthFormat;
		Ar << bDepthEnabled;
		Ar << PreviewOutputName;

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
		uint32 Width  = (RTSize.X > 0) ? RTSize.X : Ctx.FinalRT->GetWidth();
		uint32 Height = (RTSize.Y > 0) ? RTSize.Y : Ctx.FinalRT->GetHeight();
		if (Width == 0 || Height == 0)
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" execution failed because render target size is invalid (%u x %u)."), *ObjectName.ToString(), Width, Height);
			return { true, true };
		}

		NormalizePreviewOutputName();

		// (Re)create RTs if size/formats changed.
		auto HasColorRTTextureDescChanged = [this]() {
			if (CachedColorRTSettings.Num() != ColorRTs.Num())
			{
				return true;
			}
			for (int32 i = 0; i < ColorRTs.Num(); ++i)
			{
				if (CachedColorRTSettings[i].Format != ColorRTs[i].Format
					|| CachedColorRTSettings[i].ClearValue != ColorRTs[i].ClearValue)
				{
					return true;
				}
			}
			return false;
		};

		bool bNeedRebuild = CachedRTSize.X != Width || CachedRTSize.Y != Height
			|| bCachedDepthEnabled != bDepthEnabled
			|| (bDepthEnabled && CachedDepthFormat != DepthFormat)
			|| HasColorRTTextureDescChanged()
			|| CachedColorRTs.Num() != ColorRTs.Num();

		if (bNeedRebuild)
		{
			CachedColorRTs.Reset();
			CachedDepthRT.SafeRelease();
			for (int32 i = 0; i < ColorRTs.Num(); ++i)
			{
				GpuTextureDesc D{
					.Width = Width, .Height = Height,
					.Format = ToGpuFormat(ColorRTs[i].Format),
					.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::ShaderResource,
					.ClearValues = ColorRTs[i].ClearValue,
				};
				CachedColorRTs.Add(GGpuRhi->CreateTexture(D, GpuResourceState::RenderTargetWrite));
			}
			if (bDepthEnabled)
			{
				GpuTextureDesc D{
					.Width = Width, .Height = Height,
					.Format = ToGpuFormat(DepthFormat),
					.Usage = GpuTextureUsage::DepthStencil | GpuTextureUsage::ShaderResource,
				};
				CachedDepthRT = GGpuRhi->CreateTexture(D, GpuResourceState::DepthStencilWrite);
			}
			CachedRTSize = FW::Vector2u(Width, Height);
			CachedColorRTSettings = ColorRTs;
			bCachedDepthEnabled = bDepthEnabled;
			CachedDepthFormat = DepthFormat;
		}

		TArray<TRefCountPtr<GpuTexture>> ActiveColorRTs;
		ActiveColorRTs.SetNum(CachedColorRTs.Num());
		TArray<uint8> ColorInputConnected;
		ColorInputConnected.SetNumZeroed(CachedColorRTs.Num());
		for (int32 i = 0; i < CachedColorRTs.Num(); ++i)
		{
			ActiveColorRTs[i] = CachedColorRTs[i];
			const FString InputPinName = ColorPinName(i);
			GpuTexturePin* InputPin = DynamicCast<GpuTexturePin>(GetPin(InputPinName, PinDirection::Input));
			if (!InputPin || !InputPin->GetSourcePin())
			{
				continue;
			}

			GpuTexture* InputTexture = InputPin->GetValue();
			const FString& SlotName = InputPinName;
			const GpuFormat ExpectedFormat = ToGpuFormat(ColorRTs[i].Format);

			const GpuTextureDesc& InputDesc = InputTexture->GetResourceDesc();
			if (InputTexture->GetSampleCount() != 1)
			{
				SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" execution failed because %s sample count must be 1."), *ObjectName.ToString(), *SlotName);
				return { true, true };
			}
			if (InputTexture->GetWidth() != Width || InputTexture->GetHeight() != Height)
			{
				SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" execution failed because %s size is %u x %u, expected %u x %u."), *ObjectName.ToString(), *SlotName, InputTexture->GetWidth(), InputTexture->GetHeight(), Width, Height);
				return { true, true };
			}
			if (InputTexture->GetFormat() != ExpectedFormat)
			{
				SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" execution failed because %s format is %d, expected %d."), *ObjectName.ToString(), *SlotName, static_cast<int32>(InputTexture->GetFormat()), static_cast<int32>(ExpectedFormat));
				return { true, true };
			}

			ActiveColorRTs[i] = InputTexture;
			ColorInputConnected[i] = 1;
		}

		TRefCountPtr<GpuTexture> ActiveDepthRT = CachedDepthRT;
		bool bHasDepthInput = false;
		if (bDepthEnabled)
		{
			GpuTexturePin* InputPin = DynamicCast<GpuTexturePin>(GetPin(TEXT("Depth"), PinDirection::Input));
			if (InputPin && InputPin->GetSourcePin())
			{
				GpuTexture* InputTexture = InputPin->GetValue();
				const FString SlotName = TEXT("Depth");
				const GpuFormat ExpectedFormat = ToGpuFormat(DepthFormat);

				if (InputTexture->GetSampleCount() != 1)
				{
					SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" execution failed because %s sample count must be 1."), *ObjectName.ToString(), *SlotName);
					return { true, true };
				}
				if (InputTexture->GetWidth() != Width || InputTexture->GetHeight() != Height)
				{
					SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" execution failed because %s size is %u x %u, expected %u x %u."), *ObjectName.ToString(), *SlotName, InputTexture->GetWidth(), InputTexture->GetHeight(), Width, Height);
					return { true, true };
				}
				if (InputTexture->GetFormat() != ExpectedFormat)
				{
					SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" execution failed because %s format is %d, expected %d."), *ObjectName.ToString(), *SlotName, static_cast<int32>(InputTexture->GetFormat()), static_cast<int32>(ExpectedFormat));
					return { true, true };
				}

				ActiveDepthRT = InputTexture;
				bHasDepthInput = true;
			}
		}

		TRefCountPtr<GpuTexture> PreviewSourceTex;
		if (PreviewOutputName == TEXT("Depth"))
		{
			PreviewSourceTex = ActiveDepthRT;
		}
		else if (PreviewOutputName.StartsWith(TEXT("Color")))
		{
			const int32 PreviewColorIndex = FCString::Atoi(*PreviewOutputName.RightChop(5));
			if (ActiveColorRTs.IsValidIndex(PreviewColorIndex))
			{
				PreviewSourceTex = ActiveColorRTs[PreviewColorIndex];
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
		for (int32 i = 0; i < ActiveColorRTs.Num(); ++i)
		{
			PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
				ActiveColorRTs[i]->GetDefaultView(),
				ColorInputConnected[i] ? RenderTargetLoadAction::Load : RenderTargetLoadAction::Clear,
				RenderTargetStoreAction::Store,
				ColorRTs[i].ClearValue
			});
		}
		if (ActiveDepthRT.IsValid())
		{
			PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{
				ActiveDepthRT->GetDefaultView(),
				bHasDepthInput ? RenderTargetLoadAction::Load : RenderTargetLoadAction::Clear,
				RenderTargetStoreAction::Store,
				1.0f
			};
		}

		if (!ShowTimestampMs())
		{
			GpuTimeMs = 0.0;
			TimestampQuerySet = nullptr;
		}
		else
		{
			if (TimestampQuerySet)
			{
				TArray<uint64> Timestamps;
				TimestampQuerySet->ResolveResults(0, 2, Timestamps);
				double PeriodNs = TimestampQuerySet->GetTimestampPeriodNs();
				GpuTimeMs = (double)(Timestamps[1] - Timestamps[0]) * PeriodNs / 1e6;
			}

			if (GGpuRhi->GetFeature().SupportTimestampQuery() && !TimestampQuerySet)
			{
				TimestampQuerySet = GGpuRhi->CreateQuerySet(2);
			}
		}

		if (TimestampQuerySet)
		{
			PassDesc.TimestampWrites = GpuRenderPassTimestampWrites{ TimestampQuerySet, 0, 1 };
		}

		// Ensure render resources for each MRO with the current format key.
		TArray<GpuFormat> ColorFormatKey;
		for (const MeshPassColorRT& ColorRT : ColorRTs) ColorFormatKey.Add(ToGpuFormat(ColorRT.Format));
		GpuFormat DepthFormatKey = bDepthEnabled ? ToGpuFormat(DepthFormat) : GpuFormat::NUM;
		const uint32 SampleCount = 1;

		for (auto& MRO : MeshRenderObjects)
		{
			MRO->EnsureRenderResources(ColorFormatKey, DepthFormatKey, SampleCount);
		}

		auto ValidateRenderTargetNotShaderInput = [this](GpuTexture* RenderTarget, const FString& RenderTargetName) -> bool {
			for (const auto& MRO : MeshRenderObjects)
			{
				FString BindingName;
				if (MRO->UsesTextureAsShaderInput(RenderTarget, BindingName))
				{
					SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" execution failed because %s is used as both render target and shader input \"%s\" by MeshRenderObject:\"%s\"."),
						*ObjectName.ToString(), *RenderTargetName, *BindingName, *MRO->ObjectName.ToString());
					return false;
				}
			}

			return true;
		};

		for (int32 i = 0; i < ActiveColorRTs.Num(); ++i)
		{
			if (!ValidateRenderTargetNotShaderInput(ActiveColorRTs[i].GetReference(), ColorPinName(i)))
			{
				return { true, true };
			}
		}
		if (!ValidateRenderTargetNotShaderInput(ActiveDepthRT.GetReference(), TEXT("Depth")))
		{
			return { true, true };
		}

		const float Aspect = (float)Width / (float)Height;
		TOptional<Camera> Cam;
		if (CameraRef.IsValid())
		{
			Cam = CameraRef->ToCamera(Aspect);
		}

		LastActiveColorRTs = ActiveColorRTs;
		LastActiveDepthRT = ActiveDepthRT;
		LastColorFormats = ColorFormatKey;
		LastDepthFormat = DepthFormatKey;
		LastSampleCount = SampleCount;
		LastViewPortDesc = GpuViewPortDesc{ (float)Width, (float)Height };
		LastMousePos = Ctx.MousePos;
		LastCamera = Cam;

		TArray<ObjectPtr<MeshRenderObject>> MROsCopy = MeshRenderObjects;
		const Vector2f ViewportSize((float)Width, (float)Height);
		const Vector2f MousePos = Ctx.MousePos;
		const float Time = Ctx.Time;
		auto& RenderPass = Ctx.RG->AddRenderPass(ObjectName.ToString(), PassDesc,
			[MROsCopy, Cam, ViewportSize, MousePos, Time](GpuRenderPassRecorder* Rec) {
				const Camera* CameraPtr = Cam.IsSet() ? &Cam.GetValue() : nullptr;
				for (const auto& MRO : MROsCopy)
				{
					FMatrix44f ModelMat = FMatrix44f::Identity;
					if (MRO->MeshSceneObjectRef.IsValid())
					{
						ModelMat = MRO->MeshSceneObjectRef->GetWorldMatrix();
					}
					MRO->Draw(Rec, CameraPtr, ModelMat, ViewportSize, MousePos, Time);
				}
			}
		);
		TSet<GpuTexture*> ConnectedOverrideTextures;
		for (const auto& MRO : MeshRenderObjects)
		{
			MRO->CollectConnectedOverrideTextures(ConnectedOverrideTextures);
			RenderPass.Write(MRO->GetPrintBuffer()->GetResource());
		}
		for (GpuTexture* Texture : ConnectedOverrideTextures)
		{
			RenderPass.Read(Texture);
		}

		Ctx.RG->Execute();

		bool bAssertError = false;
		for (const auto& MRO : MeshRenderObjects)
		{
			if (MRO)
			{
				const FString LogPrefix = FString::Printf(TEXT("%s:%s"), *ObjectName.ToString(), *MRO->ObjectName.ToString());
				bAssertError |= MRO->FlushPrintBufferLogs(LogPrefix);
			}
		}

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
		for (int32 i = 0; i < ActiveColorRTs.Num(); ++i)
		{
			if (auto* Pin = static_cast<GpuTexturePin*>(GetPin(ColorPinName(i), PinDirection::Output)))
			{
				Pin->SetValue(ActiveColorRTs[i]);
			}
		}
		if (ActiveDepthRT.IsValid())
		{
			if (auto* Pin = static_cast<GpuTexturePin*>(GetPin(TEXT("Depth"), PinDirection::Output)))
			{
				Pin->SetValue(ActiveDepthRT);
			}
		}

		if (bAssertError)
		{
			return { true, true };
		}

		return {};
	}

	TSharedPtr<SWidget> MeshPassNode::ExtraNodeWidget(SGraphNode* OwnerWidget)
	{
		NormalizePreviewOutputName();

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

				RegisterOverridePinWidgets();

				ChildSlot
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SAssignNew(ListView, SListView<FMeshRenderObjectListItemPtr>)
						.AllowOverscroll(EAllowOverscroll::No)
						.ListItemsSource(&Items)
						.SelectionMode(ESelectionMode::None)
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

				TSharedPtr<SVerticalBox> OverridePinBox;
				Row->SetContent(
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.BorderBackgroundColor_Lambda([this, Object] {
							auto& SelectedMeshRenderObject = static_cast<Render*>(Node->GetOuterMost())->SelectedMeshRenderObject;
							if (!SelectedMeshRenderObject || SelectedMeshRenderObject.Get() != Object)
							{
								return FLinearColor::Transparent;
							}
							return HasKeyboardFocus() || HasFocusedDescendants()
								? FStyleColors::Select.GetSpecifiedColor()
								: FStyleColors::SelectInactive.GetSpecifiedColor();
						})
						.Padding(0.0f)
						.OnMouseButtonDown_Lambda([this, Item](const FGeometry&, const FPointerEvent& MouseEvent) {
							if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
							{
								OnSelectionChanged(Item, ESelectInfo::OnMouseClick);
								return FReply::Handled();
							}
							return FReply::Unhandled();
						})
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							.VAlign(VAlign_Center)
							[
								SAssignNew(OverridePinBox, SVerticalBox)
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
									if (Object->MeshSceneObjectRef.IsValid())
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
						TSharedPtr<SGraphPin> PinIcon = GetOverridePinWidget(OP.Get());
						OverridePinBox->AddSlot().AutoHeight().Padding(2,0)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								PinIcon.ToSharedRef()
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

			void RegisterOverridePinWidgets()
			{
				for (const auto& MROPtr : Node->MeshRenderObjects)
				{
					MeshRenderObject* Object = MROPtr.Get();
					for (const auto& OP : Object->OverridePins)
					{
						GetOverridePinWidget(OP.Get());
					}
				}
			}

			TSharedPtr<SGraphPin> GetOverridePinWidget(GraphPin* Pin)
			{
				if (TSharedPtr<SGraphPin>* Found = OverridePinWidgets.Find(Pin))
				{
					return *Found;
				}

				TSharedPtr<SGraphPin> PinIcon = SNew(SGraphPin, OwnerWidget).PinData(Pin);
				OverridePinWidgets.Add(Pin, PinIcon);
				OwnerWidget->Pins.Add(&*PinIcon);
				return PinIcon;
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
					if (OwnerWidget && OwnerWidget->Owner && !OwnerWidget->Owner->IsSelectedNode(OwnerWidget))
					{
						OwnerWidget->Owner->ClearSelectedNode();
						OwnerWidget->Owner->AddSelectedNode(StaticCastSharedRef<SGraphNode>(OwnerWidget->AsShared()));
					}

					auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
					static_cast<Render*>(Node->GetOuterMost())->SelectedMeshRenderObject = SelectedItem->Object;
					if (!ShEditor->IsInDebugging())
					{
						ShEditor->SetDebuggableObject(SelectedItem->Object);
					}
					GApp->GetEditor()->ShowProperty(SelectedItem->Object);
				}
				else if (MeshRenderObject* SelectedObject = GetSelectedObject())
				{
					auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
					static_cast<Render*>(Node->GetOuterMost())->SelectedMeshRenderObject.Reset();
					if (!ShEditor->IsInDebugging() && ShEditor->GetDebuggaleObject() == SelectedObject)
					{
						ShEditor->SetDebuggableObject(nullptr);
					}
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
				return GetSelectedObject() != nullptr;
			}

			bool DeleteSelected()
			{
				MeshRenderObject* SelectedObject = GetSelectedObject();
				if (!SelectedObject)
				{
					return false;
				}

				Node->RemoveMeshRenderObject(SelectedObject);
				Node->RefreshNodeWidget();
				return true;
			}

			MeshRenderObject* GetSelectedObject() const
			{
				auto& SelectedMeshRenderObjectPtr = static_cast<Render*>(Node->GetOuterMost())->SelectedMeshRenderObject;
				MeshRenderObject* SelectedMeshRenderObject = SelectedMeshRenderObjectPtr ? static_cast<MeshRenderObject*>(SelectedMeshRenderObjectPtr.Get()) : nullptr;
				for (const auto& Item : Items)
				{
					if (Item->Object && Item->Object == SelectedMeshRenderObject)
					{
						return Item->Object;
					}
				}
				return nullptr;
			}

			MeshPassNode* Node = nullptr;
			SGraphNode* OwnerWidget = nullptr;
			bool bRecognizedDragDrop = false;
			bool bHovered = false;
			TArray<FMeshRenderObjectListItemPtr> Items;
			TMap<GraphPin*, TSharedPtr<SGraphPin>> OverridePinWidgets;
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
						SNew(SBox).MinDesiredHeight(80.0f)
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
			auto ConfigureClearProperty = [](PropertyData& InPropertyData)
			{
				if (!InPropertyData.GetDisplayName().EqualTo(LOCALIZATION("Clear")) || !InPropertyData.IsOfType<PropertyVector4fItem>())
				{
					return;
				}

				PropertyVector4fItem* ClearItem = static_cast<PropertyVector4fItem*>(&InPropertyData);
				ClearItem->SetUseColorBlockPicker(true);
			};

			auto ColorRTArrayItem = MakeShared<PropertyArrayItem>(
				this,
				LOCALIZATION("ColorRT"),
				[this] { return ColorRTs.Num(); },
				[this](int32 NewNum) { ColorRTs.SetNum(NewNum); }
			);
			ColorRTArrayItem->SetRemoveAt([this](int32 Index) {
				if (ColorRTs.IsValidIndex(Index))
				{
					ColorRTs.RemoveAt(Index);
				}
			});
			ColorRTArrayItem->SetRebuildChildren([this, ConfigureClearProperty](PropertyArrayItem& InArrayItem) {
				MetaType* ColorRTMetaType = GetMetaType<MeshPassColorRT>();
				for (int32 ElementIndex = 0; ElementIndex < ColorRTs.Num(); ++ElementIndex)
				{
					auto ElementItem = MakeShared<PropertyCategory>(this, FText::Format(LOCALIZATION("Element {0}"), FText::AsNumber(ElementIndex)), true);
					ElementItem->SetArrayElementStyle(true);

					for (MetaMemberData* PropertyMember : GetProperties(ColorRTMetaType))
					{
						TArray<TSharedRef<PropertyData>> PropertyDatas = FW::GeneratePropertyDatas(this, PropertyMember, &ColorRTs[ElementIndex], true);
						for (const TSharedRef<PropertyData>& PropertyData : PropertyDatas)
						{
							ConfigureClearProperty(PropertyData.Get());
							ElementItem->AddChild(PropertyData);
						}
					}

					InArrayItem.AddChild(ElementItem);
				}
			});
			RTCat->AddChild(ColorRTArrayItem);

			{
				auto DepthEnabledItem = MakeShared<PropertyScalarItem<bool>>(this, LOCALIZATION("MeshPassDepthEnabled"), &bDepthEnabled);
				RTCat->AddChild(DepthEnabledItem);

				auto EnumItem = MakePropertyEnumItem<MeshPassDepthFormat>(
					this,
					LOCALIZATION("MeshPassDepthFormat"),
					DepthFormat,
					[this](MeshPassDepthFormat Value) {
						DepthFormat = Value;
					});
				RTCat->AddChild(EnumItem);
			}

			auto WItem = MakeShared<PropertyScalarItem<uint32>>(this, LOCALIZATION("MeshPassRTWidthAuto"), &RTSize.X);
			auto HItem = MakeShared<PropertyScalarItem<uint32>>(this, LOCALIZATION("MeshPassRTHeightAuto"), &RTSize.Y);
			RTCat->AddChild(WItem);
			RTCat->AddChild(HItem);
		}
		Result.Add(RTCat);

		return Result;
	}

	bool MeshPassNode::CanChangeProperty(PropertyData* InProperty)
	{
		if (InProperty->IsOfType<PropertyArrayItem>() && InProperty->GetDisplayName().EqualTo(LOCALIZATION("ColorRT")))
		{
			const int32 NewNum = static_cast<PropertyArrayItem*>(InProperty)->GetPendingNum();
			if (NewNum != INDEX_NONE && NewNum > 8)
			{
				auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
				MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, ShEditor->GetMainWindow(), LOCALIZATION("MeshPassColorRTMaxCountTip"));
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
			if (CurProperty->GetDisplayName().EqualTo(LOCALIZATION("ColorRT"))
				|| CurProperty->GetDisplayName().EqualTo(LOCALIZATION("MeshPassDepthEnabled"))
				|| CurProperty->GetDisplayName().EqualTo(LOCALIZATION("MeshPassDepthFormat")))
			{
				OnRenderTargetsChanged();
				break;
			}
		}
	}
}
