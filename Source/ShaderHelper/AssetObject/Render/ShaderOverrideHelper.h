#pragma once
#include "AssetManager/AssetManager.h"
#include "GpuApi/GpuBindGroupLayout.h"

namespace FW
{
	class GraphPin;
	class GpuTexture;
	struct MetaType;
	class PropertyItemBase;
}

namespace SH
{
	// Metadata for a single shader-binding override exposed as a row pin.
	struct ShaderOverrideSlot
	{
		FString BindingName;
		FString MemberName;
		FString Type;
		FW::BindingShaderStage Stage = FW::BindingShaderStage::All;
		bool bIsResource = false;
		FGuid PinGuid;
		TArray<uint8> Bytes;
		FW::AssetPtr<FW::AssetObject> TextureAsset;

		friend FArchive& operator<<(FArchive& Ar, ShaderOverrideSlot& S)
		{
			Ar << S.BindingName << S.MemberName << S.Type << S.Stage << S.bIsResource << S.PinGuid << S.Bytes << S.TextureAsset;
			return Ar;
		}
	};

	// Byte-storage helpers for ShaderOverrideSlot scalar/vector/matrix UB members.
	int32 GetOverrideByteSize(const FString& Type);
	const uint8* GetCompleteOverrideBytes(const TArray<uint8>& Bytes, const FString& Type);
	void EnsureOverrideBytesStorage(ShaderOverrideSlot& Slot);

	template<typename ValueType>
	ValueType* GetOverrideBytesAs(ShaderOverrideSlot& Slot)
	{
		EnsureOverrideBytesStorage(Slot);
		return reinterpret_cast<ValueType*>(Slot.Bytes.GetData());
	}

	// any opaque resource binding (textures + sampler + combined).
	bool IsResourceOverrideBinding(FW::BindingType BindingTypeValue);
	// resource bindings that surface as override pins (excludes Sampler).
	bool IsPinnableResourceBinding(FW::BindingType BindingTypeValue);

	FString GetResourceOverrideType(FW::BindingType BindingTypeValue);
	FW::ObjectPtr<FW::GraphPin> CreateOverridePinForType(FW::ShObject* Outer, const FString& Type, bool bIsResource);
	bool IsOverridePinTypeValid(FW::GraphPin* Pin, const FString& Type, bool bIsResource);
	FW::GpuTexture* GetDefaultOverrideTexture(FW::BindingType BindingTypeValue);
	FW::MetaType* GetTextureMetaType(const FString& Type);
	FW::GpuTexture* GetConnectedOverrideTexture(FW::GraphPin* OverridePin);
	void BreakOverridePinLink(FW::GraphPin* Pin);

	FW::GpuTexture* ResolveOverrideTexture(FW::BindingType BindingTypeValue, FW::GraphPin* OverridePin, const ShaderOverrideSlot* MatchingSlot);

	// Look up the override pin for a (BindingName, MemberName, Stage) tuple by walking Slots
	FW::GraphPin* FindOverridePin(
		const TArray<ShaderOverrideSlot>& Slots,
		const TArray<FW::ObjectPtr<FW::GraphPin>>& Pins,
		const FString& BindingName,
		const FString& MemberName,
		FW::BindingShaderStage Stage);

	void PreserveOverrideSlotData(TArray<ShaderOverrideSlot>& NewSlots, const TArray<ShaderOverrideSlot>& OldSlots);

	// Reconcile owner's pin list against current slot list:
	//   - drop pins with no matching slot
	//   - create new pins for slots that need them
	//   - update direction/label
	void ReconcileOverridePins(FW::ShObject* Owner, TArray<ShaderOverrideSlot>& Slots, TArray<FW::ObjectPtr<FW::GraphPin>>& Pins);

	// Build a property item for a scalar/vector/matrix override slot.
	TSharedRef<FW::PropertyItemBase> MakeBytesPropertyItem(FW::ShObject* Owner, FText Label, ShaderOverrideSlot& Slot);
}
