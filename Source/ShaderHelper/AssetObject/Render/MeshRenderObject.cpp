#include "CommonHeader.h"
#include "MeshRenderObject.h"
#include "ShaderOverrideHelper.h"
#include "Nodes/MeshPassNode.h"
#include "AssetObject/Pins/Pins.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "Editor/ShaderHelperEditor.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/Spirv/SpirvAssertHighlight.h"
#include "GpuApi/Spirv/SpirvParser.h"
#include "Renderer/MaterialRenderCommon.h"
#include "Renderer/RenderGraph.h"
#include "RenderResource/Mesh.h"
#include "RenderResource/RenderPass/BlitPass.h"
#include "GpuApi/GpuResourceHelper.h"
#include "ProjectManager/ShProjectManager.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"
#include "UI/Widgets/Property/PropertyData/PropertyAssetItem.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"

#include <Widgets/Input/SComboButton.h>

#include <stdexcept>

using namespace FW;

namespace SH
{
	namespace
	{
		TArray<uint8> MakeBytesFromData(const void* Data, size_t Size)
		{
			TArray<uint8> Result;
			Result.SetNumUninitialized(static_cast<int32>(Size));
			FMemory::Memcpy(Result.GetData(), Data, Size);
			return Result;
		}

		template<typename ValueType>
		TArray<uint8> MakeBytesFromValue(const ValueType& Value)
		{
			return MakeBytesFromData(&Value, sizeof(ValueType));
		}

		TArray<uint8> MakeDefaultOverrideBytes(const MaterialBindingMemberDefault& Default)
		{
			if (IsShaderMatrix4x4Type(Default.Type))
			{
				return MakeBytesFromData(Default.Values, sizeof(FMatrix44f));
			}
			if (IsShaderFloatType(Default.Type))
			{
				return MakeBytesFromValue(*reinterpret_cast<const float*>(Default.Values));
			}
			if (IsShaderIntType(Default.Type) || IsShaderBoolType(Default.Type))
			{
				return MakeBytesFromValue(*reinterpret_cast<const int32*>(Default.Values));
			}
			if (IsShaderUintType(Default.Type))
			{
				return MakeBytesFromValue(Default.Values[0]);
			}
			if (IsShaderVector2Type(Default.Type))
			{
				return MakeBytesFromData(Default.Values, sizeof(float) * 2);
			}
			if (IsShaderVector3Type(Default.Type))
			{
				return MakeBytesFromData(Default.Values, sizeof(float) * 3);
			}
			if (IsShaderVector4Type(Default.Type))
			{
				return MakeBytesFromData(Default.Values, sizeof(float) * 4);
			}
			if (IsShaderIntVector2Type(Default.Type) || IsShaderBoolVector2Type(Default.Type))
			{
				return MakeBytesFromData(Default.Values, sizeof(int32) * 2);
			}
			if (IsShaderIntVector3Type(Default.Type) || IsShaderBoolVector3Type(Default.Type))
			{
				return MakeBytesFromData(Default.Values, sizeof(int32) * 3);
			}
			if (IsShaderIntVector4Type(Default.Type) || IsShaderBoolVector4Type(Default.Type))
			{
				return MakeBytesFromData(Default.Values, sizeof(int32) * 4);
			}
			if (IsShaderUintVector2Type(Default.Type))
			{
				return MakeBytesFromData(Default.Values, sizeof(Default.Values[0]) * 2);
			}
			if (IsShaderUintVector3Type(Default.Type))
			{
				return MakeBytesFromData(Default.Values, sizeof(Default.Values[0]) * 3);
			}
			if (IsShaderUintVector4Type(Default.Type))
			{
				return MakeBytesFromData(Default.Values, sizeof(Default.Values[0]) * 4);
			}
			return {};
		}

		TRefCountPtr<GpuShader> GetMaskPs()
		{
			static TRefCountPtr<GpuShader> MaskPs;
			if (MaskPs)
			{
				return MaskPs;
			}

			MaskPs = GGpuRhi->CreateShaderFromSource({
				.Name       = TEXT("MeshRenderObject_MaskPs"),
				.Source     = TEXT("float Main() : SV_Target0\n"
					"{\n"
					"    return 1.0;\n"
					"}\n"),
				.Type       = ShaderType::Pixel,
				.EntryPoint = TEXT("Main"),
			});

			FString Err, Warn;
			GGpuRhi->CompileShader(MaskPs, Err, Warn);
			check(Err.IsEmpty());
			return MaskPs;
		}
	}

	REFLECTION_REGISTER(AddClass<MeshRenderObject>("MeshRenderObject")
		.BaseClass<ShObject>()
		.Data<&MeshRenderObject::MeshSceneObjectRef, MetaInfo::Property | MetaInfo::ReadOnly>(LOCALIZATION("MeshSceneObject"))
		.Data<&MeshRenderObject::MaterialAsset, MetaInfo::Property>(LOCALIZATION("Material"))
	)

	MeshRenderObject::MeshRenderObject()
	{
		ObjectName = FText::FromString(TEXT("RenderObject"));
	}

	MeshRenderObject::~MeshRenderObject()
	{
		UnbindMaterialDelegates();
	}

	void MeshRenderObject::BindMaterialDelegates()
	{
		Material* CurrentMaterial = MaterialAsset.Get();
		if (BoundMaterial != CurrentMaterial)
		{
			UnbindMaterialDelegates();
		}

		if (CurrentMaterial && !MaterialChangedHandle.IsValid())
		{
			BoundMaterial = CurrentMaterial;
			MaterialChangedHandle = CurrentMaterial->OnMaterialChanged.AddLambda([this] { OnMaterialChanged(); });
		}
	}

	void MeshRenderObject::UnbindMaterialDelegates()
	{
		if (BoundMaterial && MaterialChangedHandle.IsValid())
		{
			BoundMaterial->OnMaterialChanged.Remove(MaterialChangedHandle);
			MaterialChangedHandle.Reset();
		}
		BoundMaterial = nullptr;
	}

	void MeshRenderObject::OnMaterialChanged()
	{
		if (MaterialAsset)
		{
			for (int32 i = OverrideSlots.Num() - 1; i >= 0; --i)
			{
				ShaderOverrideSlot& Slot = OverrideSlots[i];
				bool bExists = false;
				if (Slot.bIsResource)
				{
					for (const auto& R : MaterialAsset->BindingResourceDefaults)
					{
						if (ShaderOverrideKey{ R.BindingName, TEXT(""), R.Stage } == Slot.Key)
						{
							bExists = true;
							Slot.Type = GetResourceOverrideType(R.BindingType);
							Slot.BindingType = R.BindingType;
							Slot.StructuredStride = R.StructuredStride;
							break;
						}
					}
				}
				else
				{
					for (const auto& M : MaterialAsset->BindingMemberDefaults)
					{
						if (ShaderOverrideKey{ M.BindingName, M.MemberName, M.Stage } == Slot.Key)
						{
							bExists = true;
							if (Slot.Type != M.Type || !GetCompleteOverrideBytes(Slot.Bytes, M.Type))
							{
								Slot.Bytes = MakeDefaultOverrideBytes(M);
							}
							Slot.Type = M.Type;
							break;
						}
					}
				}
				if (!bExists)
				{
					RemoveOverride(i);
				}
			}
		}
		ReconcileOverridePins(this, OverrideSlots, OverridePins);
		InvalidateRenderResources();
		static_cast<MeshPassNode*>(GetOuter())->RefreshNodeWidget();
	}

