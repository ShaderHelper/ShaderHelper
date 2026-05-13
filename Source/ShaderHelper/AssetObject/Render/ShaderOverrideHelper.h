#pragma once
#include "AssetManager/AssetManager.h"
#include "GpuApi/GpuBindGroupLayout.h"
#include "GpuApi/GpuTexture.h"

namespace FW
{
	class GraphPin;
	struct MetaType;
	class PropertyData;
	class PropertyItemBase;
}

namespace SH
{

	enum class RWTextureFormat
	{
		R8_UNORM,
		R8G8B8A8_UNORM,
		R16_UINT,
		R32_UINT,
		R16G16B16A16_UINT,
		R32G32_UINT,
		R32G32B32A32_UINT,
		R16_FLOAT,
		R32_FLOAT,
		R32G32_FLOAT,
		R16G16B16A16_FLOAT,
		R32G32B32A32_FLOAT,
		R16G16B16A16_UNORM,
	};

	struct ShaderOverrideKey
	{
		FString BindingName;
		FString MemberName;
		FW::BindingShaderStage Stage = FW::BindingShaderStage::All;

		bool operator==(const ShaderOverrideKey& Other) const
		{
			return BindingName == Other.BindingName && MemberName == Other.MemberName && Stage == Other.Stage;
		}

		friend FArchive& operator<<(FArchive& Ar, ShaderOverrideKey& Key);
	};

	// Metadata for a single shader-binding override exposed as a row pin.
	struct ShaderOverrideSlot
	{
		ShaderOverrideKey Key;
		FString Type;
		bool bIsResource = false;
		FW::ObserverObjectPtr<FW::GraphPin> InputPin;
		FW::ObserverObjectPtr<FW::GraphPin> OutputPin; // RW resource result output pin; same label as InputPin
		TArray<uint8> Bytes;
		FW::AssetPtr<FW::AssetObject> TextureAsset;

		// User-editable defaults used to create a default RW texture when no
		// TextureAsset is assigned and the input override pin is not connected.
		FW::Vector3i DefaultRWSize = FW::Vector3i{ 1, 1, 1 };
		RWTextureFormat DefaultRWFormat = RWTextureFormat::R8G8B8A8_UNORM;
		TRefCountPtr<FW::GpuTexture> DefaultRWTexture;

		friend FArchive& operator<<(FArchive& Ar, ShaderOverrideSlot& S);
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
	FString MakeOverrideKeyLabel(const ShaderOverrideKey& Key);
	FString MakeOverrideSlotLabel(const ShaderOverrideSlot& Slot);
	// True for shader resource type strings that represent RW (UAV) resources (e.g. RWTexture2D, RWTexture3D).
	bool IsRWResourceType(const FString& Type);
	FW::ObjectPtr<FW::GraphPin> CreateOverridePinForType(FW::ShObject* Outer, const FString& Type, bool bIsResource);
	bool IsOverridePinTypeValid(FW::GraphPin* Pin, const FString& Type, bool bIsResource);
	FW::GpuTexture* GetDefaultOverrideTexture(FW::BindingType BindingTypeValue);
	FW::MetaType* GetTextureMetaType(const FString& Type);
	FW::GpuTexture* GetConnectedOverrideTexture(FW::GraphPin* InputPin);
	void BreakOverridePinLink(FW::GraphPin* Pin);
	bool IsDefaultRWTextureProperty(const TArray<ShaderOverrideSlot>& Slots, FW::PropertyData* InProperty);

	FW::GpuTexture* ResolveOverrideTexture(FW::BindingType BindingTypeValue, FW::GraphPin* InputPin, ShaderOverrideSlot* MatchingSlot, FW::Vector2f ViewportSize = FW::Vector2f{0, 0});

	ShaderOverrideSlot* FindOverrideSlot(TArray<ShaderOverrideSlot>& Slots, const ShaderOverrideKey& Key);
	const ShaderOverrideSlot* FindOverrideSlot(const TArray<ShaderOverrideSlot>& Slots, const ShaderOverrideKey& Key);
	FW::GraphPin* FindOverrideInputPin(const TArray<ShaderOverrideSlot>& Slots, const ShaderOverrideKey& Key);

	void PreserveOverrideSlotData(TArray<ShaderOverrideSlot>& NewSlots, const TArray<ShaderOverrideSlot>& OldSlots);

	// Reconcile owner's pin list against current slot list:
	//   - drop pins with no matching slot
	//   - create new pins for slots that need them
	//   - update direction/label
	void ReconcileOverridePins(FW::ShObject* Owner, TArray<ShaderOverrideSlot>& Slots, TArray<FW::ObjectPtr<FW::GraphPin>>& Pins);

	// Build a property item for a scalar/vector/matrix override slot.
	TSharedRef<FW::PropertyItemBase> MakeBytesPropertyItem(FW::ShObject* Owner, FText Label, ShaderOverrideSlot& Slot);
	void AppendDefaultRWSizeChildren(FW::ShObject* Owner, FW::PropertyItemBase& Parent, ShaderOverrideSlot& Slot);
}
