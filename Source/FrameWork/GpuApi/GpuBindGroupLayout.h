#pragma once
#include "GpuResourceCommon.h"
#include <Containers/SortedMap.h>

namespace FW
{
	enum class GpuShaderLanguage;
	class UniformBufferBuilder;

	using BindingGroupSlot = int32;

	enum class BindingShaderStage : uint32
	{
		Vertex = 1u << 0,
		Pixel = 1u << 1,
		Compute = 1u << 2,
		All = Vertex | Pixel | Compute,
	};
    ENUM_CLASS_FLAGS(BindingShaderStage);

	enum class BindingType
	{
		UniformBuffer,
		Texture,
		TextureCube,
		Texture3D,
		Sampler,
		CombinedTextureSampler,
		CombinedTextureCubeSampler,
		CombinedTexture3DSampler,
		StructuredBuffer,
		RWStructuredBuffer,
		RawBuffer,
		RWRawBuffer,
		RWTexture,
		RWTexture3D,
	};

	struct BindingSlot
	{
		int32 SlotNum;
		BindingType Type;
		BindingShaderStage Stage;

		bool operator==(const BindingSlot& Other) const = default;
		bool operator<(const BindingSlot& Other) const
		{
			if (SlotNum != Other.SlotNum) return SlotNum < Other.SlotNum;
			if (Type != Other.Type) return Type < Other.Type;
			return Stage < Other.Stage;
		}

		friend uint32 GetTypeHash(const BindingSlot& Key)
		{
			uint32 Hash = HashCombine(::GetTypeHash(Key.SlotNum), ::GetTypeHash(Key.Type));
			return HashCombine(Hash, ::GetTypeHash(Key.Stage));
		}

		friend FArchive& operator<<(FArchive& Ar, BindingSlot& Slot)
		{
			Ar << Slot.SlotNum;
			Ar << Slot.Type;
			Ar << Slot.Stage;
			return Ar;
		}
	};

	struct LayoutBinding
	{
		BindingType Type;
		BindingShaderStage Stage;
		
		bool operator==(const LayoutBinding& Other) const
		{
			return Type == Other.Type && Stage == Other.Stage;
		}

		friend uint32 GetTypeHash(const LayoutBinding& Key)
		{
			return HashCombine(::GetTypeHash(Key.Type), ::GetTypeHash(Key.Stage));
		}
        
        friend FArchive& operator<<(FArchive& Ar, LayoutBinding& Binding)
        {
            Ar << Binding.Type;
            Ar << Binding.Stage;
            return Ar;
        }
	};

	struct GpuBindGroupLayoutDesc
	{
		bool operator==(const GpuBindGroupLayoutDesc& Other) const
		{
			return GroupNumber == Other.GroupNumber && Layouts == Other.Layouts;
		}

		friend uint32 GetTypeHash(const GpuBindGroupLayoutDesc& Key)
		{
			uint32 Hash = ::GetTypeHash(Key.GroupNumber);
			for (const auto& [K, V] : Key.Layouts)
			{
				Hash = HashCombine(Hash, GetTypeHash(K));
				Hash = HashCombine(Hash, GetTypeHash(V));
			}
			return Hash;
		}
        
        friend FArchive& operator<<(FArchive& Ar, GpuBindGroupLayoutDesc& BindLayoutDesc)
        {
            Ar << BindLayoutDesc.GroupNumber;
            Ar << BindLayoutDesc.Layouts;
            Ar << BindLayoutDesc.HlslCodegenDeclaration << BindLayoutDesc.GlslCodegenDeclaration;
            Ar << BindLayoutDesc.CodegenBindingNameToSlot;
            return Ar;
        }
        
        BindingType GetBindingType(BindingSlot InSlot) const
        {
            return Layouts[InSlot].Type;
        }
        
        bool ContainsSlotNum(int32 InSlotNum) const
        {
            for (const auto& [Slot, _] : Layouts)
            {
                if (Slot.SlotNum == InSlotNum) return true;
            }
            return false;
        }
        
        bool HasCodegenBinding(const FString& BindingName) const
        {
            return CodegenBindingNameToSlot.Contains(BindingName);
        }

		BindingGroupSlot GroupNumber;
		TSortedMap<BindingSlot, LayoutBinding> Layouts;

		FString HlslCodegenDeclaration, GlslCodegenDeclaration;
		TMap<FString, BindingSlot> CodegenBindingNameToSlot;
	};

	class FRAMEWORK_API GpuBindGroupLayout : public GpuResource
	{
	public:
		GpuBindGroupLayout(const GpuBindGroupLayoutDesc& InDesc)
			: GpuResource(GpuResourceType::BindGroupLayout)
			, Desc(InDesc)
		{}

