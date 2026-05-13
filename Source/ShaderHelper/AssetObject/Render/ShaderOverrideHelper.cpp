#include "CommonHeader.h"
#include "ShaderOverrideHelper.h"
#include "AssetObject/Graph.h"
#include "AssetObject/Pins/Pins.h"
#include "AssetObject/Texture2D.h"
#include "AssetObject/Texture3D.h"
#include "AssetObject/TextureCube.h"
#include "Renderer/MaterialRenderCommon.h"
#include "GpuApi/GpuResourceHelper.h"
#include "GpuApi/GpuRhi.h"
#include "GpuApi/GpuShader.h"
#include "UI/Widgets/Property/PropertyData/PropertyMatrixItem.h"

using namespace FW;

namespace SH
{
	static GpuFormat ToGpuFormat(RWTextureFormat Format)
	{
		switch (Format)
		{
		case RWTextureFormat::R8_UNORM:              return GpuFormat::R8_UNORM;
		case RWTextureFormat::R8G8B8A8_UNORM:        return GpuFormat::R8G8B8A8_UNORM;
		case RWTextureFormat::R16_UINT:              return GpuFormat::R16_UINT;
		case RWTextureFormat::R32_UINT:              return GpuFormat::R32_UINT;
		case RWTextureFormat::R16G16B16A16_UINT:     return GpuFormat::R16G16B16A16_UINT;
		case RWTextureFormat::R32G32_UINT:           return GpuFormat::R32G32_UINT;
		case RWTextureFormat::R32G32B32A32_UINT:     return GpuFormat::R32G32B32A32_UINT;
		case RWTextureFormat::R16_FLOAT:             return GpuFormat::R16_FLOAT;
		case RWTextureFormat::R32_FLOAT:             return GpuFormat::R32_FLOAT;
		case RWTextureFormat::R32G32_FLOAT:          return GpuFormat::R32G32_FLOAT;
		case RWTextureFormat::R16G16B16A16_FLOAT:    return GpuFormat::R16G16B16A16_FLOAT;
		case RWTextureFormat::R32G32B32A32_FLOAT:    return GpuFormat::R32G32B32A32_FLOAT;
		case RWTextureFormat::R16G16B16A16_UNORM:    return GpuFormat::R16G16B16A16_UNORM;
		}
		AUX::Unreachable();
	}

	FArchive& operator<<(FArchive& Ar, ShaderOverrideKey& Key)
	{
		Ar << Key.BindingName << Key.MemberName << Key.Stage;
		return Ar;
	}

	FArchive& operator<<(FArchive& Ar, ShaderOverrideSlot& S)
	{
		Ar << S.Key << S.Type << S.bIsResource << S.InputPin << S.Bytes << S.TextureAsset << S.OutputPin;
		Ar << S.DefaultRWSize << S.DefaultRWFormat;
		return Ar;
	}

	FString MakeOverrideKeyLabel(const ShaderOverrideKey& Key)
	{
		return Key.MemberName.IsEmpty() ? Key.BindingName : Key.BindingName + TEXT(".") + Key.MemberName;
	}

