#pragma once
#include "AssetManager/AssetManager.h"
#include "GpuApi/GpuBindGroupLayout.h"
#include "GpuApi/GpuBuffer.h"
#include "GpuApi/GpuSampler.h"
#include "GpuApi/GpuTexture.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"

namespace FW
{
	class GraphPin;
	struct MetaType;
	class PropertyData;
	class PropertyItemBase;
}

namespace SH
{
	enum class MaterialBindingValueSource : uint8
	{
		Custom,
		BuiltIn,
	};

	struct ShaderResourceBindingState
	{
		FW::BindingType BindingType = FW::BindingType::Texture;

		FW::AssetPtr<FW::AssetObject> TextureAsset;

		FW::SamplerFilter Filter = FW::SamplerFilter::Bilinear;
		FW::SamplerAddressMode AddressMode = FW::SamplerAddressMode::Wrap;

		FW::Vector3i DefaultRWSize = FW::Vector3i{ 1, 1, 1 };
		TRefCountPtr<FW::GpuTexture> DefaultRWTexture;

		uint32 BufferByteSize = 1;
		uint32 StructuredStride = 1;
		TRefCountPtr<FW::GpuBuffer> Buffer;

		FW::GpuFormat Format = FW::GpuFormat::R32_FLOAT;

		// Pass-internally-owned output texture for RW resource bindings.
		TRefCountPtr<FW::GpuTexture> RWOutputTexture;

		friend FArchive& operator<<(FArchive& Ar, ShaderResourceBindingState& State);
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
	struct ShaderOverrideSlot : ShaderResourceBindingState
	{
		ShaderOverrideKey Key;
		FString Type;
		bool bIsResource = false;
		FW::ObserverObjectPtr<FW::GraphPin> InputPin;
		FW::ObserverObjectPtr<FW::GraphPin> OutputPin; // RW resource result output pin; same label as InputPin
		TArray<uint8> Bytes;

		MaterialBindingValueSource ValueSource = MaterialBindingValueSource::Custom;
		uint8 MatrixBuiltInRaw = 0;
		uint8 FloatBuiltInRaw = 0;
		uint8 Vector2BuiltInRaw = 0;
		uint8 Vector3BuiltInRaw = 0;

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
	bool IsSamplerResourceBinding(FW::BindingType BindingTypeValue);
	bool IsBufferBinding(FW::BindingType BindingTypeValue);
	bool IsRWBufferBinding(FW::BindingType BindingTypeValue);
	bool IsStructuredBufferBinding(FW::BindingType BindingTypeValue);
	bool IsTypedBufferBinding(FW::BindingType BindingTypeValue);
	bool IsBufferType(const FString& Type);
	bool IsStructuredBufferType(const FString& Type);
	bool IsTypedBufferType(const FString& Type);
	bool IsRWBufferType(const FString& Type);
	bool IsRWStructuredBufferType(const FString& Type);
	uint32 GetMinBufferByteSize(FW::BindingType BindingTypeValue, uint32 StructuredStride, FW::GpuFormat Format = FW::GpuFormat::R32_FLOAT);

	void CopyShaderResourceBindingState(ShaderResourceBindingState& Dst, const ShaderResourceBindingState& Src);
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
	bool IsDefaultBufferProperty(const TArray<ShaderOverrideSlot>& Slots, FW::PropertyData* InProperty);
	bool IsSamplerResourceProperty(const TArray<ShaderOverrideSlot>& Slots, FW::PropertyData* InProperty);

	FW::GpuTexture* ResolveOverrideTexture(FW::BindingType BindingTypeValue, FW::GraphPin* InputPin, ShaderResourceBindingState* MatchingResource, FW::Vector2f ViewportSize = FW::Vector2f{0, 0});

	// Publishes each slot's RW resource (Slot.RWOutputTexture / Slot.Buffer) to its connected output pin.
	void PublishRWOutputsToPins(TArray<ShaderOverrideSlot>& Slots);

	// Returns Slot.RWOutputTexture, (re)creating it to match SrcTex's size/format when stale.
	FW::GpuTexture* EnsureRWOutputTexture(ShaderOverrideSlot& Slot, FW::BindingType BindingTypeValue, FW::GpuTexture* SrcTex, const FString& DebugName);
	FW::GpuSampler* ResolveResourceSampler(const ShaderResourceBindingState* ResourceState);
	void ResolveDefaultBuffer(uint32& BufferByteSize, uint32 StructuredStride, FW::GpuFormat Format, FW::BindingType BindingTypeValue, TRefCountPtr<FW::GpuBuffer>& InOutBuffer);

