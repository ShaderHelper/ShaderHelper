#pragma once
#include "GpuResourceCommon.h"

namespace FRAMEWORK
{

	using BindingGroupSlot = int32;
	using BindingSlot = int32;

	enum class BindingShaderStage : uint32
	{
		Vertex = 1u << 0,
		Pixel = 1u << 1,
		All = Vertex | Pixel,
	};
    ENUM_CLASS_FLAGS(BindingShaderStage);

	enum class BindingType
	{
		UniformBuffer,
	};

	struct LayoutBinding
	{
		BindingSlot Slot;
		BindingType Type;
		BindingShaderStage Stage = BindingShaderStage::All;
	};
	//Make sure there are no padding bytes.
	static_assert(std::has_unique_object_representations_v<LayoutBinding>);

	struct GpuBindGroupLayoutDesc
	{
		
		bool operator==(const GpuBindGroupLayoutDesc& Other) const
		{
			return GroupNumber == Other.GroupNumber && 
				!FMemory::Memcmp(Layouts.GetData(), Other.Layouts.GetData(), sizeof(LayoutBinding) * Layouts.Num());
		}

		friend uint32 GetTypeHash(const GpuBindGroupLayoutDesc& Key)
		{
			//Assume the LayoutBinding type is trivially comparable.
			uint32 Hash = FCrc::MemCrc32(Key.Layouts.GetData(), sizeof(LayoutBinding) * Key.Layouts.Num());
			return HashCombine(Hash, ::GetTypeHash(Key.GroupNumber));
		}

		BindingGroupSlot GroupNumber;
		TArray<LayoutBinding> Layouts;
	};

	class GpuBindGroupLayout : public GpuResource
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

	protected:
		GpuBindGroupLayoutDesc Desc;
	};
}