	void MeshRenderObject::InvalidateRenderResources()
	{
		Pipeline.SafeRelease();
		PipelineDesc = {};
		BindGroupLayouts.Empty();
		BindGroups.Empty();
		UniformBuffers.Empty();
		for (ShaderOverrideSlot& Slot : OverrideSlots)
		{
			Slot.RWOutputTexture.SafeRelease();
		}

		AssertHighlightPs.SafeRelease();
		bHadAssertError = false;
	}

	void MeshRenderObject::Serialize(FArchive& Ar)
	{
		ShObject::Serialize(Ar);
		Ar << MeshSceneObjectRef;
		Ar << MaterialAsset;
		Ar << OverrideSlots;

		SerializePolymorphicObjectArray(Ar, OverridePins, this);
	}

	void MeshRenderObject::PostLoad()
	{
		ShObject::PostLoad();
		BindMaterialDelegates();
		ReconcileOverridePins(this, OverrideSlots, OverridePins);
	}

	TArray<TSharedRef<PropertyData>> MeshRenderObject::GeneratePropertyDatas()
	{
		TArray<TSharedRef<PropertyData>> Result = ShObject::GeneratePropertyDatas();

		auto OverrideCat = MakeShared<PropertyCategory>(this, LOCALIZATION("BindingOverrides"));
		OverrideCat->SetAddMenuWidget(
			SNew(SComboButton)
			.ButtonContent()[SNew(STextBlock).Text(FText::FromString(TEXT("+")))]
			.OnGetMenuContent_Lambda([this]() -> TSharedRef<SWidget> {
				FMenuBuilder MenuBuilder(true, nullptr);
				if (MaterialAsset)
				{
					Material* Mat = MaterialAsset.Get();
					for (const auto& MemberDefault : Mat->BindingMemberDefaults)
					{
						const ShaderOverrideKey Key{ MemberDefault.BindingName, MemberDefault.MemberName, MemberDefault.Stage };
						bool Used = OverrideSlots.ContainsByPredicate([&](const ShaderOverrideSlot& Slot) {
							return !Slot.bIsResource && Slot.Key == Key;
						});
						if (Used)
						{
							continue;
						}

						FString Label = MemberDefault.BindingName + TEXT(".") + MemberDefault.MemberName;
						MenuBuilder.AddMenuEntry(FText::FromString(Label), FText::GetEmpty(), FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([this, MemberDefault] {
								AddOverride(MemberDefault.BindingName, MemberDefault.MemberName, MemberDefault.Type, MemberDefault.Stage, false);
							}))
						);
					}

					for (const auto& ResourceDefault : Mat->BindingResourceDefaults)
					{
						if (!IsResourceOverrideBinding(ResourceDefault.BindingType))
						{
							continue;
						}

						const ShaderOverrideKey Key{ ResourceDefault.BindingName, TEXT(""), ResourceDefault.Stage };
						bool Used = OverrideSlots.ContainsByPredicate([&](const ShaderOverrideSlot& Slot) {
							return Slot.bIsResource && Slot.Key == Key;
						});
						if (Used)
						{
							continue;
						}

						FString TypeStr = GetResourceOverrideType(ResourceDefault.BindingType);
						MenuBuilder.AddMenuEntry(FText::FromString(ResourceDefault.BindingName), FText::GetEmpty(), FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([this, ResourceDefault, TypeStr] {
								AddOverride(ResourceDefault.BindingName, TEXT(""), TypeStr, ResourceDefault.Stage, true);
							}))
						);
					}
				}

				return MenuBuilder.MakeWidget();
			})
		);

		OverrideBuiltInFactories BuiltInFactories{
			.MakeMatrixBuiltInItem  = MakeOverrideBuiltInFactory<BuiltInMatrix4x4Value>(&ShaderOverrideSlot::MatrixBuiltInRaw),
			.MakeFloatBuiltInItem   = MakeOverrideBuiltInFactory<BuiltInFloatValue>(&ShaderOverrideSlot::FloatBuiltInRaw),
			.MakeVector2BuiltInItem = MakeOverrideBuiltInFactory<BuiltInVector2Value>(&ShaderOverrideSlot::Vector2BuiltInRaw),
			.MakeVector3BuiltInItem = MakeOverrideBuiltInFactory<BuiltInVector3Value>(&ShaderOverrideSlot::Vector3BuiltInRaw),
		};

		for (int32 Index = 0; Index < OverrideSlots.Num(); ++Index)
		{
			ShaderOverrideSlot& Slot = OverrideSlots[Index];
			TSharedRef<PropertyItemBase> Entry = MakeOverrideSlotPropertyItem(this, OverrideSlots, Slot, BuiltInFactories);
			int32 EntryIndex = Index;
			Entry->SetOnDelete([this, EntryIndex] {
				RemoveOverride(EntryIndex);
				static_cast<MeshPassNode*>(GetOuter())->RefreshNodeWidget();
				GApp->GetEditor()->RefreshProperty();
			});
			OverrideCat->AddChild(Entry);
		}
		Result.Add(OverrideCat);

		return Result;
	}

	void MeshRenderObject::PostPropertyChanged(PropertyData* InProperty)
	{
		ShObject::PostPropertyChanged(InProperty);
		if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("MeshSceneObject")))
		{
			if (MeshSceneObjectRef.IsValid())
			{
				ObjectName = MeshSceneObjectRef->ObjectName;
			}
		}
		if (InProperty->GetDisplayName().EqualTo(LOCALIZATION("Material")))
		{
			BindMaterialDelegates();
			InvalidateRenderResources();
		}
		if (InProperty->IsOfType<PropertyAssetItem>())
		{
			const FString DisplayName = InProperty->GetDisplayName().ToString();
			const bool bOverrideResourceChanged = OverrideSlots.ContainsByPredicate([&](const ShaderOverrideSlot& Slot) {
				return Slot.bIsResource && MakeOverrideKeyLabel(Slot.Key) == DisplayName;
			});
			if (bOverrideResourceChanged)
			{
				InvalidateRenderResources();
				const bool bIsRWOverrideAsset = OverrideSlots.ContainsByPredicate([&](const ShaderOverrideSlot& Slot) {
					return Slot.bIsResource && IsRWResourceType(Slot.Type) && MakeOverrideKeyLabel(Slot.Key) == DisplayName;
				});
				if (bIsRWOverrideAsset)
				{
					GApp->GetEditor()->RefreshProperty();
				}
			}
		}
		else if (IsDefaultRWTextureProperty(OverrideSlots, InProperty))
		{
			InvalidateRenderResources();
		}
		else if (IsDefaultBufferProperty(OverrideSlots, InProperty))
		{
			InvalidateRenderResources();
		}
		else if (IsSamplerResourceProperty(OverrideSlots, InProperty))
		{
			InvalidateRenderResources();
		}
		ReconcileOverridePins(this, OverrideSlots, OverridePins);
		static_cast<MeshPassNode*>(GetOuter())->RefreshNodeWidget();
	}

	void MeshRenderObject::AddOverride(const FString& BindingName, const FString& MemberName, const FString& Type, BindingShaderStage Stage, bool bIsResource)
	{
		ShaderOverrideSlot Slot;
		Slot.Key = { BindingName, MemberName, Stage };
		Slot.Type = Type;
		Slot.bIsResource = bIsResource;

		if (MaterialAsset)
		{
			if (bIsResource)
			{
				if (const auto* ResourceDefault = MaterialAsset->BindingResourceDefaults.FindByPredicate([&](const MaterialBindingResourceDefault& Default) {
					return ShaderOverrideKey{ Default.BindingName, TEXT(""), Default.Stage } == Slot.Key;
				}))
				{
					CopyShaderResourceBindingState(Slot, *ResourceDefault);
					Slot.StructuredStride = ResourceDefault->StructuredStride;
				}
			}
			else
			{
				if (const auto* MemberDefault = MaterialAsset->BindingMemberDefaults.FindByPredicate([&](const MaterialBindingMemberDefault& Default) {
					return ShaderOverrideKey{ Default.BindingName, Default.MemberName, Default.Stage } == Slot.Key;
				}))
				{
					Slot.Bytes = MakeDefaultOverrideBytes(*MemberDefault);
				}
			}
		}

		auto Pin = CreateOverridePinForType(this, Type, bIsResource);
		if (Pin)
		{
			Pin->Direction = PinDirection::Input;
			Pin->ObjectName = FText::FromString(MakeOverrideSlotLabel(Slot));
			if (auto* BytesPinValue = DynamicCast<BytesPin>(Pin.Get()))
			{
				BytesPinValue->SetBytes(Slot.Bytes);
			}
			Slot.InputPin.SetReference(Pin.Get());
			OverridePins.Add(MoveTemp(Pin));
		}

		OverrideSlots.Add(MoveTemp(Slot));
		if (bIsResource)
		{
			InvalidateRenderResources();
		}
		if (auto* OuterMost = GetOuterMost()) OuterMost->MarkDirty();

		static_cast<MeshPassNode*>(GetOuter())->RefreshNodeWidget();
		GApp->GetEditor()->RefreshProperty();
	}

	void MeshRenderObject::RemoveOverride(int32 SlotIndex)
	{
		if (!OverrideSlots.IsValidIndex(SlotIndex))
		{
			return;
		}
		ShaderOverrideSlot Slot = OverrideSlots[SlotIndex];
		OverridePins.RemoveAll([&](const ObjectPtr<GraphPin>& P) {
			if (!P || (Slot.InputPin != P && Slot.OutputPin != P))
			{
				return false;
			}
			BreakOverridePinLink(P.Get());
			return true;
		});
		OverrideSlots.RemoveAt(SlotIndex);
		InvalidateRenderResources();
		if (auto* OuterMost = GetOuterMost()) OuterMost->MarkDirty();
	}

	void MeshRenderObject::BuildBindGroupFromMaterial(bool bRebuildLayouts, bool bRebuildUniformBuffers)
	{
		if (!MaterialAsset) return;

		//use override slot state info
		TArray<MaterialBindingResourceDefault> EffectiveResourceDefaults = MaterialAsset->BindingResourceDefaults;
		for (MaterialBindingResourceDefault& ResourceDefault : EffectiveResourceDefaults)
		{
			const ShaderOverrideKey Key{ ResourceDefault.BindingName, TEXT(""), ResourceDefault.Stage };
			ShaderOverrideSlot* MatchingSlot = FindOverrideSlot(OverrideSlots, Key);
			if (!MatchingSlot)
			{
				continue;
			}

			CopyShaderResourceBindingState(ResourceDefault, *MatchingSlot);
		}

		MaterialBindGroupBuildOptions Options;
		Options.bRebuildLayouts = bRebuildLayouts;
		Options.bRebuildUniformBuffers = bRebuildUniformBuffers;
		Options.PrinterBuffer = GetPrintBuffer()->GetResource();
		Options.DefaultResourceViewportSize = CurrentViewportSize;
		Options.BindingResourceDefaults = &EffectiveResourceDefaults;
		Options.TextureOverrideResolver = [this, &EffectiveResourceDefaults](const GpuShaderLayoutBinding& Binding) -> GpuTexture* {
			const ShaderOverrideKey Key{ Binding.Name, TEXT(""), Binding.Stage };
			GraphPin* OverridePin = FindOverrideInputPin(OverrideSlots, Key);
			MaterialBindingResourceDefault* ResourceDefault = EffectiveResourceDefaults.FindByPredicate([&](const MaterialBindingResourceDefault& Default) {
				return ShaderOverrideKey{ Default.BindingName, TEXT(""), Default.Stage } == Key;
			});

			// For RW resources, bind the internally-owned output texture;
			if (Binding.Type == BindingType::RWTexture || Binding.Type == BindingType::RWTexture3D)
			{
				ShaderOverrideSlot* MatchingSlot = FindOverrideSlot(OverrideSlots, Key);
				GpuTexture* SrcTex = ResolveOverrideTexture(Binding.Type, OverridePin, ResourceDefault, CurrentViewportSize);
				if (!MatchingSlot) return SrcTex;
				return EnsureRWOutputTexture(
					*MatchingSlot,
					Binding.Type,
					SrcTex,
					FString::Printf(TEXT("MRO_RWOut_%s"), *Binding.Name));
			}

			return ResolveOverrideTexture(Binding.Type, OverridePin, ResourceDefault, CurrentViewportSize);
		};

		BuildMaterialBindGroups(*MaterialAsset, BindGroupLayouts, BindGroups, UniformBuffers, Options);
	}

	void MeshRenderObject::AddRWInputBlitPasses(RenderGraph& RG)
	{
		if (!MaterialAsset)
		{
			return;
		}

		for (MaterialBindingResourceDefault& ResourceDefault : MaterialAsset->BindingResourceDefaults)
		{
			if (ResourceDefault.BindingType != BindingType::RWTexture) continue;

			const ShaderOverrideKey Key{ ResourceDefault.BindingName, TEXT(""), ResourceDefault.Stage };
			ShaderOverrideSlot* Slot = FindOverrideSlot(OverrideSlots, Key);
			if (!Slot) continue;

			GraphPin* OverridePin = FindOverrideInputPin(OverrideSlots, Key);
			ShaderResourceBindingState* ResourceState = static_cast<ShaderResourceBindingState*>(Slot);
			GpuTexture* SrcTex = ResolveOverrideTexture(ResourceDefault.BindingType, OverridePin, ResourceState, CurrentViewportSize);

			// Ensure cache is sized to the current input before the blit (the bind-group rebuild
			// inside the draw closure will pick up the same texture, since size/format match).
			GpuTexture* OutTex = EnsureRWOutputTexture(
				*Slot,
				ResourceDefault.BindingType,
				SrcTex,
				FString::Printf(TEXT("MRO_RWOut_%s"), *Key.BindingName));
			if (!OutTex || OutTex == SrcTex) continue;

			BlitPassInput BlitInput;
			BlitInput.InputView = SrcTex->GetDefaultView();
			BlitInput.InputTexSampler = GpuResourceHelper::GetSampler({});
			BlitInput.OutputView = OutTex->GetDefaultView();
			BlitInput.LoadAction = RenderTargetLoadAction::DontCare;
			AddBlitPass(RG, BlitInput);
		}
	}

	GpuTexture* MeshRenderObject::ResolveBindingTexture(const GpuShaderLayoutBinding& Binding)
	{
		if (!MaterialAsset)
		{
			return nullptr;
		}

		const ShaderOverrideKey Key{ Binding.Name, TEXT(""), Binding.Stage };
		GraphPin* OverridePin = FindOverrideInputPin(OverrideSlots, Key);
		ShaderResourceBindingState* ResourceState = FindOverrideSlot(OverrideSlots, Key);
		if (!ResourceState)
		{
			ResourceState = MaterialAsset->BindingResourceDefaults.FindByPredicate([&](const MaterialBindingResourceDefault& Default) {
				return Default.BindingName == Binding.Name && Default.Stage == Binding.Stage;
			});
		}

		return ResolveOverrideTexture(Binding.Type, OverridePin, ResourceState, CurrentViewportSize);
	}

	GpuBuffer* MeshRenderObject::ResolveBindingBuffer(const GpuShaderLayoutBinding& Binding)
	{
		if (!MaterialAsset)
		{
			return nullptr;
		}

		const ShaderOverrideKey Key{ Binding.Name, TEXT(""), Binding.Stage };
		ShaderResourceBindingState* ResourceState = FindOverrideSlot(OverrideSlots, Key);
		if (!ResourceState)
		{
			ResourceState = MaterialAsset->BindingResourceDefaults.FindByPredicate([&](const MaterialBindingResourceDefault& Default) {
				return Default.BindingName == Binding.Name && Default.Stage == Binding.Stage;
			});
		}
		if (!ResourceState)
		{
			return nullptr;
		}

		ResourceState->StructuredStride = Binding.StructuredStride;
		ResolveDefaultBuffer(ResourceState->BufferByteSize, Binding.StructuredStride, ResourceState->Format, Binding.Type, ResourceState->Buffer);
		return ResourceState->Buffer.GetReference();
	}

	void MeshRenderObject::AddRenderPassResourceAccesses(RGRenderPass& Pass)
	{
		if (!MaterialAsset)
		{
			return;
		}

		TArray<GpuShaderLayoutBinding> AllBindings;
		if (MaterialAsset->VertexShaderAsset)
		{
			if (GpuShader* Vs = MaterialAsset->VertexShaderAsset->GetCompiledShader(ShaderType::Vertex))
			{
				AllBindings.Append(Vs->GetLayout());
			}
		}
		if (MaterialAsset->PixelShaderAsset)
		{
			if (GpuShader* Ps = MaterialAsset->PixelShaderAsset->GetCompiledShader(ShaderType::Pixel))
			{
				AllBindings.Append(Ps->GetLayout());
			}
		}

		for (const GpuShaderLayoutBinding& Binding : AllBindings)
		{
			switch (Binding.Type)
			{
			case BindingType::StructuredBuffer:
			case BindingType::RawBuffer:
			case BindingType::TypedBuffer:
			{
				if (GpuBuffer* Buffer = ResolveBindingBuffer(Binding))
				{
					Pass.Read(Buffer);
				}
				break;
			}
			case BindingType::RWStructuredBuffer:
			case BindingType::RWRawBuffer:
			case BindingType::RWTypedBuffer:
			{
				if (GpuBuffer* Buffer = ResolveBindingBuffer(Binding))
				{
					Pass.Write(Buffer);
				}
				break;
			}
			case BindingType::Texture:
			case BindingType::TextureCube:
			case BindingType::Texture3D:
			case BindingType::CombinedTextureSampler:
			case BindingType::CombinedTextureCubeSampler:
			case BindingType::CombinedTexture3DSampler:
			{
				if (GpuTexture* Tex = ResolveBindingTexture(Binding))
				{
					Pass.Read(Tex);
				}
				break;
			}
			case BindingType::RWTexture:
			case BindingType::RWTexture3D:
			{
				const ShaderOverrideKey Key{ Binding.Name, TEXT(""), Binding.Stage };
				if (ShaderOverrideSlot* MatchingSlot = FindOverrideSlot(OverrideSlots, Key))
				{
					GpuTexture* SrcTex = ResolveBindingTexture(Binding);
					if (GpuTexture* OutTex = EnsureRWOutputTexture(
						*MatchingSlot,
						Binding.Type,
						SrcTex,
						FString::Printf(TEXT("MRO_RWOut_%s"), *Binding.Name)))
					{
						Pass.Write(OutTex);
					}
				}
				else if (GpuTexture* Tex = ResolveBindingTexture(Binding))
				{
					Pass.Write(Tex);
				}
				break;
			}
			default:
				break;
			}
		}
	}

	PrintBuffer* MeshRenderObject::GetPrintBuffer()
	{
		if (!PrinterBuffer)
		{
			PrinterBuffer = MakeUnique<PrintBuffer>();
		}
		return PrinterBuffer.Get();
	}

	bool MeshRenderObject::FlushPrintBufferLogs(const FString& LogPrefix)
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

	ShaderAsset* MeshRenderObject::GetShaderAsset(DebugItem Item) const
	{
		if (!MaterialAsset)
		{
			return nullptr;
		}
		switch (Item)
		{
		case DebugItem::Vertex:
			return MaterialAsset->VertexShaderAsset.Get();
		case DebugItem::Pixel:
			return MaterialAsset->PixelShaderAsset.Get();
		default:
			return nullptr;
		}
	}

	TArray<BindingBuilder> MeshRenderObject::BuildDebugBindingBuilders() const
	{
		TArray<BindingBuilder> Result;
		TArray<int32> GroupSlots;
		BindGroupLayouts.GetKeys(GroupSlots);
		GroupSlots.Sort();

		for (int32 GroupSlot : GroupSlots)
		{
			const TRefCountPtr<GpuBindGroupLayout>* LayoutPtr = BindGroupLayouts.Find(GroupSlot);
			const TRefCountPtr<GpuBindGroup>* BindGroupPtr = BindGroups.Find(GroupSlot);
			GpuBindGroupLayout* Layout = LayoutPtr ? LayoutPtr->GetReference() : nullptr;
			GpuBindGroup* BindGroup = BindGroupPtr ? BindGroupPtr->GetReference() : nullptr;
			if (!Layout || !BindGroup)
			{
				continue;
			}

			BindingBuilder Builder{ GpuBindGroupBuilder{ Layout }, GpuBindGroupLayoutBuilder{ GroupSlot } };
			for (const auto& [Slot, Binding] : Layout->GetDesc().Layouts)
			{
				Builder.LayoutBuilder.AddExistingBinding(Slot.SlotNum, Binding.Type, Binding.Stage);
			}
			for (const auto& [Slot, ResourceBindingEntry] : BindGroup->GetDesc().Resources)
			{
				Builder.BingGroupBuilder.SetExistingBinding(Slot.SlotNum, Slot.Type, ResourceBindingEntry.Resource.GetReference(), Slot.Stage);
			}
			Result.Add(MoveTemp(Builder));
		}
		return Result;
	}

	void MeshRenderObject::UpdateMaterialDrawState(const FMatrix44f& ModelMatrix, const FMatrix44f& ViewMat, const FMatrix44f& ProjMat, const Vector2f& ViewportSize, const Vector2f& MousePos, float Time, const Vector3f& CameraPos, const Vector3f& CameraDir)
	{
		const bool bHasResourceOverride = OverrideSlots.ContainsByPredicate([](const ShaderOverrideSlot& Slot) {
			return Slot.bIsResource;
		});
		if (bHasResourceOverride)
		{
			BuildBindGroupFromMaterial(false, false);
		}

		MaterialUniformBufferUpdateOptions UniformOptions = MakeMaterialUniformOptions(
			ModelMatrix, ViewMat, ProjMat, ViewportSize, MousePos, CameraPos, CameraDir, Time);
		UniformOptions.UniformOverrideBytesResolver = [this, &UniformOptions](const MaterialBindingMemberDefault& MemberDefault) -> const uint8* {
			const ShaderOverrideKey Key{ MemberDefault.BindingName, MemberDefault.MemberName, MemberDefault.Stage };
			ShaderOverrideSlot* Override = FindOverrideSlot(OverrideSlots, Key);
			if (!Override)
			{
				return nullptr;
			}

			if (GraphPin* OverridePin = FindOverrideInputPin(OverrideSlots, Key))
			{
				if (auto* BytesPinValue = DynamicCast<BytesPin>(OverridePin))
				{
					const TArray<uint8>& PinBytes = BytesPinValue->GetBytes();
					if (OverridePin->SourcePin.IsValid())
					{
						return GetCompleteOverrideBytes(PinBytes, MemberDefault.Type);
					}
					if (const uint8* PinData = GetCompleteOverrideBytes(PinBytes, MemberDefault.Type))
					{
						return PinData;
					}
				}
			}

			if (Override->ValueSource == MaterialBindingValueSource::BuiltIn)
			{
				if (IsShaderMatrix4x4Type(Override->Type))
				{
					FMatrix44f* Out = GetOverrideBytesAs<FMatrix44f>(*Override);
					switch (static_cast<BuiltInMatrix4x4Value>(Override->MatrixBuiltInRaw))
					{
					case BuiltInMatrix4x4Value::Model:    *Out = UniformOptions.ModelMatrix;    break;
					case BuiltInMatrix4x4Value::View:     *Out = UniformOptions.ViewMatrix;     break;
					case BuiltInMatrix4x4Value::Proj:     *Out = UniformOptions.ProjMatrix;     break;
					case BuiltInMatrix4x4Value::ViewProj: *Out = UniformOptions.ViewProjMatrix; break;
					case BuiltInMatrix4x4Value::MVP:      *Out = UniformOptions.MVPMatrix;      break;
					}
					return reinterpret_cast<const uint8*>(Out);
				}
				if (IsShaderFloatType(Override->Type))
				{
					float* Out = GetOverrideBytesAs<float>(*Override);
					switch (static_cast<BuiltInFloatValue>(Override->FloatBuiltInRaw))
					{
					case BuiltInFloatValue::Time: *Out = UniformOptions.Time; break;
					}
					return reinterpret_cast<const uint8*>(Out);
				}
				if (IsShaderVector2Type(Override->Type))
				{
					Vector2f* Out = GetOverrideBytesAs<Vector2f>(*Override);
					switch (static_cast<BuiltInVector2Value>(Override->Vector2BuiltInRaw))
					{
					case BuiltInVector2Value::ViewportSize: *Out = UniformOptions.ViewportSize; break;
					case BuiltInVector2Value::MousePos:     *Out = UniformOptions.MousePos;     break;
					}
					return reinterpret_cast<const uint8*>(Out);
				}
				if (IsShaderVector3Type(Override->Type))
				{
					Vector3f* Out = GetOverrideBytesAs<Vector3f>(*Override);
					switch (static_cast<BuiltInVector3Value>(Override->Vector3BuiltInRaw))
					{
					case BuiltInVector3Value::CameraPos: *Out = UniformOptions.CameraPos; break;
					case BuiltInVector3Value::CameraDir: *Out = UniformOptions.CameraDir; break;
					}
					return reinterpret_cast<const uint8*>(Out);
				}
			}

			return GetCompleteOverrideBytes(Override->Bytes, MemberDefault.Type);
		};
		UpdateMaterialUniformBuffers(*MaterialAsset, UniformBuffers, UniformOptions);
	}

	TRefCountPtr<GpuTexture> MeshRenderObject::BuildCoverageMask()
	{
		const MeshPassNode* OwnerNode = static_cast<MeshPassNode*>(GetOuter());
		if (!MeshSceneObjectRef.IsValid())
		{
			return nullptr;
		}

		if (!MaterialAsset || !Pipeline.IsValid())
		{
			return nullptr;
		}
		if (!MaterialAsset->VertexShaderAsset)
		{
			return nullptr;
		}
		GpuShader* MaterialVs = MaterialAsset->VertexShaderAsset->GetCompiledShader(ShaderType::Vertex);
		if (!MaterialVs)
		{
			return nullptr;
		}

		TRefCountPtr<GpuShader> MaskPs = GetMaskPs();

		MeshSceneObject* MSO = MeshSceneObjectRef.Get();

		GpuTexture* ReferenceTexture = nullptr;
		for (const TRefCountPtr<GpuTexture>& ColorRT : OwnerNode->GetOutputColorRTs())
		{
			if (ColorRT)
			{
				ReferenceTexture = ColorRT.GetReference();
				break;
			}
		}
		if (!ReferenceTexture && OwnerNode->GetOutputDepthRT())
		{
			ReferenceTexture = OwnerNode->GetOutputDepthRT().GetReference();
		}
		if (!ReferenceTexture)
		{
			return nullptr;
		}

		TRefCountPtr<GpuTexture> MaskTex = GGpuRhi->CreateTexture({
			.Width = ReferenceTexture->GetWidth(),
			.Height = ReferenceTexture->GetHeight(),
			.Format = GpuFormat::R8_UNORM,
			.Usage = GpuTextureUsage::RenderTarget | GpuTextureUsage::ShaderResource,
			.ClearValues = Vector4f(0, 0, 0, 0),
		}, GpuResourceState::RenderTargetWrite);

		const MeshPassDebugFrameState& FrameState = OwnerNode->GetDebugFrameState();
		const TOptional<Camera>& Cam = FrameState.Camera;
		const FMatrix44f ViewMat = Cam.IsSet() ? Cam.GetValue().GetViewMatrix() : FMatrix44f::Identity;
		const FMatrix44f ProjMat = Cam.IsSet() ? Cam.GetValue().GetProjectionMatrix() : FMatrix44f::Identity;
		const FMatrix44f WorldMat = MeshSceneObjectRef->GetWorldMatrix();
		const Vector2f ViewportSize((float)ReferenceTexture->GetWidth(), (float)ReferenceTexture->GetHeight());
		const Vector2f MousePos = FrameState.MousePos;
		const Vector3f CameraPos = Cam.IsSet() ? Cam.GetValue().Position : Vector3f(0, 0, 0);
		const Vector3f CameraDir = Cam.IsSet() ? GetCameraForwardDir(Cam.GetValue()) : Vector3f(0, 0, 0);
		const float Time = TSingleton<ShProjectManager>::Get().GetProject()->TimelineCurTime;
		UpdateMaterialDrawState(WorldMat, ViewMat, ProjMat, ViewportSize, MousePos, Time, CameraPos, CameraDir);

		TArray<GpuBindGroupLayout*> MaskLayoutArray;
		for (const auto& [_, L] : BindGroupLayouts) MaskLayoutArray.Add(L.GetReference());
		TArray<GpuBindGroup*> MaskBGArray;
		for (const auto& [_, BG] : BindGroups) MaskBGArray.Add(BG.GetReference());

		TArray<GpuVertexLayoutDesc> MaskVertexLayouts;
		if (TOptional<GpuVertexLayoutDesc> VertexLayout = BuildMaterialMeshVertexLayout(*MaterialAsset))
		{
			MaskVertexLayouts.Add(MoveTemp(*VertexLayout));
		}

		GpuRenderPipelineStateDesc MaskPipelineDesc{
			.Vs = MaterialVs,
			.Ps = MaskPs.GetReference(),
			.Targets = {{ .TargetFormat = MaskTex->GetFormat() }},
			.BindGroupLayouts = MaskLayoutArray,
			.VertexLayout = MoveTemp(MaskVertexLayouts),
			.RasterizerState = { MaterialAsset->FillMode, MaterialAsset->CullMode },
			.Primitive = MaterialAsset->Primitive,
		};

		TRefCountPtr<GpuRenderPipelineState> MaskPipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(MaskPipelineDesc);

		const uint32 InstanceCount = MSO->InstanceCount;
		TFunction<void(GpuRenderPassRecorder*)> DrawFunction;
		if (Model* ModelAsset = MSO->ModelAsset.Get())
		{
			TArray<MeshBuffers> GpuMeshes = ModelAsset->GetGpuMeshes();
			DrawFunction = [MaskPipeline, MaskBGArray, GpuMeshes, InstanceCount](GpuRenderPassRecorder* PassRecorder) {
				PassRecorder->SetBindGroups(MaskBGArray);
				PassRecorder->SetRenderPipelineState(MaskPipeline);
				for (const MeshBuffers& MB : GpuMeshes)
				{
					PassRecorder->SetVertexBuffer(0, MB.VertexBuffer);
					PassRecorder->SetIndexBuffer(MB.IndexBuffer);
					PassRecorder->DrawIndexed(0, MB.IndexCount, 0, 0, InstanceCount);
				}
			};
		}
		else
		{
			DrawFunction = [MaskPipeline, MaskBGArray, VertexCount = MSO->VertexCount, InstanceCount](GpuRenderPassRecorder* PassRecorder) {
				PassRecorder->SetBindGroups(MaskBGArray);
				PassRecorder->SetRenderPipelineState(MaskPipeline);
				PassRecorder->DrawPrimitive(0, VertexCount, 0, InstanceCount);
			};
		}

		GpuRenderPassDesc PassDesc;
		PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
			MaskTex->GetDefaultView(),
			RenderTargetLoadAction::Clear,
			RenderTargetStoreAction::Store
		});

		RenderGraph RG;
		RG.AddRenderPass(TEXT("MeshRenderObjectDebugMask"), MoveTemp(PassDesc), MoveTemp(DrawFunction));
		RG.Execute();
		return MaskTex;
	}

	DebugTargetInfo MeshRenderObject::OnStartDebugging(DebugItem Item)
	{
		ShaderAsset* MainShader = GetShaderAsset(Item);
		AssetOp::OpenAsset(MainShader);
		auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (SShaderEditorBox* ShaderEditor = ShEditor->GetShaderEditor(MainShader))
		{
			ShaderEditor->Compile();
		}
		TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop = true;
		TSingleton<ShProjectManager>::Get().GetProject()->bScenePreview = false;
		ShEditor->ForceRender();
		bDebugging = true;

		MeshPassNode* OwnerNode = static_cast<MeshPassNode*>(GetOuter());
		if (Item == DebugItem::Pixel)
		{
			return OwnerNode->MakeDebugTargetInfo(this);
		}
		return {};
	}

	InvocationState MeshRenderObject::GetInvocationState(DebugItem Item)
	{
		MeshPassNode* OwnerNode = static_cast<MeshPassNode*>(GetOuter());
		if (Item == DebugItem::Pixel)
		{
			MeshSceneObject* MSO = MeshSceneObjectRef.Get();
			const uint32 InstanceCount = MSO->InstanceCount;
			TFunction<void(GpuRenderPassRecorder*)> DrawFunction;
			if (Model* ModelAsset = MSO->ModelAsset.Get())
			{
				TArray<MeshBuffers> Meshes = ModelAsset->GetGpuMeshes();
				DrawFunction = [Meshes, InstanceCount](GpuRenderPassRecorder* Recorder) {
					for (const MeshBuffers& MeshBuffer : Meshes)
					{
						Recorder->SetVertexBuffer(0, MeshBuffer.VertexBuffer);
						Recorder->SetIndexBuffer(MeshBuffer.IndexBuffer);
						Recorder->DrawIndexed(0, MeshBuffer.IndexCount, 0, 0, InstanceCount);
					}
				};
			}
			else
			{
				DrawFunction = [VertexCount = MSO->VertexCount, InstanceCount](GpuRenderPassRecorder* Recorder) {
					Recorder->DrawPrimitive(0, VertexCount, 0, InstanceCount);
				};
			}
			return PixelState{
				.ViewPortDesc = OwnerNode->GetOutputViewPortDesc(),
				.Builders = BuildDebugBindingBuilders(),
				.PipelineDesc = PipelineDesc,
				.DrawFunction = MoveTemp(DrawFunction)
			};
		}
		AUX::Unreachable();
	}

	void MeshRenderObject::OnFinalizePixel(const Vector2u& PixelCoord)
	{
		auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->DebugPixel(PixelCoord, GetInvocationState(DebugItem::Pixel));
	}

	bool MeshRenderObject::UsesTextureAsShaderInput(GpuTexture* Texture, FString& OutBindingName) const
	{
		if (!MaterialAsset)
		{
			return false;
		}

		for (const MaterialBindingResourceDefault& ResourceDefault : MaterialAsset->BindingResourceDefaults)
		{
			if (!IsPinnableResourceBinding(ResourceDefault.BindingType))
			{
				continue;
			}

			GpuTexture* ShaderInputTexture = nullptr;
			if (GraphPin* OverridePin = FindOverrideInputPin(OverrideSlots, ShaderOverrideKey{ ResourceDefault.BindingName, TEXT(""), ResourceDefault.Stage }))
			{
				ShaderInputTexture = GetConnectedOverrideTexture(OverridePin);
			}

			if (ShaderInputTexture && ShaderInputTexture == Texture)
			{
				OutBindingName = ResourceDefault.BindingName;
				return true;
			}
		}

		return false;
	}

	bool MeshRenderObject::BuildPipeline(const TArray<GpuFormat>& ColorFormats, TOptional<GpuFormat> DepthFormat, uint32 SampleCount)
	{
		const MeshPassNode* OwnerNode = static_cast<MeshPassNode*>(GetOuter());
		const FString NodeName = OwnerNode->ObjectName.ToString();
		const FString RenderObjectName = ObjectName.ToString();
		const FString MaterialName = MaterialAsset
			? (MaterialAsset->GetPath().IsEmpty() ? MaterialAsset->ObjectName.ToString() : MaterialAsset->GetPath())
			: TEXT("");

		if (!MaterialAsset)
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" MeshRenderObject:\"%s\" cannot build pipeline because no material is assigned."), *NodeName, *RenderObjectName);
			return false;
		}

		if (BindGroupLayouts.IsEmpty())
		{
			BuildBindGroupFromMaterial();
		}

		MaterialPipelineBuildOptions Options;
		Options.ColorFormats = ColorFormats;
		Options.DepthFormat = DepthFormat;
		Options.SampleCount = SampleCount;
		for (auto& [_, L] : BindGroupLayouts) Options.BindGroupLayouts.Add(L.GetReference());

		MaterialPipelineBuildResult Result;
		if (!BuildMaterialPipeline(*MaterialAsset, Options, Result))
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" MeshRenderObject:\"%s\" material \"%s\" failed to build pipeline: %s"),
				*NodeName, *RenderObjectName, *MaterialName, *Result.Error.ToString());
			return false;
		}

		PipelineDesc = MoveTemp(Result.Desc);
		Pipeline = MoveTemp(Result.Pipeline);
		return Pipeline.IsValid();
	}

	bool MeshRenderObject::EnsureRenderResources(const TArray<GpuFormat>& ColorFormats, TOptional<GpuFormat> DepthFormat, uint32 SampleCount)
	{
		BindMaterialDelegates();
		return BuildPipeline(ColorFormats, DepthFormat, SampleCount);
	}

	void MeshRenderObject::Draw(GpuRenderPassRecorder* Recorder, const Camera* InCamera, const FMatrix44f& ModelMatrix, const Vector2f& ViewportSize, const Vector2f& MousePos, float Time)
	{
		if (!MeshSceneObjectRef.IsValid())
		{
			return;
		}
		MeshSceneObject* MSO = MeshSceneObjectRef.Get();

		const FMatrix44f ViewMat = InCamera ? InCamera->GetViewMatrix() : FMatrix44f::Identity;
		const FMatrix44f ProjMat = InCamera ? InCamera->GetProjectionMatrix() : FMatrix44f::Identity;

		if (!MaterialAsset || !Pipeline.IsValid())
		{
			return;
		}

		const Vector3f CameraPos = InCamera ? InCamera->Position : Vector3f(0, 0, 0);
		const Vector3f CameraDir = InCamera ? GetCameraForwardDir(*InCamera) : Vector3f(0, 0, 0);
		UpdateMaterialDrawState(ModelMatrix, ViewMat, ProjMat, ViewportSize, MousePos, Time, CameraPos, CameraDir);

		TArray<GpuBindGroup*> BGArray;
		for (auto& [_, BG] : BindGroups) BGArray.Add(BG.GetReference());

		Recorder->SetRenderPipelineState(Pipeline);
		Recorder->SetBindGroups(BGArray);

		if (Model* ModelAsset = MSO->ModelAsset.Get())
		{
			const TArray<MeshBuffers>& GpuMeshes = ModelAsset->GetGpuMeshes();
			for (const MeshBuffers& MB : GpuMeshes)
			{
				Recorder->SetVertexBuffer(0, MB.VertexBuffer);
				Recorder->SetIndexBuffer(MB.IndexBuffer);
				Recorder->DrawIndexed(0, MB.IndexCount, 0, 0, MSO->InstanceCount);
			}
		}
		else
		{
			Recorder->DrawPrimitive(0, MSO->VertexCount, 0, MSO->InstanceCount);
		}
	}

	bool MeshRenderObject::BuildAssertHighlightPs()
	{
		if (!MaterialAsset || !MaterialAsset->PixelShaderAsset)
		{
			return false;
		}

		Shader* PsAsset = MaterialAsset->PixelShaderAsset.Get();
		ShaderDesc Desc = PsAsset->GetShaderDesc(PsAsset->EditorContent, ShaderType::Pixel);

		TRefCountPtr<GpuShader> TempPs = GGpuRhi->CreateShaderFromSource(Desc.SourceDesc);
		TempPs->CompilerFlag |= GpuShaderCompilerFlag::GenSpvForDebugging;
		TArray<FString> SpvExtraArgs = Desc.ExtraArgs;
		SpvExtraArgs.Add(TEXT("-D"));
		SpvExtraArgs.Add(TEXT("GPrivate_ENABLE_PRINT=0"));
		FString ErrorInfo, WarnInfo;
		if (!GGpuRhi->CompileShader(TempPs, ErrorInfo, WarnInfo, SpvExtraArgs))
		{
			return false;
		}

		SpirvParser Parser;
		Parser.Parse(TempPs->SpvCode);
		SpvMetaContext HlContext;
		SpvMetaVisitor MetaVisitor{ HlContext };
		Parser.Accept(&MetaVisitor);

		TArray<SpvBinding> RuntimeBindings;
		if (GpuShader* RuntimePs = PsAsset->GetCompiledShader(ShaderType::Pixel))
		{
			for (const GpuShaderLayoutBinding& Binding : RuntimePs->GetLayout())
			{
				RuntimeBindings.Add({
					.Name = Binding.Name,
					.DescriptorSet = Binding.Group,
					.Binding = Binding.Slot,
					.Type = Binding.Type,
				});
			}
		}

		SpvAssertHighlightVisitor HlVisitor{ HlContext, ShaderType::Pixel, MoveTemp(RuntimeBindings) };
		Parser.Accept(&HlVisitor);

		TArray<uint32> PatchedSpv = HlVisitor.GetPatcher().GetSpv();
		FString PatchedAsm = HlVisitor.GetPatcher().GetAsm();
		const FString DumpName = PsAsset->GetShaderName() + TEXT("_AssertHighlight.spvasm");
		FFileHelper::SaveStringToFile(PatchedAsm, *(PathHelper::SavedShaderDir() / PsAsset->GetShaderName() / DumpName));

		GpuShaderSourceDesc PatchedDesc = Desc.SourceDesc;
		PatchedDesc.Source = PatchedAsm;
		TRefCountPtr<GpuShader> NewAssertHighlightPs = GGpuRhi->CreateShaderFromSource(PatchedDesc);
		NewAssertHighlightPs->SpvCode = MoveTemp(PatchedSpv);
		NewAssertHighlightPs->CompilerFlag |= GpuShaderCompilerFlag::CompileFromSpvCode;
		if (!GGpuRhi->CompileShader(NewAssertHighlightPs, ErrorInfo, WarnInfo, Desc.ExtraArgs))
		{
			SH_LOG(LogGraph, Error, TEXT("AssertHighlight: patched PS compile failed: %s"), *ErrorInfo);
			return false;
		}

		AssertHighlightPs = MoveTemp(NewAssertHighlightPs);
		return true;
	}

	void MeshRenderObject::AddAssertHighlightPass(RenderGraph& RG, const TArray<TRefCountPtr<GpuTexture>>& ColorRTs, TRefCountPtr<GpuTexture> DepthRT, const Vector2f& ViewportSize, const Camera* Cam)
	{
		if (!bHadAssertError) return;
		if (!MaterialAsset || !MeshSceneObjectRef.IsValid()) return;
		if (!Pipeline.IsValid()) return;
		if (!BuildAssertHighlightPs()) return;

		MeshSceneObject* MSO = MeshSceneObjectRef.Get();

		const FMatrix44f WorldMat = MSO->GetWorldMatrix();
		const FMatrix44f ViewMat = Cam ? Cam->GetViewMatrix() : FMatrix44f::Identity;
		const FMatrix44f ProjMat = Cam ? Cam->GetProjectionMatrix() : FMatrix44f::Identity;
		const Vector3f CameraPos = Cam ? Cam->Position : Vector3f(0, 0, 0);
		const Vector3f CameraDir = Cam ? GetCameraForwardDir(*Cam) : Vector3f(0, 0, 0);
		const float Time = TSingleton<ShProjectManager>::Get().GetProject()->TimelineCurTime;

		UpdateMaterialDrawState(WorldMat, ViewMat, ProjMat, ViewportSize, Vector2f(0, 0), Time, CameraPos, CameraDir);

		GpuRenderPipelineStateDesc HlDesc = PipelineDesc;
		HlDesc.Ps = AssertHighlightPs.GetReference();
		if (HlDesc.DepthStencilState.IsSet())
		{
			HlDesc.DepthStencilState->DepthWriteEnable = false;
			HlDesc.DepthStencilState->DepthCompare = CompareMode::LessEqual;
		}

		TRefCountPtr<GpuRenderPipelineState> HlPipeline;
		try
		{
			HlPipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(HlDesc);
		}
		catch (const std::runtime_error& e)
		{
			SH_LOG(LogShader, Error, TEXT("AssertHighlight: PSO create failed: %s"), ANSI_TO_TCHAR(e.what()));
			return;
		}

		GpuRenderPassDesc PassDesc;
		for (const TRefCountPtr<GpuTexture>& Rt : ColorRTs)
		{
			if (!Rt) continue;
			PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
				Rt->GetDefaultView(),
				RenderTargetLoadAction::Load,
				RenderTargetStoreAction::Store
			});
		}
		if (DepthRT.IsValid())
		{
			PassDesc.DepthStencilTarget = GpuDepthStencilTargetInfo{
				DepthRT->GetDefaultView(),
				RenderTargetLoadAction::Load,
				RenderTargetStoreAction::Store,
				1.0f
			};
		}

		TArray<GpuBindGroup*> BGArray;
		for (auto& [_, BG] : BindGroups) BGArray.Add(BG.GetReference());

		ObserverObjectPtr<MeshSceneObject> MsoObs = MeshSceneObjectRef;
		auto& Pass = RG.AddRenderPass(TEXT("MeshRenderObjectAssertHighlight"), MoveTemp(PassDesc),
			[HlPipeline, BGArray, MsoObs](GpuRenderPassRecorder* Recorder) {
				if (!MsoObs.IsValid()) return;
				MeshSceneObject* InMso = MsoObs.Get();
				Recorder->SetRenderPipelineState(HlPipeline);
				Recorder->SetBindGroups(BGArray);
				if (Model* MdlAsset = InMso->ModelAsset.Get())
				{
					const TArray<MeshBuffers>& GpuMeshes = MdlAsset->GetGpuMeshes();
					for (const MeshBuffers& MB : GpuMeshes)
					{
						Recorder->SetVertexBuffer(0, MB.VertexBuffer);
						Recorder->SetIndexBuffer(MB.IndexBuffer);
						Recorder->DrawIndexed(0, MB.IndexCount, 0, 0, InMso->InstanceCount);
					}
				}
				else
				{
					Recorder->DrawPrimitive(0, InMso->VertexCount, 0, InMso->InstanceCount);
				}
			}
		);

		AddRenderPassResourceAccesses(Pass);
	}
}