		const GpuBindGroupLayoutDesc& GetDesc() const {
			return Desc;
		}
		BindingGroupSlot GetGroupNumber() const { return Desc.GroupNumber; }
		const FString& GetCodegenDeclaration(GpuShaderLanguage Language) const;

	protected:
		GpuBindGroupLayoutDesc Desc;
	};

	class FRAMEWORK_API GpuBindGroupLayoutBuilder
	{
	public:
		GpuBindGroupLayoutBuilder(BindingGroupSlot InGroupSlot);
		//!! If have bindings that are not from codegen, must first call this method for each binding, to make sure the bindings are compact.
		GpuBindGroupLayoutBuilder& AddExistingBinding(int32 InSlotNum, BindingType Type, BindingShaderStage InStage);

		//Add the codegen bindings, then get the CodegenDeclaration that will be injected into a shader.
		GpuBindGroupLayoutBuilder& AddUniformBuffer(const FString& BindingName, const UniformBufferBuilder& UbBuilder, BindingShaderStage InStage);
		GpuBindGroupLayoutBuilder& AddTexture(const FString& BindingName, BindingShaderStage InStage);
		GpuBindGroupLayoutBuilder& AddTextureCube(const FString& BindingName, BindingShaderStage InStage);
		GpuBindGroupLayoutBuilder& AddTexture3D(const FString& BindingName, BindingShaderStage InStage);
		//TODO StaticSampler ? It will be embed into BingGroupLayout, vulkan has the same concept, but metal might not have.
		GpuBindGroupLayoutBuilder& AddSampler(const FString& BindingName, BindingShaderStage InStage);
		GpuBindGroupLayoutBuilder& AddCombinedTextureSampler(const FString& BindingName, BindingShaderStage InStage);
		GpuBindGroupLayoutBuilder& AddCombinedTextureCubeSampler(const FString& BindingName, BindingShaderStage InStage);
		GpuBindGroupLayoutBuilder& AddCombinedTexture3DSampler(const FString& BindingName, BindingShaderStage InStage);

		const FString& GetCodegenDeclaration(GpuShaderLanguage Language) const;
        const GpuBindGroupLayoutDesc& GetDesc() const { return LayoutDesc; }
		TRefCountPtr<GpuBindGroupLayout> Build() const;
        
        friend FArchive& operator<<(FArchive& Ar, GpuBindGroupLayoutBuilder& BindLayoutBuilder)
        {
            Ar << BindLayoutBuilder.AutoSlot;
            Ar << BindLayoutBuilder.LayoutDesc;
            return Ar;
        }

	private:
		int32 AutoSlot;
		GpuBindGroupLayoutDesc LayoutDesc;
	};

	// Binding shift offsets to separate HLSL register namespaces (b/t/s/u)
	// into non-overlapping flat binding/index ranges for backends that need it (Vulkan, Metal).
	inline constexpr int32 BindingShift_Buffer  = 0;
	inline constexpr int32 BindingShift_Texture = 128;
	inline constexpr int32 BindingShift_Sampler = 256;
	inline constexpr int32 BindingShift_CombinedSampler = 320;
	inline constexpr int32 BindingShift_UAV     = 384;

	// Per-stage binding offset for Vulkan: PS bindings are shifted by this amount
	// so that VS and PS can independently use the same slot numbers.
	inline constexpr int32 StageBindingOffset_Pixel = 512;

	inline int32 GetStageBindingOffset(BindingShaderStage Stage)
	{
		if (Stage == BindingShaderStage::Pixel) return StageBindingOffset_Pixel;
		return 0;
	}

	inline int32 GetBindingShift(BindingType Type)
	{
		switch (Type)
		{
		case BindingType::UniformBuffer:
			return BindingShift_Buffer;
		case BindingType::Texture:
		case BindingType::TextureCube:
		case BindingType::Texture3D:
		case BindingType::StructuredBuffer:
		case BindingType::RawBuffer:
			return BindingShift_Texture;
		case BindingType::Sampler:
			return BindingShift_Sampler;
		case BindingType::RWStructuredBuffer:
		case BindingType::RWRawBuffer:
		case BindingType::RWTexture:
		case BindingType::RWTexture3D:
			return BindingShift_UAV;
		case BindingType::CombinedTextureSampler:
		case BindingType::CombinedTextureCubeSampler:
		case BindingType::CombinedTexture3DSampler:
			return BindingShift_CombinedSampler;
		default:
			AUX::Unreachable();
		}
	}
}
