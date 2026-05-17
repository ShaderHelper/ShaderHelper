#include "CommonHeader.h"
#include "MeshRenderObject.h"
#include "ShaderOverrideHelper.h"
#include "Nodes/MeshPassNode.h"
#include "AssetObject/Pins/Pins.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "Editor/ShaderHelperEditor.h"
#include "GpuApi/GpuRhi.h"
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

using namespace FW;

namespace SH
{
	namespace
	{
		FString MakeRWOutputKey(const FString& BindingName, BindingShaderStage Stage)
		{
			return FString::Printf(TEXT("%s|%u"), *BindingName, (uint32)Stage);
		}

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
				.Source     = TEXT("float4 Main() : SV_Target0\n"
					"{\n"
					"    return 1.0.xxxx;\n"
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
		RWOutputTextures.Empty();
		bDrawMaterialError = false;
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
								static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
							}))
						);
					}

					for (const auto& ResourceDefault : Mat->BindingResourceDefaults)
					{
						if (!IsPinnableResourceBinding(ResourceDefault.BindingType))
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
								static_cast<MeshPassNode*>(GetOuter())->RefreshNodeWidget();
								static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
							}))
						);
					}
				}

				return MenuBuilder.MakeWidget();
			})
		);

		for (int32 Index = 0; Index < OverrideSlots.Num(); ++Index)
		{
			ShaderOverrideSlot& Slot = OverrideSlots[Index];
			const FText LabelText = FText::FromString(MakeOverrideSlotLabel(Slot));
			GraphPin* OverridePin = FindOverrideInputPin(OverrideSlots, Slot.Key);
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
				auto AssetItem = MakeShared<PropertyAssetItem>(this, LabelText, GetTextureMetaType(Slot.Type), &Slot.TextureAsset);
				if (IsRWResourceType(Slot.Type) && !Slot.TextureAsset)
				{
					AppendDefaultRWSizeChildren(this, *AssetItem, Slot);
				}
				Entry = AssetItem;
			}
			else
			{
				Entry = MakeBytesPropertyItem(this, LabelText, Slot);
			}
			int32 EntryIndex = Index;
			Entry->SetOnDelete([this, EntryIndex] {
				RemoveOverride(EntryIndex);
				static_cast<MeshPassNode*>(GetOuter())->RefreshNodeWidget();
				static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
			});
			OverrideCat->AddChild(Entry.ToSharedRef());
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
					static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
				}
			}
		}
		else if (IsDefaultRWTextureProperty(OverrideSlots, InProperty))
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
					Slot.TextureAsset = ResourceDefault->TextureAsset;
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

		MaterialBindGroupBuildOptions Options;
		Options.bRebuildLayouts = bRebuildLayouts;
		Options.bRebuildUniformBuffers = bRebuildUniformBuffers;
		Options.PrinterBuffer = GetPrintBuffer()->GetResource();
		Options.TextureOverrideResolver = [this](const GpuShaderLayoutBinding& Binding) -> GpuTexture* {
				const ShaderOverrideKey Key{ Binding.Name, TEXT(""), Binding.Stage };
				GraphPin* OverridePin = FindOverrideInputPin(OverrideSlots, Key);
				ShaderOverrideSlot* MatchingSlot = FindOverrideSlot(OverrideSlots, Key);

				// For RW resources, bind the internally-owned output texture;
				if (Binding.Type == BindingType::RWTexture || Binding.Type == BindingType::RWTexture3D)
				{
					GpuTexture* SrcTex = ResolveOverrideTexture(Binding.Type, OverridePin, MatchingSlot, CurrentViewportSize);
					return EnsureRWOutputTexture(MakeRWOutputKey(Binding.Name, Binding.Stage), Binding.Type, SrcTex);
				}

				// nullptr signals to BuildMaterialBindGroups to fall back to ResourceDefault's texture asset
				// rather than the default override texture.
				if (!OverridePin && !MatchingSlot) return nullptr;
				return ResolveOverrideTexture(Binding.Type, OverridePin, MatchingSlot, CurrentViewportSize);
			};

		BuildMaterialBindGroups(*MaterialAsset, BindGroupLayouts, BindGroups, UniformBuffers, Options);
	}

	GpuTexture* MeshRenderObject::EnsureRWOutputTexture(const FString& Key, BindingType BindingTypeValue, GpuTexture* SrcTex)
	{
		const uint32 Width = SrcTex->GetWidth();
		const uint32 Height = SrcTex->GetHeight();
		const uint32 Depth = SrcTex->GetDepth();
		const GpuFormat Format = SrcTex->GetFormat();
		const bool bIs3D = BindingTypeValue == BindingType::RWTexture3D;

		TRefCountPtr<GpuTexture>& Slot = RWOutputTextures.FindOrAdd(Key);
		if (Slot.IsValid()
			&& Slot->GetWidth() == Width
			&& Slot->GetHeight() == Height
			&& Slot->GetFormat() == Format
			&& (!bIs3D || Slot->GetDepth() == Depth))
		{
			return Slot.GetReference();
		}

		GpuTextureDesc Desc{
			.Width = Width,
			.Height = Height,
			.Format = Format,
			.Usage = GpuTextureUsage::ShaderResource | GpuTextureUsage::UnorderedAccess | GpuTextureUsage::RenderTarget,
		};
		if (bIs3D)
		{
			Desc.Depth = Depth;
			Desc.Dimension = GpuTextureDimension::Tex3D;
			// 3D textures can't be Blit destinations (no render pass support); drop RenderTarget usage.
			Desc.Usage = GpuTextureUsage::ShaderResource | GpuTextureUsage::UnorderedAccess;
		}
		Slot = GGpuRhi->CreateTexture(Desc, GpuResourceState::UnorderedAccess);
		GGpuRhi->SetResourceName(TCHAR_TO_ANSI(*FString::Printf(TEXT("MRO_RWOut_%s"), *Key)), Slot);
		return Slot.GetReference();
	}

	void MeshRenderObject::AddRWInputBlitPasses(RenderGraph& RG)
	{
		if (!MaterialAsset || bDrawMaterialError)
		{
			return;
		}

		for (ShaderOverrideSlot& Slot : OverrideSlots)
		{
			if (!Slot.bIsResource) continue;

			const MaterialBindingResourceDefault* ResourceDefault = MaterialAsset->BindingResourceDefaults.FindByPredicate(
				[&](const MaterialBindingResourceDefault& Default) {
					return ShaderOverrideKey{ Default.BindingName, TEXT(""), Default.Stage } == Slot.Key;
				});
			if (!ResourceDefault) continue;
			if (ResourceDefault->BindingType != BindingType::RWTexture) continue;

			GraphPin* OverridePin = FindOverrideInputPin(OverrideSlots, Slot.Key);
			GpuTexture* SrcTex = ResolveOverrideTexture(ResourceDefault->BindingType, OverridePin, &Slot, CurrentViewportSize);

			// Ensure cache is sized to the current input before the blit (the bind-group rebuild
			// inside the draw closure will pick up the same texture, since size/format match).
			const FString OutputKey = MakeRWOutputKey(Slot.Key.BindingName, Slot.Key.Stage);
			GpuTexture* OutTex = EnsureRWOutputTexture(OutputKey, ResourceDefault->BindingType, SrcTex);
			if (!OutTex || OutTex == SrcTex) continue;

			BlitPassInput BlitInput;
			BlitInput.InputView = SrcTex->GetDefaultView();
			BlitInput.InputTexSampler = GpuResourceHelper::GetSampler({});
			BlitInput.OutputView = OutTex->GetDefaultView();
			BlitInput.LoadAction = RenderTargetLoadAction::DontCare;
			AddBlitPass(RG, BlitInput);
		}
	}

	void MeshRenderObject::PublishRWOutputsToPins()
	{
		for (const ShaderOverrideSlot& Slot : OverrideSlots)
		{
			if (!Slot.bIsResource) continue;

			const TRefCountPtr<GpuTexture>* OutTexPtr = RWOutputTextures.Find(MakeRWOutputKey(Slot.Key.BindingName, Slot.Key.Stage));
			if (!OutTexPtr || !OutTexPtr->IsValid()) continue;
			GpuTexture* OutTex = OutTexPtr->GetReference();

			GraphPin* OutputPin = Slot.OutputPin.IsValid() ? Slot.OutputPin.Get() : nullptr;
			if (auto* TexPin = DynamicCast<GpuTexturePin>(OutputPin))
			{
				TexPin->SetValue(OutTex);
			}
			else if (auto* Tex3DPin = DynamicCast<GpuTexture3DPin>(OutputPin))
			{
				Tex3DPin->SetValue(OutTex);
			}
		}
	}

	void MeshRenderObject::CollectConnectedOverrideTextures(TSet<GpuTexture*>& OutTextures) const
	{
		if (!MaterialAsset || bDrawMaterialError)
		{
			return;
		}

		for (const ShaderOverrideSlot& Slot : OverrideSlots)
		{
			if (!Slot.bIsResource)
			{
				continue;
			}

			if (GraphPin* OverridePin = FindOverrideInputPin(OverrideSlots, Slot.Key))
			{
				if (GpuTexture* Texture = GetConnectedOverrideTexture(OverridePin))
				{
					OutTextures.Add(Texture);
				}
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
		UniformOptions.UniformOverrideBytesResolver = [this](const MaterialBindingMemberDefault& MemberDefault) -> const uint8* {
			const ShaderOverrideKey Key{ MemberDefault.BindingName, MemberDefault.MemberName, MemberDefault.Stage };
			const ShaderOverrideSlot* Override = FindOverrideSlot(OverrideSlots, Key);
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

		if (!MaterialAsset || bDrawMaterialError || !Pipeline.IsValid())
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
		TArray<MeshBuffers> GpuMeshes;
		uint32 VertexCount = 0;
		if (Model* ModelAsset = MSO->ModelAsset.Get())
		{
			GpuMeshes = ModelAsset->GetGpuMeshes();
		}
		else
		{
			VertexCount = MSO->VertexCount;
		}

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
			.Format = GpuFormat::B8G8R8A8_UNORM,
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

		GpuRenderPipelineStateDesc MaskPipelineDesc{
			.Vs = MaterialVs,
			.Ps = MaskPs.GetReference(),
			.Targets = {{ .TargetFormat = MaskTex->GetFormat() }},
			.BindGroupLayouts = MaskLayoutArray,
			.VertexLayout = { BuildMaterialMeshVertexLayout(*MaterialAsset) },
			.RasterizerState = { MaterialAsset->FillMode, MaterialAsset->CullMode },
			.Primitive = MaterialAsset->Primitive,
		};

		TRefCountPtr<GpuRenderPipelineState> MaskPipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(MaskPipelineDesc);

		RenderGraph RG;
		for (int32 MeshIndex = 0; MeshIndex < GpuMeshes.Num(); ++MeshIndex)
		{
			const MeshBuffers& Buffers = GpuMeshes[MeshIndex];
			GpuRenderPassDesc PassDesc;
			PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
				MaskTex->GetDefaultView(),
				MeshIndex == 0 ? RenderTargetLoadAction::Clear : RenderTargetLoadAction::Load,
				RenderTargetStoreAction::Store
			});

			RG.AddRenderPass(TEXT("MeshRenderObjectDebugMask"), MoveTemp(PassDesc),
				[MaskPipeline, MaskBGArray, VB = Buffers.VertexBuffer, IB = Buffers.IndexBuffer, IdxCount = Buffers.IndexCount]
				(GpuRenderPassRecorder* PassRecorder) {
					PassRecorder->SetBindGroups(MaskBGArray);
					PassRecorder->SetRenderPipelineState(MaskPipeline);
					PassRecorder->SetVertexBuffer(0, VB);
					PassRecorder->SetIndexBuffer(IB);
					PassRecorder->DrawIndexed(0, IdxCount);
				}
			);
		}
		if (VertexCount > 0)
		{
			GpuRenderPassDesc PassDesc;
			PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{
				MaskTex->GetDefaultView(),
				RenderTargetLoadAction::Clear,
				RenderTargetStoreAction::Store
			});

			RG.AddRenderPass(TEXT("MeshRenderObjectDebugMask"), MoveTemp(PassDesc),
				[MaskPipeline, MaskBGArray, VertexCount](GpuRenderPassRecorder* PassRecorder) {
					PassRecorder->SetBindGroups(MaskBGArray);
					PassRecorder->SetRenderPipelineState(MaskPipeline);
					PassRecorder->DrawPrimitive(0, VertexCount, 0, 1);
				}
			);
		}
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
			TArray<MeshBuffers> Meshes;
			uint32 VertexCount = 0;
			if (Model* ModelAsset = MSO->ModelAsset.Get())
			{
				Meshes = ModelAsset->GetGpuMeshes();
			}
			else
			{
				VertexCount = MSO->VertexCount;
			}
			return PixelState{
				.ViewPortDesc = OwnerNode->GetOutputViewPortDesc(),
				.Builders = BuildDebugBindingBuilders(),
				.PipelineDesc = PipelineDesc,
				.DrawFunction = [Meshes, VertexCount](GpuRenderPassRecorder* Recorder) {
					for (const MeshBuffers& MeshBuffer : Meshes)
					{
						Recorder->SetVertexBuffer(0, MeshBuffer.VertexBuffer);
						Recorder->SetIndexBuffer(MeshBuffer.IndexBuffer);
						Recorder->DrawIndexed(0, MeshBuffer.IndexCount);
					}
					if (VertexCount > 0)
					{
						Recorder->DrawPrimitive(0, VertexCount, 0, 1);
					}
				}
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
		if (!MaterialAsset || bDrawMaterialError)
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

	bool MeshRenderObject::BuildPipeline(const TArray<GpuFormat>& ColorFormats, GpuFormat DepthFormat, uint32 SampleCount)
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

	bool MeshRenderObject::EnsureRenderResources(const TArray<GpuFormat>& ColorFormats, GpuFormat DepthFormat, uint32 SampleCount)
	{
		BindMaterialDelegates();
		bDrawMaterialError = !BuildPipeline(ColorFormats, DepthFormat, SampleCount);
		if (bDrawMaterialError)
		{
			return EnsureMaterialErrorRenderResources(ErrorResources, ColorFormats, DepthFormat, SampleCount);
		}
		return true;
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
		const FMatrix44f MVPMat = ModelMatrix * (ViewMat * ProjMat);

		if (bDrawMaterialError)
		{
			if (Model* ErrorModel = MSO->ModelAsset.Get())
			{
				const TArray<MeshBuffers>& GpuMeshes = ErrorModel->GetGpuMeshes();
				for (const MeshBuffers& MeshBuffer : GpuMeshes)
				{
					DrawMaterialErrorMesh(Recorder, ErrorResources, MeshBuffer, MVPMat);
				}
			}
			return;
		}

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
				Recorder->DrawIndexed(0, MB.IndexCount);
			}
		}
		else if (MSO->VertexCount > 0)
		{
			Recorder->DrawPrimitive(0, MSO->VertexCount, 0, 1);
		}
	}
}
