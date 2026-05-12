#include "CommonHeader.h"
#include "ComputePassNode.h"
#include "AssetObject/Pins/Pins.h"
#include "AssetObject/Render/ShaderOverrideHelper.h"
#include "Editor/ShaderHelperEditor.h"
#include "Renderer/MaterialRenderCommon.h"
#include "Renderer/RenderRenderComp.h"
#include "UI/Widgets/Graph/SGraphPanel.h"
#include "UI/Widgets/Graph/SGraphNode.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"
#include "UI/Widgets/Property/PropertyData/PropertyAssetItem.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuResourceHelper.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<ComputePassNode>("ComputePass Node")
		.BaseClass<GraphNode>()
		.Data<&ComputePassNode::ShaderAsset, MetaInfo::Property>(LOCALIZATION("Shader"))
	)
	REFLECTION_REGISTER(AddClass<ComputePassNodeOp>()
		.BaseClass<ShObjectOp>()
	)

	REGISTER_NODE_TO_GRAPH(ComputePassNode, "Render")

	MetaType* ComputePassNodeOp::SupportType()
	{
		return GetMetaType<ComputePassNode>();
	}

	ComputePassNode::ComputePassNode()
	{
		ObjectName = FText::FromString(TEXT("ComputePass"));
	}

	ComputePassNode::ComputePassNode(AssetPtr<Shader> InShader)
		: ShaderAsset(MoveTemp(InShader))
	{
		ObjectName = FText::FromString(TEXT("ComputePass"));
	}

	ComputePassNode::~ComputePassNode()
	{
		UnbindShaderDelegates();
	}

	void ComputePassNode::Init()
	{
		BindShaderDelegates();
		SyncOverridesFromReflection();
	}

	void ComputePassNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
		Ar << ShaderAsset;
		Ar << ThreadGroupCount;
		Ar << OverrideSlots;
	}

	void ComputePassNode::PostLoad()
	{
		GraphNode::PostLoad();
		BindShaderDelegates();
		SyncOverridesFromReflection();
	}

	void ComputePassNode::BindShaderDelegates()
	{
		Shader* Current = ShaderAsset.Get();
		if (BoundShader != Current)
		{
			UnbindShaderDelegates();
		}
		if (Current && !ShaderRefreshedHandle.IsValid())
		{
			BoundShader = Current;
			ShaderRefreshedHandle = Current->OnShaderRefreshed.AddLambda([this] { OnShaderRefreshed(); });
		}
	}

	void ComputePassNode::UnbindShaderDelegates()
	{
		if (BoundShader && ShaderRefreshedHandle.IsValid())
		{
			BoundShader->OnShaderRefreshed.Remove(ShaderRefreshedHandle);
			ShaderRefreshedHandle.Reset();
		}
		BoundShader = nullptr;
	}

	void ComputePassNode::OnShaderRefreshed()
	{
		SyncOverridesFromReflection();
		InvalidateRenderResources();
		RefreshNodeWidget();
	}

	void ComputePassNode::SyncOverridesFromReflection()
	{
		// Collect compute-stage bindings, build target slot list, preserve old values by name.
		TArray<ShaderOverrideSlot> NewSlots;

		GpuShader* Cs = (ShaderAsset && ShaderAsset->IsStageEnabled(ShaderType::Compute))
			? ShaderAsset->GetCompiledShader(ShaderType::Compute)
			: nullptr;

		if (Cs)
		{
			for (const GpuShaderLayoutBinding& Binding : Cs->GetLayout())
			{
				if (Binding.Type == BindingType::UniformBuffer)
				{
					for (const GpuShaderUbMemberInfo& Member : Binding.UbMembers)
					{
						ShaderOverrideSlot Slot;
						Slot.BindingName = Binding.Name;
						Slot.MemberName = Member.Name;
						Slot.Type = Member.Type;
						Slot.Stage = BindingShaderStage::Compute;
						Slot.bIsResource = false;
						NewSlots.Add(MoveTemp(Slot));
					}
				}
				else if (IsPinnableResourceBinding(Binding.Type))
				{
					ShaderOverrideSlot Slot;
					Slot.BindingName = Binding.Name;
					Slot.MemberName.Empty();
					Slot.Type = GetResourceOverrideType(Binding.Type);
					Slot.Stage = BindingShaderStage::Compute;
					Slot.bIsResource = true;
					NewSlots.Add(MoveTemp(Slot));
				}
			}
		}

		PreserveOverrideSlotData(NewSlots, OverrideSlots);

		OverrideSlots = MoveTemp(NewSlots);

		ReconcileOverridePins(this, OverrideSlots, Pins);
	}

	void ComputePassNode::RefreshNodeWidget()
	{
		auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (auto* Panel = ShEditor->GetGraphPanel())
		{
			if (auto* NodeWidget = Panel->GetNode(this)) NodeWidget->RebuildPins();
		}
	}

	void ComputePassNode::InvalidateRenderResources()
	{
		Pipeline.SafeRelease();
		BindGroupLayouts.Empty();
		BindGroups.Empty();
		UniformBuffers.Empty();
	}

	bool ComputePassNode::CanChangeProperty(PropertyData* InProperty)
	{
		if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("ThreadGroupCount")))
		{
			Vector3i Value = static_cast<PropertyVector3iItem*>(InProperty)->GetPendingValue();
			if (Value.x < 1 || Value.y < 1 || Value.z < 1)
			{
				auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
				MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, ShEditor->GetMainWindow(), LOCALIZATION("ComputePassThreadGroupCountTip"));
				return false;
			}
		}

		return true;
	}

	void ComputePassNode::PostPropertyChanged(PropertyData* InProperty)
	{
		GraphNode::PostPropertyChanged(InProperty);
		if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("Shader")))
		{
			BindShaderDelegates();
			SyncOverridesFromReflection();
			InvalidateRenderResources();
			RefreshNodeWidget();
			static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
		}
	}

	TArray<TSharedRef<PropertyData>> ComputePassNode::GeneratePropertyDatas()
	{
		TArray<TSharedRef<PropertyData>> Result = GraphNode::GeneratePropertyDatas();

		auto ThreadCountItem = MakeShared<PropertyVector3iItem>(this, LOCALIZATION("ThreadGroupCount"), reinterpret_cast<int32*>(&ThreadGroupCount.x));
		Result.Add(ThreadCountItem);

		for (ShaderOverrideSlot& Slot : OverrideSlots)
		{
			FString Label = Slot.BindingName + (Slot.MemberName.IsEmpty() ? TEXT("") : (TEXT(".") + Slot.MemberName));
			const FText LabelText = FText::FromString(Label);
			GraphPin* OverridePin = FindOverridePin(OverrideSlots, Pins, Slot.BindingName, Slot.MemberName, BindingShaderStage::Compute);
			TSharedPtr<PropertyItemBase> Entry;
			if (OverridePin && OverridePin->SourcePin.IsValid())
			{
				Entry = MakeShared<PropertyItemBase>(this, LabelText);
				Entry->SetEmbedWidget(
					SNew(STextBlock)
					.TextStyle(&FAppCommonStyle::Get().GetWidgetStyle<FTextBlockStyle>("MinorText"))
					.Text(FText::FromString(Slot.Type))
				);
			}
			else if (Slot.bIsResource)
			{
				Entry = MakeShared<PropertyAssetItem>(this, LabelText, GetTextureMetaType(Slot.Type), &Slot.TextureAsset);
			}
			else
			{
				Entry = MakeBytesPropertyItem(this, LabelText, Slot);
			}
			Result.Add(Entry.ToSharedRef());
		}

		return Result;
	}

	bool ComputePassNode::BuildBindGroups()
	{
		BindGroupLayouts.Empty();
		BindGroups.Empty();
		UniformBuffers.Empty();

		GpuShader* Cs = ShaderAsset->GetCompiledShader(ShaderType::Compute);
		if (!Cs) return false;

		// Group bindings by BindingGroupSlot.
		TMap<BindingGroupSlot, TArray<GpuShaderLayoutBinding>> Grouped;
		for (const GpuShaderLayoutBinding& Binding : Cs->GetLayout())
		{
			Grouped.FindOrAdd(Binding.Group).Add(Binding);
		}

		// Build layouts.
		for (auto& [GroupSlot, Bindings] : Grouped)
		{
			GpuBindGroupLayoutBuilder LayoutBuilder{ GroupSlot };
			for (const auto& Binding : Bindings)
			{
				LayoutBuilder.AddExistingBinding(Binding.Slot, Binding.Type, Binding.Stage);
			}
			BindGroupLayouts.Add(GroupSlot, LayoutBuilder.Build());
		}

		// Build bind groups + uniform buffers.
		for (auto& [GroupSlot, Bindings] : Grouped)
		{
			GpuBindGroupBuilder GroupBuilder{ BindGroupLayouts[GroupSlot] };

			// uniform buffers.
			for (const auto& Binding : Bindings)
			{
				if (Binding.Type != BindingType::UniformBuffer) continue;
				if (!UniformBuffers.Contains(Binding.Name))
				{
					UniformBuffers.Add(Binding.Name, BuildMaterialUniformBufferFromReflection(Binding.UbMembers));
				}
				if (UniformBuffers[Binding.Name])
				{
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, UniformBuffers[Binding.Name]->GetGpuResource(), Binding.Stage);
				}
			}

			// resources.
			for (const auto& Binding : Bindings)
			{
				if (Binding.Type == BindingType::UniformBuffer) continue;

				if (Binding.Name == TEXT("GPrivate_Printer") && Binding.Type == BindingType::RWRawBuffer)
				{
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, GetPrintBuffer()->GetResource(), Binding.Stage);
					continue;
				}

				auto FindSlot = [&] {
					return OverrideSlots.FindByPredicate([&](const ShaderOverrideSlot& S) {
						return S.bIsResource && S.BindingName == Binding.Name;
					});
				};

				switch (Binding.Type)
				{
				case BindingType::Texture:
				case BindingType::TextureCube:
				case BindingType::Texture3D:
				case BindingType::RWTexture:
				case BindingType::RWTexture3D:
				{
					GpuTexture* Tex = ResolveOverrideTexture(Binding.Type, FindOverridePin(OverrideSlots, Pins, Binding.Name, TEXT(""), BindingShaderStage::Compute), FindSlot());
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, Tex->GetDefaultView(), Binding.Stage);
					break;
				}
				case BindingType::Sampler:
				{
					GpuSamplerDesc SamplerDesc;
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, GpuResourceHelper::GetSampler(SamplerDesc), Binding.Stage);
					break;
				}
				case BindingType::CombinedTextureSampler:
				case BindingType::CombinedTextureCubeSampler:
				case BindingType::CombinedTexture3DSampler:
				{
					GpuTexture* Tex = ResolveOverrideTexture(Binding.Type, FindOverridePin(OverrideSlots, Pins, Binding.Name, TEXT(""), BindingShaderStage::Compute), FindSlot());
					GpuSamplerDesc SamplerDesc;
					GpuSampler* Sampler = GpuResourceHelper::GetSampler(SamplerDesc);
					GroupBuilder.SetExistingBinding(Binding.Slot, Binding.Type, new GpuCombinedTextureSampler(Tex->GetDefaultView(), Sampler), Binding.Stage);
					break;
				}
				default:
					break;
				}
			}

			BindGroups.Add(GroupSlot, GroupBuilder.Build());
		}

		return true;
	}

	void ComputePassNode::UpdateUniformBuffers()
	{
		if (UniformBuffers.IsEmpty()) return;

		for (auto& [Name, UB] : UniformBuffers)
		{
			if (!UB) continue;
			for (const auto& [MemberName, MemberInfo] : UB->GetMetaData().Members)
			{
				const ShaderOverrideSlot* OverrideSlot = OverrideSlots.FindByPredicate([&](const ShaderOverrideSlot& Slot) {
					return !Slot.bIsResource && Slot.BindingName == Name && Slot.MemberName == MemberName;
				});
				if (!OverrideSlot) continue;

				const uint8* BytesPtr = nullptr;
				if (GraphPin* Pin = FindOverridePin(OverrideSlots, Pins, Name, MemberName, BindingShaderStage::Compute))
				{
					if (auto* BytesPinValue = DynamicCast<BytesPin>(Pin))
					{
						const TArray<uint8>& PinBytes = BytesPinValue->GetBytes();
						if (Pin->SourcePin.IsValid())
						{
							BytesPtr = GetCompleteOverrideBytes(PinBytes, OverrideSlot->Type);
						}
						else if (const uint8* P = GetCompleteOverrideBytes(PinBytes, OverrideSlot->Type))
						{
							BytesPtr = P;
						}
					}
				}
				if (!BytesPtr)
				{
					BytesPtr = GetCompleteOverrideBytes(OverrideSlot->Bytes, OverrideSlot->Type);
				}

				if (BytesPtr)
				{
					void* Dest = static_cast<uint8*>(UB->GetWriteCombinedData()) + MemberInfo.Offset;
					FMemory::Memcpy(Dest, BytesPtr, MemberInfo.Size);
					void* ReadDest = static_cast<uint8*>(UB->GetReadableData()) + MemberInfo.Offset;
					FMemory::Memcpy(ReadDest, BytesPtr, MemberInfo.Size);
				}
			}
		}
	}

	ExecRet ComputePassNode::Exec(GraphExecContext& Context)
	{
		auto& Ctx = static_cast<RenderExecContext&>(Context);

		if (!ShaderAsset || !ShaderAsset->IsStageEnabled(ShaderType::Compute))
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" has no compute shader assigned."), *ObjectName.ToString());
			return { true, true };
		}
		GpuShader* Cs = ShaderAsset->GetCompiledShader(ShaderType::Compute);
		if (!Cs)
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" compute shader is not compiled."), *ObjectName.ToString());
			return { true, true };
		}

		if (!BuildBindGroups())
		{
			return { true, true };
		}
		UpdateUniformBuffers();

		TArray<GpuBindGroupLayout*> LayoutPtrs;
		TArray<int32> SortedGroupSlots;
		BindGroupLayouts.GetKeys(SortedGroupSlots);
		SortedGroupSlots.Sort();
		for (int32 GroupSlot : SortedGroupSlots)
		{
			LayoutPtrs.Add(BindGroupLayouts[GroupSlot].GetReference());
		}
		GpuComputePipelineStateDesc PipelineDesc{ .Cs = Cs, .BindGroupLayouts = LayoutPtrs };
		Pipeline = GpuPsoCacheManager::Get().CreateComputePipelineState(PipelineDesc);
		if (!Pipeline)
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" failed to create compute pipeline."), *ObjectName.ToString());
			return { true, true };
		}

		const FString PassName = ObjectName.ToString();
		TOptional<GpuPassTimestampWrites> TimestampWrites = GpuResourceHelper::PreparePassTimestampWrites(TimestampQuerySet, GpuTimeMs);

		TArray<TRefCountPtr<GpuBindGroup>> GroupRefs;
		for (int32 GroupSlot : SortedGroupSlots)
		{
			GroupRefs.Add(BindGroups[GroupSlot]);
		}

		auto& Pass = Ctx.RG->AddComputePass(PassName,
			[this, GroupRefs](GpuComputePassRecorder* PassRecorder) {
				TArray<GpuBindGroup*> Groups;
				for (const auto& G : GroupRefs)
				{
					Groups.Add(G.GetReference());
				}
				PassRecorder->SetBindGroups(Groups);
				PassRecorder->SetComputePipelineState(Pipeline);
				PassRecorder->Dispatch(ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z);
			},
			TimestampWrites
		).Write(GetPrintBuffer()->GetResource());

		for (const GpuShaderLayoutBinding& Binding : Cs->GetLayout())
		{
			switch (Binding.Type)
			{
			case BindingType::Texture:
			case BindingType::TextureCube:
			case BindingType::Texture3D:
			case BindingType::CombinedTextureSampler:
			case BindingType::CombinedTextureCubeSampler:
			case BindingType::CombinedTexture3DSampler:
			{
				if (GpuTexture* Tex = ResolveOverrideTexture(Binding.Type,
					FindOverridePin(OverrideSlots, Pins, Binding.Name, TEXT(""), BindingShaderStage::Compute),
					OverrideSlots.FindByPredicate([&](const ShaderOverrideSlot& S) { return S.bIsResource && S.BindingName == Binding.Name; })))
				{
					Pass.Read(Tex);
				}
				break;
			}
			case BindingType::RWTexture:
			case BindingType::RWTexture3D:
			{
				if (GpuTexture* Tex = ResolveOverrideTexture(Binding.Type,
					FindOverridePin(OverrideSlots, Pins, Binding.Name, TEXT(""), BindingShaderStage::Compute),
					OverrideSlots.FindByPredicate([&](const ShaderOverrideSlot& S) { return S.bIsResource && S.BindingName == Binding.Name; })))
				{
					Pass.Write(Tex);
				}
				break;
			}
			default:
				break;
			}
		}

		Ctx.RG->Execute();

		const bool bAssertError = FlushPrintBufferLogs(ObjectName.ToString());
		if (bAssertError)
		{
			return { true, true };
		}

		return {};
	}

	PrintBuffer* ComputePassNode::GetPrintBuffer()
	{
		if (!PrinterBuffer)
		{
			PrinterBuffer = MakeUnique<PrintBuffer>();
		}
		return PrinterBuffer.Get();
	}

	bool ComputePassNode::FlushPrintBufferLogs(const FString& LogPrefix)
	{
		int32 ExtraLineNum = 1;
		ShaderAssertInfo AssertInfo;
		TArray<ShaderPrintInfo> Logs = PrinterBuffer->GetPrintStrings(AssertInfo);
		for (const ShaderPrintInfo& Log : Logs)
		{
			if (Log.Line - ExtraLineNum > 0)
			{
				SH_LOG(LogShader, Display, TEXT("%s:%d:%s"), *LogPrefix, Log.Line - ExtraLineNum, *Log.PrintStr);
			}
			else
			{
				SH_LOG(LogShader, Display, TEXT("%s:%s"), *LogPrefix, *Log.PrintStr);
			}
		}

		const bool bAssertFailed = !AssertInfo.AssertString.IsEmpty();
		if (bAssertFailed)
		{
			if (AssertInfo.Line - ExtraLineNum > 0)
			{
				SH_LOG(LogShader, Error, TEXT("%s:%d:%s"), *LogPrefix, AssertInfo.Line - ExtraLineNum, *AssertInfo.AssertString);
			}
			else
			{
				SH_LOG(LogShader, Error, TEXT("%s:%s"), *LogPrefix, *AssertInfo.AssertString);
			}
		}

		PrinterBuffer->Clear();
		return bAssertFailed;
	}
}