	ShaderOverrideSlot* FindOverrideSlot(TArray<ShaderOverrideSlot>& Slots, const ShaderOverrideKey& Key);
	const ShaderOverrideSlot* FindOverrideSlot(const TArray<ShaderOverrideSlot>& Slots, const ShaderOverrideKey& Key);
	FW::GraphPin* FindOverrideInputPin(const TArray<ShaderOverrideSlot>& Slots, const ShaderOverrideKey& Key);

	void PreserveOverrideSlotData(TArray<ShaderOverrideSlot>& NewSlots, const TArray<ShaderOverrideSlot>& OldSlots);

	// Reconcile owner's pin list against current slot list:
	//   - drop pins with no matching slot
	//   - create new pins for slots that need them
	//   - update direction/label
	void ReconcileOverridePins(FW::ShObject* Owner, TArray<ShaderOverrideSlot>& Slots, TArray<FW::ObjectPtr<FW::GraphPin>>& Pins);

	struct OverrideBuiltInFactories
	{
		using ItemFactory = TFunction<TSharedRef<FW::PropertyItemBase>(
			FW::ShObject* Owner, FText Label, ShaderOverrideSlot& Slot)>;

		ItemFactory MakeMatrixBuiltInItem;
		ItemFactory MakeFloatBuiltInItem;
		ItemFactory MakeVector2BuiltInItem;
		ItemFactory MakeVector3BuiltInItem;
	};

	template<typename EnumType>
	OverrideBuiltInFactories::ItemFactory MakeOverrideBuiltInFactory(uint8 ShaderOverrideSlot::* RawField)
	{
		return [RawField](FW::ShObject* Owner, FText Label, ShaderOverrideSlot& Slot) -> TSharedRef<FW::PropertyItemBase> {
			return FW::MakePropertyEnumItem<EnumType>(
				Owner, Label, static_cast<EnumType>(Slot.*RawField),
				[&Slot, RawField](EnumType v) {
					Slot.*RawField = static_cast<uint8>(v);
				});
		};
	}

	void AttachValueSourceSwitchMenu(
		const TSharedRef<FW::PropertyItemBase>& Item,
		FW::ShObject* Owner,
		MaterialBindingValueSource& ValueSource,
		bool bShowCustomEntry = true,
		bool bShowBuiltInEntry = true);

	// Build a property item for a scalar/vector/matrix override slot.
	TSharedRef<FW::PropertyItemBase> MakeBytesPropertyItem(FW::ShObject* Owner, FText Label, TArray<ShaderOverrideSlot>& Slots, ShaderOverrideSlot& Slot, const OverrideBuiltInFactories& Factories = {});
	void AppendDefaultRWSizeChildren(FW::ShObject* Owner, FW::PropertyItemBase& Parent, ShaderResourceBindingState& ResourceState, const FString& Type);
	void AppendDefaultRWSizeChildren(FW::ShObject* Owner, FW::PropertyItemBase& Parent, ShaderOverrideSlot& Slot);
	void AppendSamplerPropertyChildren(FW::ShObject* Owner, FW::PropertyItemBase& Parent, ShaderResourceBindingState& ResourceState);
	TSharedRef<FW::PropertyItemBase> MakeSamplerPropertyItem(FW::ShObject* Owner, FText Label, ShaderResourceBindingState& ResourceState);
	TSharedRef<FW::PropertyItemBase> MakeTextureResourcePropertyItem(FW::ShObject* Owner, FText Label, ShaderResourceBindingState& ResourceState, const FString& Type, bool bIncludeSamplerSettings);
	TSharedRef<FW::PropertyItemBase> MakeOverrideSlotPropertyItem(FW::ShObject* Owner, TArray<ShaderOverrideSlot>& Slots, ShaderOverrideSlot& Slot, const OverrideBuiltInFactories& Factories = {});
	// Build a property item for StructuredBuffer/RWStructuredBuffer/RawBuffer/RWRawBuffer bindings.
	TSharedRef<FW::PropertyItemBase> MakeBufferPropertyItem(FW::ShObject* Owner, FText Label, FW::BindingType BindingTypeValue, uint32 StructuredStride, uint32& BufferByteSize, FW::GpuFormat& Format);
	TSharedRef<FW::PropertyItemBase> MakeBufferPropertyItem(FW::ShObject* Owner, FText Label, ShaderOverrideSlot& Slot);
}
