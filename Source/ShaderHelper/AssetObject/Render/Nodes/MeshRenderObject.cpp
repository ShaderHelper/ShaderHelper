#include "CommonHeader.h"
#include "MeshRenderObject.h"
#include "MeshPassNode.h"
#include "App/App.h"
#include "AssetObject/Pins/Pins.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "Editor/ShaderHelperEditor.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuResourceHelper.h"
#include "Renderer/MaterialRenderResources.h"
#include "AssetObject/Texture2D.h"
#include "AssetObject/TextureCube.h"
#include "AssetObject/Texture3D.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"
#include "UI/Widgets/Property/PropertyData/PropertyAssetItem.h"
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
				return MakeBytesFromValue(FMatrix44f::Identity);
			}
			if (IsShaderFloatType(Default.Type))
			{
				return MakeBytesFromValue(Default.Values[0]);
			}
			if (IsShaderIntType(Default.Type) || IsShaderBoolType(Default.Type))
			{
				return MakeBytesFromValue(Default.IntValues[0]);
			}
			if (IsShaderUintType(Default.Type))
			{
				return MakeBytesFromValue(Default.UintValues[0]);
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
				return MakeBytesFromData(Default.IntValues, sizeof(int32) * 2);
			}
			if (IsShaderIntVector3Type(Default.Type) || IsShaderBoolVector3Type(Default.Type))
			{
				return MakeBytesFromData(Default.IntValues, sizeof(int32) * 3);
			}
			if (IsShaderIntVector4Type(Default.Type) || IsShaderBoolVector4Type(Default.Type))
			{
				return MakeBytesFromData(Default.IntValues, sizeof(int32) * 4);
			}
			if (IsShaderUintVector2Type(Default.Type))
			{
				return MakeBytesFromData(Default.UintValues, sizeof(uint32) * 2);
			}
			if (IsShaderUintVector3Type(Default.Type))
			{
				return MakeBytesFromData(Default.UintValues, sizeof(uint32) * 3);
			}
			if (IsShaderUintVector4Type(Default.Type))
			{
				return MakeBytesFromData(Default.UintValues, sizeof(uint32) * 4);
			}
			return {};
		}

		int32 GetOverrideByteSize(const FString& Type)
		{
			if (IsShaderMatrix4x4Type(Type)) return static_cast<int32>(sizeof(FMatrix44f));
			if (IsShaderFloatType(Type) || IsShaderIntType(Type) || IsShaderUintType(Type) || IsShaderBoolType(Type)) return static_cast<int32>(sizeof(int32));
			if (IsShaderVector2Type(Type) || IsShaderIntVector2Type(Type) || IsShaderUintVector2Type(Type) || IsShaderBoolVector2Type(Type)) return static_cast<int32>(sizeof(int32) * 2);
			if (IsShaderVector3Type(Type) || IsShaderIntVector3Type(Type) || IsShaderUintVector3Type(Type) || IsShaderBoolVector3Type(Type)) return static_cast<int32>(sizeof(int32) * 3);
			if (IsShaderVector4Type(Type) || IsShaderIntVector4Type(Type) || IsShaderUintVector4Type(Type) || IsShaderBoolVector4Type(Type)) return static_cast<int32>(sizeof(int32) * 4);
			return 0;
		}

		const uint8* GetCompleteOverrideBytes(const TArray<uint8>& Bytes, const FString& Type)
		{
			const int32 RequiredSize = GetOverrideByteSize(Type);
			return RequiredSize > 0 && Bytes.Num() >= RequiredSize ? Bytes.GetData() : nullptr;
		}

		void EnsureOverrideBytesStorage(MaterialOverrideSlot& Slot)
		{
			const int32 RequiredSize = GetOverrideByteSize(Slot.Type);
			if (RequiredSize <= 0 || Slot.Bytes.Num() >= RequiredSize)
			{
				return;
			}

			if (IsShaderMatrix4x4Type(Slot.Type))
			{
				Slot.Bytes = MakeBytesFromValue(FMatrix44f::Identity);
			}
			else
			{
				Slot.Bytes.SetNumZeroed(RequiredSize);
			}
		}

		template<typename ValueType>
		ValueType* GetOverrideBytesAs(MaterialOverrideSlot& Slot)
		{
			EnsureOverrideBytesStorage(Slot);
			return reinterpret_cast<ValueType*>(Slot.Bytes.GetData());
		}

		GpuTexture* GetTextureData(AssetObject* TextureAsset)
		{
			if (auto* Texture = DynamicCast<Texture2D>(TextureAsset)) return Texture->GetGpuData();
			if (auto* Texture = DynamicCast<TextureCube>(TextureAsset)) return Texture->GetGpuData();
			if (auto* Texture = DynamicCast<Texture3D>(TextureAsset)) return Texture->GetGpuData();
			return nullptr;
		}

		MetaType* GetTextureMetaType(const FString& Type)
		{
			if (Type.Contains(TEXT("Cube"))) return GetMetaType<TextureCube>();
			if (Type.Contains(TEXT("3D"))) return GetMetaType<Texture3D>();
			return GetMetaType<Texture2D>();
		}

		FString GetTextureOverrideType(BindingType BindingTypeValue)
		{
			if (BindingTypeValue == BindingType::TextureCube || BindingTypeValue == BindingType::CombinedTextureCubeSampler) return TEXT("TextureCube");
			if (BindingTypeValue == BindingType::Texture3D || BindingTypeValue == BindingType::CombinedTexture3DSampler) return TEXT("Texture3D");
			return TEXT("Texture2D");
		}

		bool IsTextureShaderInputBinding(BindingType BindingTypeValue)
		{
			switch (BindingTypeValue)
			{
			case BindingType::Texture:
			case BindingType::TextureCube:
			case BindingType::Texture3D:
			case BindingType::CombinedTextureSampler:
			case BindingType::CombinedTextureCubeSampler:
			case BindingType::CombinedTexture3DSampler:
				return true;
			default:
				return false;
			}
		}

		GpuTexture* GetConnectedOverrideTexture(GraphPin* OverridePin)
		{
			if (!OverridePin || !OverridePin->SourcePin.IsValid())
			{
				return nullptr;
			}

			if (auto* TexturePin = DynamicCast<GpuTexturePin>(OverridePin)) return TexturePin->GetValue();
			if (auto* CubemapPin = DynamicCast<GpuCubemapPin>(OverridePin)) return CubemapPin->GetValue();
			if (auto* Texture3DPin = DynamicCast<GpuTexture3DPin>(OverridePin)) return Texture3DPin->GetValue();
			return nullptr;
		}

		void BreakOverridePinLink(GraphPin* Pin)
		{
			if (!Pin || !Pin->SourcePin.IsValid())
			{
				return;
			}

			GraphPin* SourcePin = Pin->GetSourcePin();
			if (SourcePin)
			{
				if (GraphNode* SourceNode = SourcePin->GetOwnerNode())
				{
					SourceNode->OutPinToInPin.Remove(SourcePin, Pin);
				}
			}
			Pin->SourcePin.Reset();
			Pin->Refuse();
		}

		TSharedRef<PropertyItemBase> MakeBytesPropertyItem(MeshRenderObject* Owner, FText Label, MaterialOverrideSlot& Slot)
		{
			if (IsShaderMatrix4x4Type(Slot.Type))
			{
				return MakeShared<PropertyMatrix4x4fItem>(Owner, Label, GetOverrideBytesAs<FMatrix44f>(Slot));
			}
			if (IsShaderFloatType(Slot.Type)) return MakeShared<PropertyScalarItem<float>>(Owner, Label, GetOverrideBytesAs<float>(Slot));
			if (IsShaderIntType(Slot.Type)) return MakeShared<PropertyScalarItem<int32>>(Owner, Label, GetOverrideBytesAs<int32>(Slot));
			if (IsShaderUintType(Slot.Type))
			{
				auto Item = MakeShared<PropertyScalarItem<int32>>(Owner, Label, GetOverrideBytesAs<int32>(Slot));
				Item->SetMinValue(0);
				return Item;
			}
			if (IsShaderBoolType(Slot.Type))
			{
				auto Item = MakeShared<PropertyScalarItem<int32>>(Owner, Label, GetOverrideBytesAs<int32>(Slot));
				Item->SetMinValue(0);
				Item->SetMaxValue(1);
				return Item;
			}
			if (IsShaderVector2Type(Slot.Type)) return MakeShared<PropertyVector2fItem>(Owner, Label, GetOverrideBytesAs<Vector2f>(Slot));
			if (IsShaderVector3Type(Slot.Type)) return MakeShared<PropertyVector3fItem>(Owner, Label, GetOverrideBytesAs<Vector3f>(Slot));
			if (IsShaderVector4Type(Slot.Type)) return MakeShared<PropertyVector4fItem>(Owner, Label, GetOverrideBytesAs<Vector4f>(Slot));
			if (IsShaderIntVector2Type(Slot.Type) || IsShaderBoolVector2Type(Slot.Type)) return MakeShared<PropertyVector2iItem>(Owner, Label, GetOverrideBytesAs<int32>(Slot));
			if (IsShaderIntVector3Type(Slot.Type) || IsShaderBoolVector3Type(Slot.Type)) return MakeShared<PropertyVector3iItem>(Owner, Label, GetOverrideBytesAs<int32>(Slot));
			if (IsShaderIntVector4Type(Slot.Type) || IsShaderBoolVector4Type(Slot.Type)) return MakeShared<PropertyVector4iItem>(Owner, Label, GetOverrideBytesAs<int32>(Slot));
			if (IsShaderUintVector2Type(Slot.Type)) return MakeShared<PropertyVector2iItem>(Owner, Label, GetOverrideBytesAs<int32>(Slot));
			if (IsShaderUintVector3Type(Slot.Type)) return MakeShared<PropertyVector3iItem>(Owner, Label, GetOverrideBytesAs<int32>(Slot));
			if (IsShaderUintVector4Type(Slot.Type)) return MakeShared<PropertyVector4iItem>(Owner, Label, GetOverrideBytesAs<int32>(Slot));
			return MakeShared<PropertyItemBase>(Owner, Label);
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
				MaterialOverrideSlot& Slot = OverrideSlots[i];
				bool bExists = false;
				if (Slot.bIsResource)
				{
					for (const auto& R : MaterialAsset->BindingResourceDefaults)
					{
						if (R.BindingName == Slot.BindingName && R.Stage == Slot.Stage)
						{
							bExists = true;
							Slot.Type = GetTextureOverrideType(R.BindingType);
							break;
						}
					}
				}
				else
				{
					for (const auto& M : MaterialAsset->BindingMemberDefaults)
					{
						if (M.BindingName == Slot.BindingName && M.MemberName == Slot.MemberName && M.Stage == Slot.Stage)
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
		EnsureOverridePins();
		SyncOverridePinsFromSlots();
		InvalidateRenderResources();
	}

	void MeshRenderObject::InvalidateRenderResources()
	{
		Pipeline.SafeRelease();
		BindGroupLayouts.Empty();
		BindGroups.Empty();
		UniformBuffers.Empty();
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
		EnsureOverridePins();
		SyncOverridePinsFromSlots();
	}

	void MeshRenderObject::EnsureOverridePins()
	{
		for (MaterialOverrideSlot& Slot : OverrideSlots)
		{
			GraphPin* ExistingPin = nullptr;
			if (Slot.PinGuid.IsValid())
			{
				for (const auto& Pin : OverridePins)
				{
					if (Pin && Pin->GetGuid() == Slot.PinGuid)
					{
						ExistingPin = Pin.Get();
						break;
					}
				}
			}

			bool bPinTypeValid = false;
			if (ExistingPin)
			{
				if (Slot.bIsResource)
				{
					bPinTypeValid = Slot.Type.Contains(TEXT("Cube")) ? DynamicCast<GpuCubemapPin>(ExistingPin) != nullptr
						: Slot.Type.Contains(TEXT("3D")) ? DynamicCast<GpuTexture3DPin>(ExistingPin) != nullptr
						: DynamicCast<GpuTexturePin>(ExistingPin) != nullptr;
				}
				else
				{
					bPinTypeValid = DynamicCast<BytesPin>(ExistingPin) != nullptr;
				}
			}
			if (!bPinTypeValid)
			{
				if (ExistingPin)
				{
					BreakOverridePinLink(ExistingPin);
					OverridePins.RemoveAll([ExistingPin](const ObjectPtr<GraphPin>& Pin) {
						return Pin.Get() == ExistingPin;
					});
				}

				auto NewPin = CreateOverridePin(this, Slot.Type, Slot.bIsResource);
				if (!NewPin)
				{
					continue;
				}
				NewPin->Direction = PinDirection::Input;
				NewPin->ObjectName = FText::FromString(Slot.MemberName.IsEmpty() ? Slot.BindingName : Slot.BindingName + TEXT(".") + Slot.MemberName);
				Slot.PinGuid = NewPin->GetGuid();
				OverridePins.Add(MoveTemp(NewPin));
			}
			else
			{
				ExistingPin->Direction = PinDirection::Input;
				ExistingPin->ObjectName = FText::FromString(Slot.MemberName.IsEmpty() ? Slot.BindingName : Slot.BindingName + TEXT(".") + Slot.MemberName);
			}
		}

		OverridePins.RemoveAll([this](const ObjectPtr<GraphPin>& Pin) {
			const bool bOrphan = !Pin || !OverrideSlots.ContainsByPredicate([&](const MaterialOverrideSlot& Slot) {
				return Slot.PinGuid.IsValid() && Pin->GetGuid() == Slot.PinGuid;
			});
			if (bOrphan && Pin)
			{
				BreakOverridePinLink(Pin.Get());
			}
			return bOrphan;
		});
	}

	void MeshRenderObject::SyncOverridePinsFromSlots()
	{
		for (const MaterialOverrideSlot& Slot : OverrideSlots)
		{
			if (Slot.bIsResource || !Slot.PinGuid.IsValid())
			{
				continue;
			}
			for (const auto& Pin : OverridePins)
			{
				if (Pin && Pin->GetGuid() == Slot.PinGuid)
				{
					if (auto* BytesPinValue = DynamicCast<BytesPin>(Pin.Get()))
					{
						if (!BytesPinValue->SourcePin.IsValid())
						{
							BytesPinValue->SetBytes(Slot.Bytes);
						}
					}
					break;
				}
			}
		}
	}

	TArray<TSharedRef<PropertyData>> MeshRenderObject::GeneratePropertyDatas()
	{
		TArray<TSharedRef<PropertyData>> Result = ShObject::GeneratePropertyDatas();

		auto OverrideCat = MakeShared<PropertyCategory>(this, FText::FromString(TEXT("Overrides")));
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
						bool Used = OverrideSlots.ContainsByPredicate([&](const MaterialOverrideSlot& Slot) {
							return !Slot.bIsResource && Slot.BindingName == MemberDefault.BindingName && Slot.MemberName == MemberDefault.MemberName && Slot.Stage == MemberDefault.Stage;
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
						if (ResourceDefault.BindingType != BindingType::Texture && ResourceDefault.BindingType != BindingType::TextureCube
							&& ResourceDefault.BindingType != BindingType::Texture3D
							&& ResourceDefault.BindingType != BindingType::CombinedTextureSampler
							&& ResourceDefault.BindingType != BindingType::CombinedTextureCubeSampler
							&& ResourceDefault.BindingType != BindingType::CombinedTexture3DSampler)
						{
							continue;
						}

						bool Used = OverrideSlots.ContainsByPredicate([&](const MaterialOverrideSlot& Slot) {
							return Slot.bIsResource && Slot.BindingName == ResourceDefault.BindingName && Slot.Stage == ResourceDefault.Stage;
						});
						if (Used)
						{
							continue;
						}

						FString TypeStr = GetTextureOverrideType(ResourceDefault.BindingType);
						MenuBuilder.AddMenuEntry(FText::FromString(ResourceDefault.BindingName), FText::GetEmpty(), FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([this, ResourceDefault, TypeStr] {
								AddOverride(ResourceDefault.BindingName, TEXT(""), TypeStr, ResourceDefault.Stage, true);
								if (auto* OwnerNode = dynamic_cast<MeshPassNode*>(GetOuter()))
								{
									OwnerNode->RefreshNodeWidget();
								}
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
			MaterialOverrideSlot& Slot = OverrideSlots[Index];
			FString Label = Slot.BindingName
				+ (Slot.MemberName.IsEmpty() ? TEXT("") : (TEXT(".") + Slot.MemberName));
			const FText LabelText = FText::FromString(Label);
			GraphPin* OverridePin = FindOverridePin(Slot.BindingName, Slot.MemberName, Slot.Stage);
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
			int32 EntryIndex = Index;
			Entry->SetOnDelete([this, EntryIndex] {
				RemoveOverride(EntryIndex);
				if (auto* OwnerNode = dynamic_cast<MeshPassNode*>(GetOuter()))
				{
					OwnerNode->RefreshNodeWidget();
				}
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
		if (InProperty && InProperty->GetDisplayName().EqualTo(LOCALIZATION("MeshSceneObject")))
		{
			if (MeshSceneObjectRef.IsValid() && MeshSceneObjectRef.Get())
			{
				ObjectName = MeshSceneObjectRef->ObjectName;
			}
		}
		if (InProperty && InProperty->GetDisplayName().EqualTo(LOCALIZATION("Material")))
		{
			BindMaterialDelegates();
			InvalidateRenderResources();
		}
		if (InProperty && InProperty->IsOfType<PropertyAssetItem>())
		{
			const FString DisplayName = InProperty->GetDisplayName().ToString();
			const bool bOverrideResourceChanged = OverrideSlots.ContainsByPredicate([&](const MaterialOverrideSlot& Slot) {
				return Slot.bIsResource && (Slot.MemberName.IsEmpty() ? Slot.BindingName : Slot.MemberName) == DisplayName;
			});
			if (bOverrideResourceChanged)
			{
				InvalidateRenderResources();
			}
		}
		SyncOverridePinsFromSlots();
		if (auto* OwnerNode = dynamic_cast<MeshPassNode*>(GetOuter()))
		{
			OwnerNode->RefreshNodeWidget();
		}
	}

	ObjectPtr<GraphPin> MeshRenderObject::CreateOverridePin(ShObject* Outer, const FString& Type, bool bIsResource)
	{
		if (bIsResource)
		{
			if (Type.Contains(TEXT("Cube")))
			{
				return NewShObject<GpuCubemapPin>(Outer);
			}
			if (Type.Contains(TEXT("3D")))
			{
				return NewShObject<GpuTexture3DPin>(Outer);
			}
			return NewShObject<GpuTexturePin>(Outer);
		}

		return NewShObject<BytesPin>(Outer);
	}

	void MeshRenderObject::AddOverride(const FString& BindingName, const FString& MemberName, const FString& Type, BindingShaderStage Stage, bool bIsResource)
	{
		MaterialOverrideSlot Slot;
		Slot.BindingName = BindingName;
		Slot.MemberName = MemberName;
		Slot.Type = Type;
		Slot.Stage = Stage;
		Slot.bIsResource = bIsResource;

		if (MaterialAsset)
		{
			if (bIsResource)
			{
				if (const auto* ResourceDefault = MaterialAsset->BindingResourceDefaults.FindByPredicate([&](const MaterialBindingResourceDefault& Default) {
					return Default.BindingName == BindingName && Default.Stage == Stage;
				}))
				{
					Slot.TextureAsset = ResourceDefault->TextureAsset;
				}
			}
			else
			{
				if (const auto* MemberDefault = MaterialAsset->BindingMemberDefaults.FindByPredicate([&](const MaterialBindingMemberDefault& Default) {
					return Default.BindingName == BindingName && Default.MemberName == MemberName && Default.Stage == Stage;
				}))
				{
					Slot.Bytes = MakeDefaultOverrideBytes(*MemberDefault);
				}
			}
		}

		auto Pin = CreateOverridePin(this, Type, bIsResource);
		if (Pin)
		{
			Pin->Direction = PinDirection::Input;
			Pin->ObjectName = FText::FromString(MemberName.IsEmpty() ? BindingName : BindingName + TEXT(".") + MemberName);
			if (auto* BytesPinValue = DynamicCast<BytesPin>(Pin.Get()))
			{
				BytesPinValue->SetBytes(Slot.Bytes);
			}
			Slot.PinGuid = Pin->GetGuid();
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
		MaterialOverrideSlot Slot = OverrideSlots[SlotIndex];
		if (Slot.PinGuid.IsValid())
		{
			OverridePins.RemoveAll([&](const ObjectPtr<GraphPin>& P) {
				if (P && P->GetGuid() == Slot.PinGuid)
				{
					BreakOverridePinLink(P.Get());
					return true;
				}
				return false;
			});
		}
		OverrideSlots.RemoveAt(SlotIndex);
		InvalidateRenderResources();
		if (auto* OuterMost = GetOuterMost()) OuterMost->MarkDirty();
	}

	GraphPin* MeshRenderObject::FindOverridePin(const FString& BindingName, const FString& MemberName, BindingShaderStage Stage) const
	{
		for (int32 i = 0; i < OverrideSlots.Num(); ++i)
		{
			const MaterialOverrideSlot& S = OverrideSlots[i];
			if (S.BindingName == BindingName && S.MemberName == MemberName && S.Stage == Stage)
			{
				for (const auto& P : OverridePins)
				{
					if (P && P->GetGuid() == S.PinGuid) return P.Get();
				}
			}
		}
		return nullptr;
	}

	void MeshRenderObject::BuildBindGroupFromMaterial(bool bRebuildLayouts, bool bRebuildUniformBuffers)
	{
		if (!MaterialAsset) return;

		MaterialBindGroupBuildOptions Options;
		Options.bRebuildLayouts = bRebuildLayouts;
		Options.bRebuildUniformBuffers = bRebuildUniformBuffers;
		Options.TextureOverrideResolver = [this](const GpuShaderLayoutBinding& Binding) -> GpuTexture* {
				if (GraphPin* OverridePin = FindOverridePin(Binding.Name, TEXT(""), Binding.Stage))
				{
					if (GpuTexture* Texture = GetConnectedOverrideTexture(OverridePin)) return Texture;
				}

				if (const auto* Override = OverrideSlots.FindByPredicate([&](const MaterialOverrideSlot& Slot) {
					return Slot.bIsResource && Slot.BindingName == Binding.Name && Slot.Stage == Binding.Stage;
				}))
				{
					if (GpuTexture* Texture = GetTextureData(Override->TextureAsset.Get())) return Texture;
					if (Binding.Type == BindingType::TextureCube || Binding.Type == BindingType::CombinedTextureCubeSampler) return GpuResourceHelper::GetGlobalBlackCubemapTex();
					if (Binding.Type == BindingType::Texture3D || Binding.Type == BindingType::CombinedTexture3DSampler) return GpuResourceHelper::GetGlobalBlackVolumeTex();
					return GpuResourceHelper::GetGlobalBlackTex();
				}
				return nullptr;
			};

		BuildMaterialBindGroups(*MaterialAsset, BindGroupLayouts, BindGroups, UniformBuffers, Options);
	}

	bool MeshRenderObject::UsesTextureAsShaderInput(GpuTexture* Texture, FString& OutBindingName) const
	{
		if (!MaterialAsset || bDrawMaterialError)
		{
			return false;
		}

		for (const MaterialBindingResourceDefault& ResourceDefault : MaterialAsset->BindingResourceDefaults)
		{
			if (!IsTextureShaderInputBinding(ResourceDefault.BindingType))
			{
				continue;
			}

			GpuTexture* ShaderInputTexture = nullptr;
			if (GraphPin* OverridePin = FindOverridePin(ResourceDefault.BindingName, TEXT(""), ResourceDefault.Stage))
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
		const MeshPassNode* OwnerNode = dynamic_cast<MeshPassNode*>(GetOuter());
		const FString NodeName = OwnerNode ? OwnerNode->ObjectName.ToString() : TEXT("<unknown>");
		const FString RenderObjectName = ObjectName.ToString();

		if (!MaterialAsset)
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" MeshRenderObject:\"%s\" cannot build pipeline because no material is assigned."), *NodeName, *RenderObjectName);
			return false;
		}

		const FString MaterialPath = MaterialAsset->GetPath();
		const FString MaterialName = MaterialPath.IsEmpty() ? MaterialAsset->ObjectName.ToString() : MaterialPath;
		if (!MaterialAsset->VertexShaderAsset)
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" MeshRenderObject:\"%s\" material \"%s\" cannot build pipeline because the vertex shader asset is missing."), *NodeName, *RenderObjectName, *MaterialName);
			return false;
		}
		if (!MaterialAsset->PixelShaderAsset)
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" MeshRenderObject:\"%s\" material \"%s\" cannot build pipeline because the pixel shader asset is missing."), *NodeName, *RenderObjectName, *MaterialName);
			return false;
		}

		GpuShader* Vs = MaterialAsset->VertexShaderAsset->GetCompiledShader(ShaderType::Vertex);
		GpuShader* Ps = MaterialAsset->PixelShaderAsset->GetCompiledShader(ShaderType::Pixel);
		if (!Vs)
		{
			const FString VertexShaderPath = MaterialAsset->VertexShaderAsset->GetPath();
			const FString VertexShaderName = VertexShaderPath.IsEmpty() ? MaterialAsset->VertexShaderAsset->ObjectName.ToString() : VertexShaderPath;
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" MeshRenderObject:\"%s\" material \"%s\" cannot build pipeline because the %s shader is unavailable. Shader asset: %s"), *NodeName, *RenderObjectName, *MaterialName, ANSI_TO_TCHAR(magic_enum::enum_name(ShaderType::Vertex).data()), *VertexShaderName);
			return false;
		}
		if (!Ps)
		{
			const FString PixelShaderPath = MaterialAsset->PixelShaderAsset->GetPath();
			const FString PixelShaderName = PixelShaderPath.IsEmpty() ? MaterialAsset->PixelShaderAsset->ObjectName.ToString() : PixelShaderPath;
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" MeshRenderObject:\"%s\" material \"%s\" cannot build pipeline because the %s shader is unavailable. Shader asset: %s"), *NodeName, *RenderObjectName, *MaterialName, ANSI_TO_TCHAR(magic_enum::enum_name(ShaderType::Pixel).data()), *PixelShaderName);
			return false;
		}
		if (Vs->GetShaderLanguage() != Ps->GetShaderLanguage())
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" MeshRenderObject:\"%s\" material \"%s\" cannot build pipeline because shader languages differ (VS=%d, PS=%d)."), *NodeName, *RenderObjectName, *MaterialName, static_cast<int32>(Vs->GetShaderLanguage()), static_cast<int32>(Ps->GetShaderLanguage()));
			return false;
		}

		if (BindGroupLayouts.IsEmpty())
		{
			BuildBindGroupFromMaterial();
		}

		GpuVertexLayoutDesc VertexLayoutDesc = BuildMaterialMeshVertexLayout(*MaterialAsset);

		TArray<GpuBindGroupLayout*> LayoutArray;
		for (auto& [_, L] : BindGroupLayouts) LayoutArray.Add(L.GetReference());

		TArray<PipelineTargetDesc, TFixedAllocator<GpuResourceLimit::MaxRenderTargetNum>> Targets;
		for (GpuFormat F : ColorFormats)
		{
			Targets.Add(PipelineTargetDesc{
				.TargetFormat = F,
				.BlendEnable = MaterialAsset->BlendEnable,
				.SrcFactor = MaterialAsset->SrcBlendFactor,
				.ColorOp = MaterialAsset->ColorBlendOp,
				.DestFactor = MaterialAsset->DestBlendFactor,
			});
		}

		GpuRenderPipelineStateDesc Desc{
			.Vs = Vs,
			.Ps = Ps,
			.Targets = MoveTemp(Targets),
			.BindGroupLayouts = MoveTemp(LayoutArray),
			.VertexLayout = { MoveTemp(VertexLayoutDesc) },
			.RasterizerState = { .FillMode = MaterialAsset->FillMode, .CullMode = MaterialAsset->CullMode },
			.Primitive = MaterialAsset->Primitive,
			.SampleCount = SampleCount,
			.DepthStencilState = (DepthFormat != GpuFormat::NUM && MaterialAsset->DepthTestEnable)
				? TOptional<DepthStencilStateDesc>(DepthStencilStateDesc{ .DepthFormat = DepthFormat, .DepthCompare = MaterialAsset->DepthCompare })
				: TOptional<DepthStencilStateDesc>(),
		};

		try
		{
			Pipeline = GpuPsoCacheManager::Get().CreateRenderPipelineState(Desc);
		}
		catch (const std::runtime_error& Error)
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" MeshRenderObject:\"%s\" material \"%s\" failed to create render pipeline state: %s"), *NodeName, *RenderObjectName, *MaterialName, UTF8_TO_TCHAR(Error.what()));
			return false;
		}
		if (!Pipeline.IsValid())
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" MeshRenderObject:\"%s\" material \"%s\" failed to create a valid render pipeline state."), *NodeName, *RenderObjectName, *MaterialName);
		}
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

	void MeshRenderObject::Draw(GpuRenderPassRecorder* Recorder, const Camera* InCamera, const FMatrix44f& ModelMatrix)
	{
		if (!MeshSceneObjectRef.IsValid())
		{
			return;
		}
		MeshSceneObject* MSO = MeshSceneObjectRef.Get();
		if (!MSO || !MSO->ModelAsset) return;

		Model* ModelAsset = MSO->ModelAsset.Get();
		const FMatrix44f ViewMat = InCamera ? InCamera->GetViewMatrix() : FMatrix44f::Identity;
		const FMatrix44f ProjMat = InCamera ? InCamera->GetProjectionMatrix() : FMatrix44f::Identity;
		const FMatrix44f ViewProjMat = ViewMat * ProjMat;
		const FMatrix44f MVPMat = ModelMatrix * ViewProjMat;

		if (bDrawMaterialError)
		{
			const TArray<MeshBuffers>& GpuMeshes = ModelAsset->GetGpuMeshes();
			for (const MeshBuffers& MeshBuffer : GpuMeshes)
			{
				DrawMaterialErrorMesh(Recorder, ErrorResources, MeshBuffer, MVPMat);
			}
			return;
		}

		if (!MaterialAsset || !Pipeline.IsValid())
		{
			return;
		}

		bool bHasResourceOverride = false;
		TSet<GpuTexture*> ConnectedOverrideTextures;
		for (const auto& Slot : OverrideSlots)
		{
			if (Slot.bIsResource)
			{
				bHasResourceOverride = true;
				if (GraphPin* OverridePin = FindOverridePin(Slot.BindingName, TEXT(""), Slot.Stage))
				{
					if (GpuTexture* Texture = GetConnectedOverrideTexture(OverridePin))
					{
						ConnectedOverrideTextures.Add(Texture);
					}
				}
			}
		}
		if (!ConnectedOverrideTextures.IsEmpty())
		{
			TArray<GpuBarrierInfo> BarrierInfos;
			for (GpuTexture* Texture : ConnectedOverrideTextures)
			{
				BarrierInfos.Emplace(Texture, GpuResourceState::ShaderResourceRead);
			}
			Recorder->Barriers(BarrierInfos);
		}
		if (bHasResourceOverride)
		{
			BuildBindGroupFromMaterial(false, false);
		}

		MaterialUniformBufferUpdateOptions UniformOptions;
		UniformOptions.ModelMatrix = ModelMatrix;
		UniformOptions.ViewMatrix = ViewMat;
		UniformOptions.ProjMatrix = ProjMat;
		UniformOptions.ViewProjMatrix = ViewProjMat;
		UniformOptions.MVPMatrix = MVPMat;
		UniformOptions.UniformOverrideBytesResolver = [this](const MaterialBindingMemberDefault& MemberDefault) -> const uint8* {
			const MaterialOverrideSlot* Override = OverrideSlots.FindByPredicate([&](const MaterialOverrideSlot& Slot) {
				return !Slot.bIsResource && Slot.BindingName == MemberDefault.BindingName && Slot.MemberName == MemberDefault.MemberName && Slot.Stage == MemberDefault.Stage;
			});
			if (!Override)
			{
				return nullptr;
			}

			if (GraphPin* OverridePin = FindOverridePin(MemberDefault.BindingName, MemberDefault.MemberName, MemberDefault.Stage))
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

		TArray<GpuBindGroup*> BGArray;
		for (auto& [_, BG] : BindGroups) BGArray.Add(BG.GetReference());

		const TArray<MeshBuffers>& GpuMeshes = ModelAsset->GetGpuMeshes();
		for (const MeshBuffers& MB : GpuMeshes)
		{
			Recorder->SetRenderPipelineState(Pipeline);
			Recorder->SetBindGroups(BGArray);
			Recorder->SetVertexBuffer(0, MB.VertexBuffer);
			Recorder->SetIndexBuffer(MB.IndexBuffer);
			Recorder->DrawIndexed(0, MB.IndexCount);
		}
	}
}
