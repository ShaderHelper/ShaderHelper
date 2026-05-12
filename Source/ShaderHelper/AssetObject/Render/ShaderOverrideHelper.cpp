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
			static TRefCountPtr<GpuTexture> DefaultRWTexture3D = GGpuRhi->CreateTexture({
				.Width = 1,
				.Height = 1,
				.Format = GpuFormat::R8G8B8A8_UNORM,
				.Usage = GpuTextureUsage::ShaderResource | GpuTextureUsage::UnorderedAccess,
				.Depth = 1,
				.Dimension = GpuTextureDimension::Tex3D,
			});
			return DefaultRWTexture3D;
		}
		if (BindingTypeValue == BindingType::RWTexture)
		{
			static TRefCountPtr<GpuTexture> DefaultRWTexture = GGpuRhi->CreateTexture({
				.Width = 1,
				.Height = 1,
				.Format = GpuFormat::R8G8B8A8_UNORM,
				.Usage = GpuTextureUsage::ShaderResource | GpuTextureUsage::UnorderedAccess,
			}, GpuResourceState::UnorderedAccess);
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

	GpuTexture* GetConnectedOverrideTexture(GraphPin* OverridePin)
	{
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
		Graph* OwnerGraph = static_cast<Graph*>(Pin->GetOuterMost());
		OwnerGraph->RemoveLink(Pin->GetSourcePin(), Pin);
	}

	GpuTexture* ResolveOverrideTexture(BindingType BindingTypeValue, GraphPin* OverridePin, const ShaderOverrideSlot* MatchingSlot)
	{
		if (OverridePin)
		{
			if (GpuTexture* Tex = GetConnectedOverrideTexture(OverridePin)) return Tex;
		}
		if (MatchingSlot && MatchingSlot->TextureAsset)
		{
			if (GpuTexture* Tex = ResolveTextureAssetGpu(MatchingSlot->TextureAsset.Get())) return Tex;
		}
		return GetDefaultOverrideTexture(BindingTypeValue);
	}

	GraphPin* FindOverridePin(
		const TArray<ShaderOverrideSlot>& Slots,
		const TArray<ObjectPtr<GraphPin>>& Pins,
		const FString& BindingName,
		const FString& MemberName,
		BindingShaderStage Stage)
	{
		for (const ShaderOverrideSlot& Slot : Slots)
		{
			if (Slot.BindingName != BindingName || Slot.MemberName != MemberName || Slot.Stage != Stage)
			{
				continue;
			}
			for (const ObjectPtr<GraphPin>& Pin : Pins)
			{
				if (Pin && Pin->GetGuid() == Slot.PinGuid) return Pin.Get();
			}
		}
		return nullptr;
	}

	void PreserveOverrideSlotData(TArray<ShaderOverrideSlot>& NewSlots, const TArray<ShaderOverrideSlot>& OldSlots)
	{
		for (ShaderOverrideSlot& NewSlot : NewSlots)
		{
			for (const ShaderOverrideSlot& Old : OldSlots)
			{
				if (Old.BindingName != NewSlot.BindingName
					|| Old.MemberName != NewSlot.MemberName
					|| Old.bIsResource != NewSlot.bIsResource
					|| Old.Type != NewSlot.Type)
				{
					continue;
				}
				if (NewSlot.bIsResource)
				{
					NewSlot.TextureAsset = Old.TextureAsset;
				}
				else
				{
					NewSlot.Bytes = Old.Bytes;
				}
				NewSlot.PinGuid = Old.PinGuid;
				break;
			}
		}
	}

	void ReconcileOverridePins(ShObject* Owner, TArray<ShaderOverrideSlot>& Slots, TArray<ObjectPtr<GraphPin>>& Pins)
	{
		auto MakePinLabel = [](const ShaderOverrideSlot& Slot) {
			return FText::FromString(Slot.MemberName.IsEmpty() ? Slot.BindingName : Slot.BindingName + TEXT(".") + Slot.MemberName);
		};

		for (ShaderOverrideSlot& Slot : Slots)
		{
			GraphPin* ExistingPin = nullptr;
			if (Slot.PinGuid.IsValid())
			{
				for (const auto& Pin : Pins)
				{
					if (Pin && Pin->GetGuid() == Slot.PinGuid)
					{
						ExistingPin = Pin.Get();
						break;
					}
				}
			}

			if (!IsOverridePinTypeValid(ExistingPin, Slot.Type, Slot.bIsResource))
			{
				if (ExistingPin)
				{
					BreakOverridePinLink(ExistingPin);
					Pins.RemoveAll([ExistingPin](const ObjectPtr<GraphPin>& P) { return P.Get() == ExistingPin; });
				}

				ObjectPtr<GraphPin> NewPin = CreateOverridePinForType(Owner, Slot.Type, Slot.bIsResource);
				if (!NewPin) continue;
				NewPin->Direction = PinDirection::Input;
				NewPin->ObjectName = MakePinLabel(Slot);
				if (auto* BytesPinValue = DynamicCast<BytesPin>(NewPin.Get()))
				{
					if (!Slot.Bytes.IsEmpty()) BytesPinValue->SetBytes(Slot.Bytes);
				}
				Slot.PinGuid = NewPin->GetGuid();
				Pins.Add(MoveTemp(NewPin));
			}
			else
			{
				ExistingPin->Direction = PinDirection::Input;
				ExistingPin->ObjectName = MakePinLabel(Slot);
				if (auto* BytesPinValue = DynamicCast<BytesPin>(ExistingPin))
				{
					if (!BytesPinValue->SourcePin.IsValid() && !Slot.Bytes.IsEmpty())
					{
						BytesPinValue->SetBytes(Slot.Bytes);
					}
				}
			}
		}

		Pins.RemoveAll([&Slots](const ObjectPtr<GraphPin>& Pin) {
			const bool bOrphan = !Pin || !Slots.ContainsByPredicate([&](const ShaderOverrideSlot& Slot) {
				return Slot.PinGuid.IsValid() && Pin->GetGuid() == Slot.PinGuid;
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
}