	FString MakeOverrideSlotLabel(const ShaderOverrideSlot& Slot)
	{
		return MakeOverrideKeyLabel(Slot.Key);
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

	void EnsureOverrideBytesStorage(ShaderOverrideSlot& Slot)
	{
		const int32 RequiredSize = GetOverrideByteSize(Slot.Type);
		if (Slot.Bytes.Num() >= RequiredSize)
		{
			return;
		}

		if (IsShaderMatrix4x4Type(Slot.Type))
		{
			Slot.Bytes.SetNumUninitialized(RequiredSize);
			const FMatrix44f Identity = FMatrix44f::Identity;
			FMemory::Memcpy(Slot.Bytes.GetData(), &Identity, RequiredSize);
		}
		else
		{
			Slot.Bytes.SetNumZeroed(RequiredSize);
		}
	}

	MetaType* GetTextureMetaType(const FString& Type)
	{
		if (Type.Contains(TEXT("Cube"))) return GetMetaType<TextureCube>();
		if (Type.Contains(TEXT("3D"))) return GetMetaType<Texture3D>();
		return GetMetaType<Texture2D>();
	}

	bool IsResourceOverrideBinding(BindingType BindingTypeValue)
	{
		switch (BindingTypeValue)
		{
		case BindingType::Texture:
		case BindingType::TextureCube:
		case BindingType::Texture3D:
		case BindingType::Sampler:
		case BindingType::CombinedTextureSampler:
		case BindingType::CombinedTextureCubeSampler:
		case BindingType::CombinedTexture3DSampler:
		case BindingType::RWTexture:
		case BindingType::RWTexture3D:
			return true;
		default:
			return false;
		}
	}

	bool IsPinnableResourceBinding(BindingType BindingTypeValue)
	{
		return IsResourceOverrideBinding(BindingTypeValue) && BindingTypeValue != BindingType::Sampler;
	}

	FString GetResourceOverrideType(BindingType BindingTypeValue)
	{
		switch (BindingTypeValue)
		{
		case BindingType::TextureCube:
		case BindingType::CombinedTextureCubeSampler:
			return TEXT("TextureCube");
		case BindingType::Texture3D:
		case BindingType::CombinedTexture3DSampler:
			return TEXT("Texture3D");
		case BindingType::RWTexture3D:
			return TEXT("RWTexture3D");
		case BindingType::RWTexture:
			return TEXT("RWTexture2D");
		case BindingType::Sampler:
			return TEXT("Sampler");
		default:
			return TEXT("Texture2D");
		}
	}

	bool IsRWResourceType(const FString& Type)
	{
		return Type.StartsWith(TEXT("RW"));
	}

	ObjectPtr<GraphPin> CreateOverridePinForType(ShObject* Outer, const FString& Type, bool bIsResource)
	{
		if (bIsResource)
		{
			if (Type == TEXT("Sampler")) return nullptr;
			if (Type.Contains(TEXT("Cube"))) return NewShObject<GpuCubemapPin>(Outer);
			if (Type.Contains(TEXT("3D"))) return NewShObject<GpuTexture3DPin>(Outer);
			return NewShObject<GpuTexturePin>(Outer);
		}
		return NewShObject<BytesPin>(Outer);
	}

	bool IsOverridePinTypeValid(GraphPin* Pin, const FString& Type, bool bIsResource)
	{
		if (!Pin) return false;
		if (!bIsResource) return DynamicCast<BytesPin>(Pin) != nullptr;
		if (Type == TEXT("Sampler")) return false;
		return Type.Contains(TEXT("Cube")) ? DynamicCast<GpuCubemapPin>(Pin) != nullptr
			: Type.Contains(TEXT("3D")) ? DynamicCast<GpuTexture3DPin>(Pin) != nullptr
			: DynamicCast<GpuTexturePin>(Pin) != nullptr;
	}

	GpuTexture* GetDefaultOverrideTexture(BindingType BindingTypeValue)
	{
		if (BindingTypeValue == BindingType::RWTexture3D)
		{
			static TRefCountPtr<GpuTexture> DefaultRWTexture3D = []{
				constexpr uint32 ByteSize = 1 * 1 * 1 * 4;
				TArray<uint8> ZeroData;
				ZeroData.SetNumZeroed(ByteSize);
				return GGpuRhi->CreateTexture({
					.Width = 1,
					.Height = 1,
					.Format = GpuFormat::R8G8B8A8_UNORM,
					.Usage = GpuTextureUsage::ShaderResource | GpuTextureUsage::UnorderedAccess,
					.InitialData = ZeroData,
					.Depth = 1,
					.Dimension = GpuTextureDimension::Tex3D,
				}, GpuResourceState::UnorderedAccess);
			}();
			return DefaultRWTexture3D;
		}
		if (BindingTypeValue == BindingType::RWTexture)
		{
			static TRefCountPtr<GpuTexture> DefaultRWTexture = []{
				constexpr uint32 ByteSize = 1 * 1 * 4;
				TArray<uint8> ZeroData;
				ZeroData.SetNumZeroed(ByteSize);
				return GGpuRhi->CreateTexture({
					.Width = 1,
					.Height = 1,
					.Format = GpuFormat::R8G8B8A8_UNORM,
					.Usage = GpuTextureUsage::ShaderResource | GpuTextureUsage::UnorderedAccess,
					.InitialData = ZeroData,
				}, GpuResourceState::UnorderedAccess);
			}();
			return DefaultRWTexture;
		}
		if (BindingTypeValue == BindingType::TextureCube || BindingTypeValue == BindingType::CombinedTextureCubeSampler)
		{
			return GpuResourceHelper::GetGlobalBlackCubemapTex();
		}
		if (BindingTypeValue == BindingType::Texture3D || BindingTypeValue == BindingType::CombinedTexture3DSampler)
		{
			return GpuResourceHelper::GetGlobalBlackVolumeTex();
		}
		return GpuResourceHelper::GetGlobalBlackTex();
	}

	GpuTexture* GetConnectedOverrideTexture(GraphPin* InputPin)
	{
		GraphPin* SourcePin = InputPin ? InputPin->GetSourcePin() : nullptr;
		if (auto* TexturePin = DynamicCast<GpuTexturePin>(SourcePin)) return TexturePin->GetValue();
		if (auto* CubemapPin = DynamicCast<GpuCubemapPin>(SourcePin)) return CubemapPin->GetValue();
		if (auto* Texture3DPin = DynamicCast<GpuTexture3DPin>(SourcePin)) return Texture3DPin->GetValue();
		return nullptr;
	}

	bool IsDefaultRWTextureProperty(const TArray<ShaderOverrideSlot>& Slots, PropertyData* InProperty)
	{
		const FText DisplayName = InProperty->GetDisplayName();
		const bool bDefaultRWChild = DisplayName.EqualTo(LOCALIZATION("WidthAuto"))
			|| DisplayName.EqualTo(LOCALIZATION("HeightAuto"))
			|| DisplayName.EqualTo(LOCALIZATION("Depth"))
			|| DisplayName.EqualTo(LOCALIZATION("Format"));
		if (!bDefaultRWChild)
		{
			return false;
		}

		for (PropertyData* Parent = InProperty->GetParent(); Parent; Parent = Parent->GetParent())
		{
			const FString ParentName = Parent->GetDisplayName().ToString();
			if (Slots.ContainsByPredicate([&](const ShaderOverrideSlot& Slot) {
				return Slot.bIsResource && IsRWResourceType(Slot.Type) && MakeOverrideSlotLabel(Slot) == ParentName;
			}))
			{
				return true;
			}
		}
		return false;
	}

	void BreakOverridePinLink(GraphPin* Pin)
	{
		if (!Pin)
		{
			return;
		}
		Graph* OwnerGraph = static_cast<Graph*>(Pin->GetOuterMost());
		if (Pin->SourcePin.IsValid())
		{
			OwnerGraph->RemoveLink(Pin->GetSourcePin(), Pin);
		}

		TArray<GraphPin*> TargetPins = Pin->GetTargetPins();
		for (GraphPin* TargetPin : TargetPins)
		{
			OwnerGraph->RemoveLink(Pin, TargetPin);
		}
	}

	GpuTexture* ResolveOverrideTexture(BindingType BindingTypeValue, GraphPin* InputPin, ShaderOverrideSlot* MatchingSlot, Vector2f ViewportSize)
	{
		if (InputPin)
		{
			if (GpuTexture* Tex = GetConnectedOverrideTexture(InputPin)) return Tex;
		}
		if (MatchingSlot && MatchingSlot->TextureAsset)
		{
			if (GpuTexture* Tex = ResolveTextureAssetGpu(MatchingSlot->TextureAsset.Get())) return Tex;
		}
		// For RW resources with no asset and no incoming connection
		if (MatchingSlot && (BindingTypeValue == BindingType::RWTexture || BindingTypeValue == BindingType::RWTexture3D))
		{
			const bool bIs3D = BindingTypeValue == BindingType::RWTexture3D;
			auto ResolveExtent = [](int32 SlotValue, float ViewportValue) -> uint32 {
				if (SlotValue <= 0 && ViewportValue > 0) return static_cast<uint32>(ViewportValue);
				return static_cast<uint32>(FMath::Max(1, SlotValue));
			};
			const uint32 W = ResolveExtent(MatchingSlot->DefaultRWSize.x, ViewportSize.X);
			const uint32 H = ResolveExtent(MatchingSlot->DefaultRWSize.y, ViewportSize.Y);
			const uint32 D = static_cast<uint32>(FMath::Max(1, MatchingSlot->DefaultRWSize.z));
			const GpuFormat Fmt = ToGpuFormat(MatchingSlot->DefaultRWFormat);
			GpuTexture* Existing = MatchingSlot->DefaultRWTexture.GetReference();
			const bool bMatches = Existing
				&& Existing->GetWidth() == W
				&& Existing->GetHeight() == H
				&& Existing->GetFormat() == Fmt
				&& (!bIs3D || Existing->GetDepth() == D);
			if (!bMatches)
			{
				const uint32 ByteSize = W * H * D * GetFormatByteSize(Fmt);
				TArray<uint8> ZeroData;
				ZeroData.SetNumZeroed(static_cast<int32>(ByteSize));

				GpuTextureDesc Desc{
					.Width = W,
					.Height = H,
					.Format = Fmt,
					.Usage = GpuTextureUsage::ShaderResource | GpuTextureUsage::UnorderedAccess,
					.InitialData = ZeroData,
				};
				if (bIs3D)
				{
					Desc.Depth = D;
					Desc.Dimension = GpuTextureDimension::Tex3D;
				}
				MatchingSlot->DefaultRWTexture = GGpuRhi->CreateTexture(Desc, GpuResourceState::UnorderedAccess);
			}
			return MatchingSlot->DefaultRWTexture.GetReference();
		}
		return GetDefaultOverrideTexture(BindingTypeValue);
	}

	ShaderOverrideSlot* FindOverrideSlot(TArray<ShaderOverrideSlot>& Slots, const ShaderOverrideKey& Key)
	{
		return Slots.FindByPredicate([&Key](const ShaderOverrideSlot& Slot) {
			return Slot.Key == Key;
		});
	}

	const ShaderOverrideSlot* FindOverrideSlot(const TArray<ShaderOverrideSlot>& Slots, const ShaderOverrideKey& Key)
	{
		return Slots.FindByPredicate([&Key](const ShaderOverrideSlot& Slot) {
			return Slot.Key == Key;
		});
	}

	GraphPin* FindOverrideInputPin(const TArray<ShaderOverrideSlot>& Slots, const ShaderOverrideKey& Key)
	{
		const ShaderOverrideSlot* Slot = FindOverrideSlot(Slots, Key);
		return Slot && Slot->InputPin.IsValid() ? Slot->InputPin.Get() : nullptr;
	}

	void PreserveOverrideSlotData(TArray<ShaderOverrideSlot>& NewSlots, const TArray<ShaderOverrideSlot>& OldSlots)
	{
		for (ShaderOverrideSlot& NewSlot : NewSlots)
		{
			for (const ShaderOverrideSlot& Old : OldSlots)
			{
				if (!(Old.Key == NewSlot.Key)
					|| Old.bIsResource != NewSlot.bIsResource
					|| Old.Type != NewSlot.Type)
				{
					continue;
				}
				if (NewSlot.bIsResource)
				{
					NewSlot.TextureAsset = Old.TextureAsset;
					NewSlot.DefaultRWSize = Old.DefaultRWSize;
					NewSlot.DefaultRWFormat = Old.DefaultRWFormat;
				}
				else
				{
					NewSlot.Bytes = Old.Bytes;
				}
				NewSlot.InputPin = Old.InputPin;
				NewSlot.OutputPin = Old.OutputPin;
				break;
			}
		}
	}

	void ReconcileOverridePins(ShObject* Owner, TArray<ShaderOverrideSlot>& Slots, TArray<ObjectPtr<GraphPin>>& Pins)
	{
		auto MakePinLabel = [](const ShaderOverrideSlot& Slot) {
			return FText::FromString(MakeOverrideSlotLabel(Slot));
		};

		auto ResolveSlotPin = [&Pins](ObserverObjectPtr<GraphPin>& PinRef) -> GraphPin* {
			if (PinRef.IsValid())
			{
				return PinRef.Get();
			}
			for (const ObjectPtr<GraphPin>& Pin : Pins)
			{
				if (Pin && PinRef == Pin)
				{
					PinRef.SetReference(Pin.Get());
					return Pin.Get();
				}
			}
			PinRef.Reset();
			return nullptr;
		};

		auto RemovePin = [&Pins](GraphPin* PinToRemove) {
			if (!PinToRemove) return;
			BreakOverridePinLink(PinToRemove);
			Pins.RemoveAll([PinToRemove](const ObjectPtr<GraphPin>& P) { return P.Get() == PinToRemove; });
		};

		for (ShaderOverrideSlot& Slot : Slots)
		{
			GraphPin* ExistingInputPin = ResolveSlotPin(Slot.InputPin);

			if (!IsOverridePinTypeValid(ExistingInputPin, Slot.Type, Slot.bIsResource))
			{
				RemovePin(ExistingInputPin);

				ObjectPtr<GraphPin> NewInputPin = CreateOverridePinForType(Owner, Slot.Type, Slot.bIsResource);
				if (!NewInputPin)
				{
					Slot.InputPin.Reset();
				}
				else
				{
					NewInputPin->Direction = PinDirection::Input;
					NewInputPin->ObjectName = MakePinLabel(Slot);
					if (auto* BytesPinValue = DynamicCast<BytesPin>(NewInputPin.Get()))
					{
						if (!Slot.Bytes.IsEmpty()) BytesPinValue->SetBytes(Slot.Bytes);
					}
					Slot.InputPin.SetReference(NewInputPin.Get());
					Pins.Add(MoveTemp(NewInputPin));
				}
			}
			else
			{
				ExistingInputPin->Direction = PinDirection::Input;
				ExistingInputPin->ObjectName = MakePinLabel(Slot);
				if (auto* BytesPinValue = DynamicCast<BytesPin>(ExistingInputPin))
				{
					if (!BytesPinValue->SourcePin.IsValid() && !Slot.Bytes.IsEmpty())
					{
						BytesPinValue->SetBytes(Slot.Bytes);
					}
				}
			}

			// Output pin: only for RW resource slots; same label as input pin.
			const bool bNeedsOutputPin = Slot.bIsResource && IsRWResourceType(Slot.Type);
			GraphPin* ExistingOutputPin = ResolveSlotPin(Slot.OutputPin);

			if (bNeedsOutputPin)
			{
				if (!IsOverridePinTypeValid(ExistingOutputPin, Slot.Type, true))
				{
					RemovePin(ExistingOutputPin);

					ObjectPtr<GraphPin> NewOutputPin = CreateOverridePinForType(Owner, Slot.Type, true);
					if (!NewOutputPin)
					{
						Slot.OutputPin.Reset();
					}
					else
					{
						NewOutputPin->Direction = PinDirection::Output;
						NewOutputPin->ObjectName = MakePinLabel(Slot);
						Slot.OutputPin.SetReference(NewOutputPin.Get());
						Pins.Add(MoveTemp(NewOutputPin));
					}
				}
				else
				{
					ExistingOutputPin->Direction = PinDirection::Output;
					ExistingOutputPin->ObjectName = MakePinLabel(Slot);
				}
			}
			else if (ExistingOutputPin)
			{
				RemovePin(ExistingOutputPin);
				Slot.OutputPin.Reset();
			}
		}

		Pins.RemoveAll([&Slots](const ObjectPtr<GraphPin>& Pin) {
			const bool bOrphan = !Pin || !Slots.ContainsByPredicate([&](const ShaderOverrideSlot& Slot) {
				return Slot.InputPin == Pin || Slot.OutputPin == Pin;
			});
			if (bOrphan && Pin)
			{
				BreakOverridePinLink(Pin.Get());
			}
			return bOrphan;
		});
	}

	TSharedRef<PropertyItemBase> MakeBytesPropertyItem(ShObject* Owner, FText Label, ShaderOverrideSlot& Slot)
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

	void AppendDefaultRWSizeChildren(ShObject* Owner, PropertyItemBase& Parent, ShaderOverrideSlot& Slot)
	{
		const bool bIs3D = Slot.Type.Contains(TEXT("3D"));

		auto WidthItem = MakeShared<PropertyScalarItem<int32>>(Owner, LOCALIZATION("WidthAuto"), &Slot.DefaultRWSize.x);
		WidthItem->SetMinValue(0);
		Parent.AddChild(WidthItem);

		auto HeightItem = MakeShared<PropertyScalarItem<int32>>(Owner, LOCALIZATION("HeightAuto"), &Slot.DefaultRWSize.y);
		HeightItem->SetMinValue(0);
		Parent.AddChild(HeightItem);

		if (bIs3D)
		{
			auto DepthItem = MakeShared<PropertyScalarItem<int32>>(Owner, LOCALIZATION("Depth"), &Slot.DefaultRWSize.z);
			DepthItem->SetMinValue(1);
			Parent.AddChild(DepthItem);
		}

		auto FormatItem = MakePropertyEnumItem<RWTextureFormat>(
			Owner, LOCALIZATION("Format"), Slot.DefaultRWFormat,
			[&Slot](RWTextureFormat NewValue) { Slot.DefaultRWFormat = NewValue; }
		);
		Parent.AddChild(FormatItem);
	}
}
