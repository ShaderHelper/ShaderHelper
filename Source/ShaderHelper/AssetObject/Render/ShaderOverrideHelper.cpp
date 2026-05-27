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
#include "UI/Styles/FAppCommonStyle.h"
#include "UI/Widgets/Property/PropertyData/PropertyAssetItem.h"
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

	static GpuFormat ToGpuFormat(TypedBufferFormat Format)
	{
		switch (Format)
		{
		case TypedBufferFormat::R32_FLOAT:             return GpuFormat::R32_FLOAT;
		case TypedBufferFormat::R32_UINT:              return GpuFormat::R32_UINT;
		case TypedBufferFormat::R16G16B16A16_UINT:     return GpuFormat::R16G16B16A16_UINT;
		case TypedBufferFormat::R32G32B32A32_UINT:     return GpuFormat::R32G32B32A32_UINT;
		}
		AUX::Unreachable();
	}

	FArchive& operator<<(FArchive& Ar, ShaderOverrideKey& Key)
	{
		Ar << Key.BindingName << Key.MemberName << Key.Stage;
		return Ar;
	}

	FArchive& operator<<(FArchive& Ar, ShaderResourceBindingState& State)
	{
		Ar << State.BindingType << State.TextureAsset;
		Ar << State.Filter << State.AddressMode;
		Ar << State.DefaultRWSize << State.DefaultRWFormat;
		Ar << State.BufferByteSize << State.StructuredStride << State.BufferFormat;
		return Ar;
	}

	FArchive& operator<<(FArchive& Ar, ShaderOverrideSlot& S)
	{
		Ar << S.Key << S.Type << S.bIsResource << S.InputPin << S.Bytes << S.OutputPin;
		Ar << static_cast<ShaderResourceBindingState&>(S);
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
		case BindingType::StructuredBuffer:
		case BindingType::RWStructuredBuffer:
		case BindingType::TypedBuffer:
		case BindingType::RWTypedBuffer:
		case BindingType::RawBuffer:
		case BindingType::RWRawBuffer:
			return true;
		default:
			return false;
		}
	}

	bool IsPinnableResourceBinding(BindingType BindingTypeValue)
	{
		return IsResourceOverrideBinding(BindingTypeValue) && BindingTypeValue != BindingType::Sampler;
	}

	bool IsSamplerResourceBinding(BindingType BindingTypeValue)
	{
		return BindingTypeValue == BindingType::Sampler
			|| BindingTypeValue == BindingType::CombinedTextureSampler
			|| BindingTypeValue == BindingType::CombinedTextureCubeSampler
			|| BindingTypeValue == BindingType::CombinedTexture3DSampler;
	}

	bool IsBufferBinding(BindingType BindingTypeValue)
	{
		return IsStructuredBufferBinding(BindingTypeValue)
			|| IsTypedBufferBinding(BindingTypeValue)
			|| BindingTypeValue == BindingType::RawBuffer
			|| BindingTypeValue == BindingType::RWRawBuffer;
	}

	bool IsRWBufferBinding(BindingType BindingTypeValue)
	{
		return BindingTypeValue == BindingType::RWStructuredBuffer
			|| BindingTypeValue == BindingType::RWRawBuffer
			|| BindingTypeValue == BindingType::RWTypedBuffer;
	}

	bool IsStructuredBufferBinding(BindingType BindingTypeValue)
	{
		return BindingTypeValue == BindingType::StructuredBuffer || BindingTypeValue == BindingType::RWStructuredBuffer;
	}

	bool IsTypedBufferBinding(BindingType BindingTypeValue)
	{
		return BindingTypeValue == BindingType::TypedBuffer || BindingTypeValue == BindingType::RWTypedBuffer;
	}

	bool IsBufferType(const FString& Type)
	{
		return IsStructuredBufferType(Type) || IsTypedBufferType(Type) || Type == TEXT("RawBuffer") || Type == TEXT("RWRawBuffer");
	}

	bool IsStructuredBufferType(const FString& Type)
	{
		return Type == TEXT("StructuredBuffer") || Type == TEXT("RWStructuredBuffer");
	}

	bool IsTypedBufferType(const FString& Type)
	{
		return Type == TEXT("TypedBuffer") || Type == TEXT("RWTypedBuffer");
	}

	bool IsRWBufferType(const FString& Type)
	{
		return Type == TEXT("RWStructuredBuffer") || Type == TEXT("RWRawBuffer") || Type == TEXT("RWTypedBuffer");
	}

	bool IsRWStructuredBufferType(const FString& Type)
	{
		return Type == TEXT("RWStructuredBuffer");
	}

	uint32 GetMinBufferByteSize(BindingType BindingTypeValue, uint32 StructuredStride, TypedBufferFormat BufferFormat)
	{
		if (IsStructuredBufferBinding(BindingTypeValue))
			return FMath::Max(1u, StructuredStride);
		if (IsTypedBufferBinding(BindingTypeValue))
			return FMath::Max(1u, GetFormatByteSize(ToGpuFormat(BufferFormat)));
		return 1u;
	}

	void CopyShaderResourceBindingState(ShaderResourceBindingState& Dst, const ShaderResourceBindingState& Src)
	{
		Dst.BindingType = Src.BindingType;
		Dst.TextureAsset = Src.TextureAsset;
		Dst.Filter = Src.Filter;
		Dst.AddressMode = Src.AddressMode;
		Dst.DefaultRWSize = Src.DefaultRWSize;
		Dst.DefaultRWFormat = Src.DefaultRWFormat;
		Dst.BufferByteSize = Src.BufferByteSize;
		Dst.BufferFormat = Src.BufferFormat;
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
		case BindingType::StructuredBuffer:
			return TEXT("StructuredBuffer");
		case BindingType::RWStructuredBuffer:
			return TEXT("RWStructuredBuffer");
		case BindingType::RawBuffer:
			return TEXT("RawBuffer");
		case BindingType::RWRawBuffer:
			return TEXT("RWRawBuffer");
		case BindingType::TypedBuffer:
			return TEXT("TypedBuffer");
		case BindingType::RWTypedBuffer:
			return TEXT("RWTypedBuffer");
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
			if (IsBufferType(Type)) return NewShObject<BytesPin>(Outer);
			if (Type.Contains(TEXT("Cube"))) return NewShObject<GpuCubemapPin>(Outer);
			if (Type.Contains(TEXT("3D"))) return NewShObject<GpuTexture3DPin>(Outer);
			return NewShObject<GpuTexturePin>(Outer);
		}
		return NewShObject<BytesPin>(Outer);
	}

	bool IsOverridePinTypeValid(GraphPin* Pin, const FString& Type, bool bIsResource)
	{
		if (!Pin) return false;
		if (!bIsResource || IsBufferType(Type)) return DynamicCast<BytesPin>(Pin) != nullptr;
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

	bool IsDefaultBufferProperty(const TArray<ShaderOverrideSlot>& Slots, PropertyData* InProperty)
	{
		if (!InProperty->GetDisplayName().EqualTo(LOCALIZATION("ByteSize")))
		{
			return false;
		}

		for (PropertyData* Parent = InProperty->GetParent(); Parent; Parent = Parent->GetParent())
		{
			const FString ParentName = Parent->GetDisplayName().ToString();
			if (Slots.ContainsByPredicate([&](const ShaderOverrideSlot& Slot) {
				return Slot.bIsResource && IsRWBufferType(Slot.Type) && MakeOverrideSlotLabel(Slot) == ParentName;
			}))
			{
				return true;
			}
		}
		return false;
	}

	bool IsSamplerResourceProperty(const TArray<ShaderOverrideSlot>& Slots, PropertyData* InProperty)
	{
		const FText DisplayName = InProperty->GetDisplayName();
		const bool bSamplerChild = DisplayName.EqualTo(LOCALIZATION("FilterMode"))
			|| DisplayName.EqualTo(LOCALIZATION("WrapMode"));
		if (!bSamplerChild)
		{
			return false;
		}

		for (PropertyData* Parent = InProperty->GetParent(); Parent; Parent = Parent->GetParent())
		{
			const FString ParentName = Parent->GetDisplayName().ToString();
			if (Slots.ContainsByPredicate([&](const ShaderOverrideSlot& Slot) {
				return Slot.bIsResource && IsSamplerResourceBinding(Slot.BindingType) && MakeOverrideSlotLabel(Slot) == ParentName;
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

	GpuTexture* ResolveOverrideTexture(BindingType BindingTypeValue, GraphPin* InputPin, ShaderResourceBindingState* MatchingResource, Vector2f ViewportSize)
	{
		if (InputPin)
		{
			if (GpuTexture* Tex = GetConnectedOverrideTexture(InputPin)) return Tex;
		}
		if (MatchingResource && MatchingResource->TextureAsset)
		{
			if (GpuTexture* Tex = ResolveTextureAssetGpu(MatchingResource->TextureAsset.Get())) return Tex;
		}
		// For RW resources with no asset and no incoming connection
		if (MatchingResource && (BindingTypeValue == BindingType::RWTexture || BindingTypeValue == BindingType::RWTexture3D))
		{
			const bool bIs3D = BindingTypeValue == BindingType::RWTexture3D;
			auto ResolveExtent = [](int32 SlotValue, float ViewportValue) -> uint32 {
				if (SlotValue <= 0 && ViewportValue > 0) return static_cast<uint32>(ViewportValue);
				return static_cast<uint32>(FMath::Max(1, SlotValue));
			};
			const uint32 W = ResolveExtent(MatchingResource->DefaultRWSize.x, ViewportSize.X);
			const uint32 H = ResolveExtent(MatchingResource->DefaultRWSize.y, ViewportSize.Y);
			const uint32 D = static_cast<uint32>(FMath::Max(1, MatchingResource->DefaultRWSize.z));
			const GpuFormat Fmt = ToGpuFormat(MatchingResource->DefaultRWFormat);
			GpuTexture* Existing = MatchingResource->DefaultRWTexture.GetReference();
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
				MatchingResource->DefaultRWTexture = GGpuRhi->CreateTexture(Desc, GpuResourceState::UnorderedAccess);
			}
			return MatchingResource->DefaultRWTexture.GetReference();
		}
		return GetDefaultOverrideTexture(BindingTypeValue);
	}

	GpuSampler* ResolveResourceSampler(const ShaderResourceBindingState* ResourceState)
	{
		GpuSamplerDesc Desc;
		if (ResourceState)
		{
			Desc.Filter = ResourceState->Filter;
			Desc.AddressU = ResourceState->AddressMode;
			Desc.AddressV = ResourceState->AddressMode;
			Desc.AddressW = ResourceState->AddressMode;
		}
		return GpuResourceHelper::GetSampler(Desc);
	}

	void ResolveDefaultBuffer(uint32& BufferByteSize, uint32 StructuredStride, TypedBufferFormat BufferFormat, BindingType BindingTypeValue, TRefCountPtr<GpuBuffer>& InOutBuffer)
	{
		check(IsBufferBinding(BindingTypeValue));

		BufferByteSize = FMath::Max(BufferByteSize, GetMinBufferByteSize(BindingTypeValue, StructuredStride, BufferFormat));

		GpuBufferUsage Usage = GpuBufferUsage::Structured;
		switch (BindingTypeValue)
		{
		case BindingType::StructuredBuffer:
			Usage = GpuBufferUsage::Structured;
			break;
		case BindingType::RWStructuredBuffer:
			Usage = GpuBufferUsage::RWStructured;
			break;
		case BindingType::RawBuffer:
			Usage = GpuBufferUsage::Raw;
			break;
		case BindingType::RWRawBuffer:
			Usage = GpuBufferUsage::RWRaw;
			break;
		case BindingType::TypedBuffer:
			Usage = GpuBufferUsage::Typed;
			break;
		case BindingType::RWTypedBuffer:
			Usage = GpuBufferUsage::RWTyped;
			break;
		default:
			AUX::Unreachable();
		}
		const GpuFormat TypedFormat = ToGpuFormat(BufferFormat);
		const bool bMatches = InOutBuffer.IsValid()
			&& InOutBuffer->GetUsage() == Usage
			&& InOutBuffer->GetByteSize() == BufferByteSize
			&& (!IsStructuredBufferBinding(BindingTypeValue) || InOutBuffer->GetStructuredStride() == StructuredStride)
			&& (!IsTypedBufferBinding(BindingTypeValue) || InOutBuffer->GetTypedFormat() == TypedFormat);
		if (!bMatches)
		{
			TArray<uint8> ZeroData;
			ZeroData.SetNumZeroed(static_cast<int32>(BufferByteSize));
			GpuBufferDesc Desc{
				.ByteSize = BufferByteSize,
				.Usage = Usage,
				.InitialData = ZeroData,
			};
			if (IsStructuredBufferBinding(BindingTypeValue))
			{
				Desc.StructuredInit.Stride = StructuredStride;
			}
			else if (IsTypedBufferBinding(BindingTypeValue))
			{
				Desc.TypedInit.Format = TypedFormat;
			}
			InOutBuffer = GGpuRhi->CreateBuffer(Desc, GetBufferState(Usage));
		}
	}

	GpuTexture* EnsureRWOutputTexture(ShaderOverrideSlot& Slot, BindingType BindingTypeValue, GpuTexture* SrcTex, const FString& DebugName)
	{
		if (!SrcTex) return nullptr;
		const uint32 Width = SrcTex->GetWidth();
		const uint32 Height = SrcTex->GetHeight();
		const uint32 Depth = SrcTex->GetDepth();
		const GpuFormat Format = SrcTex->GetFormat();
		const bool bIs3D = BindingTypeValue == BindingType::RWTexture3D;

		GpuTexture* Existing = Slot.RWOutputTexture.GetReference();
		if (Existing
			&& Existing->GetWidth() == Width
			&& Existing->GetHeight() == Height
			&& Existing->GetFormat() == Format
			&& (!bIs3D || Existing->GetDepth() == Depth))
		{
			return Existing;
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
		Slot.RWOutputTexture = GGpuRhi->CreateTexture(Desc, GpuResourceState::UnorderedAccess);
		if (!DebugName.IsEmpty())
		{
			GGpuRhi->SetResourceName(TCHAR_TO_ANSI(*DebugName), Slot.RWOutputTexture);
		}
		return Slot.RWOutputTexture.GetReference();
	}

	void PublishRWOutputsToPins(TArray<ShaderOverrideSlot>& Slots)
	{
		for (ShaderOverrideSlot& Slot : Slots)
		{
			if (!Slot.bIsResource || !IsRWResourceType(Slot.Type)) continue;
			GraphPin* OutputPin = Slot.OutputPin.IsValid() ? Slot.OutputPin.Get() : nullptr;
			if (!OutputPin) continue;

			if (IsBufferType(Slot.Type))
			{
				auto* BytesPinValue = DynamicCast<BytesPin>(OutputPin);
				if (!BytesPinValue) continue;
				if (BytesPinValue->GetTargetPins().Num() == 0) continue;

				GpuBuffer* Buffer = Slot.Buffer.GetReference();
				const uint32 ByteSize = Buffer->GetByteSize();
				const uint8* Mapped = (const uint8*)GGpuRhi->MapGpuBuffer(Buffer, GpuResourceMapMode::Read_Only);
				TArray<uint8> Out;
				Out.SetNumUninitialized(static_cast<int32>(ByteSize));
				FMemory::Memcpy(Out.GetData(), Mapped, ByteSize);
				GGpuRhi->UnMapGpuBuffer(Buffer);
				BytesPinValue->SetBytes(MoveTemp(Out));
				continue;
			}

			GpuTexture* OutTex = Slot.RWOutputTexture.GetReference();
			if (!OutTex) continue;
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
					|| Old.Type != NewSlot.Type || Old.BindingType != NewSlot.BindingType)
				{
					continue;
				}
				if (NewSlot.bIsResource)
				{
					CopyShaderResourceBindingState(NewSlot, Old);
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

	void AppendDefaultRWSizeChildren(ShObject* Owner, PropertyItemBase& Parent, ShaderResourceBindingState& ResourceState, const FString& Type)
	{
		const bool bIs3D = Type.Contains(TEXT("3D"));

		auto WidthItem = MakeShared<PropertyScalarItem<int32>>(Owner, LOCALIZATION("WidthAuto"), &ResourceState.DefaultRWSize.x);
		WidthItem->SetMinValue(0);
		Parent.AddChild(WidthItem);

		auto HeightItem = MakeShared<PropertyScalarItem<int32>>(Owner, LOCALIZATION("HeightAuto"), &ResourceState.DefaultRWSize.y);
		HeightItem->SetMinValue(0);
		Parent.AddChild(HeightItem);

		if (bIs3D)
		{
			auto DepthItem = MakeShared<PropertyScalarItem<int32>>(Owner, LOCALIZATION("Depth"), &ResourceState.DefaultRWSize.z);
			DepthItem->SetMinValue(1);
			Parent.AddChild(DepthItem);
		}

		auto FormatItem = MakePropertyEnumItem<RWTextureFormat>(
			Owner, LOCALIZATION("Format"), ResourceState.DefaultRWFormat,
			[&ResourceState](RWTextureFormat NewValue) { ResourceState.DefaultRWFormat = NewValue; }
		);
		Parent.AddChild(FormatItem);
	}

	void AppendDefaultRWSizeChildren(ShObject* Owner, PropertyItemBase& Parent, ShaderOverrideSlot& Slot)
	{
		AppendDefaultRWSizeChildren(Owner, Parent, Slot, Slot.Type);
	}

	void AppendSamplerPropertyChildren(ShObject* Owner, PropertyItemBase& Parent, ShaderResourceBindingState& ResourceState)
	{
		auto FilterItem = MakePropertyEnumItem<SamplerFilter>(
			Owner, LOCALIZATION("FilterMode"), ResourceState.Filter,
			[&ResourceState](SamplerFilter NewFilter) { ResourceState.Filter = NewFilter; }
		);
		Parent.AddChild(FilterItem);

		auto AddrItem = MakePropertyEnumItem<SamplerAddressMode>(
			Owner, LOCALIZATION("WrapMode"), ResourceState.AddressMode,
			[&ResourceState](SamplerAddressMode NewMode) { ResourceState.AddressMode = NewMode; }
		);
		Parent.AddChild(AddrItem);
	}

	TSharedRef<PropertyItemBase> MakeSamplerPropertyItem(ShObject* Owner, FText Label, ShaderResourceBindingState& ResourceState)
	{
		auto Item = MakeShared<PropertyItemBase>(Owner, Label);
		AppendSamplerPropertyChildren(Owner, *Item, ResourceState);
		return Item;
	}

	TSharedRef<PropertyItemBase> MakeTextureResourcePropertyItem(ShObject* Owner, FText Label, ShaderResourceBindingState& ResourceState, const FString& Type, bool bIncludeSamplerSettings)
	{
		auto AssetItem = MakeShared<PropertyAssetItem>(Owner, Label, GetTextureMetaType(Type), &ResourceState.TextureAsset);
		if (IsRWResourceType(Type) && !ResourceState.TextureAsset)
		{
			AppendDefaultRWSizeChildren(Owner, *AssetItem, ResourceState, Type);
		}
		if (bIncludeSamplerSettings)
		{
			AppendSamplerPropertyChildren(Owner, *AssetItem, ResourceState);
		}
		return AssetItem;
	}

	TSharedRef<PropertyItemBase> MakeOverrideSlotPropertyItem(ShObject* Owner, const TArray<ShaderOverrideSlot>& Slots, ShaderOverrideSlot& Slot)
	{
		const FText Label = FText::FromString(MakeOverrideSlotLabel(Slot));
		GraphPin* OverridePin = FindOverrideInputPin(Slots, Slot.Key);
		if (OverridePin && OverridePin->SourcePin.IsValid())
		{
			auto Item = MakeShared<PropertyItemBase>(Owner, Label);
			Item->SetEmbedWidget(
				SNew(STextBlock)
				.TextStyle(&FAppCommonStyle::Get().GetWidgetStyle<FTextBlockStyle>("MinorText"))
				.Text(FText::FromString(Slot.Type))
			);
			if (Slot.bIsResource && IsSamplerResourceBinding(Slot.BindingType))
			{
				AppendSamplerPropertyChildren(Owner, *Item, Slot);
			}
			return Item;
		}

		if (!Slot.bIsResource)
		{
			return MakeBytesPropertyItem(Owner, Label, Slot);
		}

		if (Slot.BindingType == BindingType::Sampler)
		{
			return MakeSamplerPropertyItem(Owner, Label, Slot);
		}
		if (IsBufferType(Slot.Type))
		{
			return MakeBufferPropertyItem(Owner, Label, Slot);
		}
		return MakeTextureResourcePropertyItem(Owner, Label, Slot, Slot.Type, IsSamplerResourceBinding(Slot.BindingType));
	}

	static TSharedRef<PropertyItemBase> MakeBufferPropertyItemImpl(ShObject* Owner, FText Label, BindingType BindingTypeValue, uint32 StructuredStride, uint32& BufferByteSize, TypedBufferFormat& BufferFormat)
	{
		BufferByteSize = FMath::Max(BufferByteSize, GetMinBufferByteSize(BindingTypeValue, StructuredStride, BufferFormat));
		auto Item = MakeShared<PropertyItemBase>(Owner, Label);

		if (IsRWBufferBinding(BindingTypeValue))
		{
			auto ByteSizeItem = MakeShared<PropertyScalarItem<int32>>(Owner, LOCALIZATION("ByteSize"), reinterpret_cast<int32*>(&BufferByteSize));
			ByteSizeItem->SetMinValue(GetMinBufferByteSize(BindingTypeValue, StructuredStride, BufferFormat));
			Item->AddChild(ByteSizeItem);
		}

		if (IsTypedBufferBinding(BindingTypeValue))
		{
			TypedBufferFormat* FormatPtr = &BufferFormat;
			auto FormatItem = MakePropertyEnumItem<TypedBufferFormat>(
				Owner, LOCALIZATION("Format"), BufferFormat,
				[FormatPtr](TypedBufferFormat NewValue) { *FormatPtr = NewValue; }
			);
			Item->AddChild(FormatItem);
		}

		return Item;
	}

	TSharedRef<PropertyItemBase> MakeBufferPropertyItem(ShObject* Owner, FText Label, BindingType BindingTypeValue, uint32 StructuredStride, uint32& BufferByteSize, TypedBufferFormat& BufferFormat)
	{
		return MakeBufferPropertyItemImpl(Owner, Label, BindingTypeValue, StructuredStride, BufferByteSize, BufferFormat);
	}

	TSharedRef<PropertyItemBase> MakeBufferPropertyItem(ShObject* Owner, FText Label, ShaderOverrideSlot& Slot)
	{
		return MakeBufferPropertyItemImpl(Owner, Label, Slot.BindingType, Slot.StructuredStride, Slot.BufferByteSize, Slot.BufferFormat);
	}
}
