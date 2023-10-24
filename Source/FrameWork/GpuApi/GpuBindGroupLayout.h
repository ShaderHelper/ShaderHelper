#pragma once
#include "GpuResourceCommon.h"

namespace FRAMEWORK
{

	using BindingGroupIndex = int32;
	using BindingSlot = int32;

	enum class BindingShaderStage
	{
		Vertex,
		Pixel,
		All,
		Num,
	};

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

	template <>
	struct TTypeTraits<LayoutBinding> : public TTypeTraitsBase <LayoutBinding>
	{
		enum { IsBytewiseComparable = true };
	};

	struct GpuBindGroupLayoutDesc
	{
		
		bool operator==(const GpuBindGroupLayoutDesc& Other) const
		{
			return GroupNumber == Other.GroupNumber && Layouts == Other.Layouts;
		}

		friend uint32 GetTypeHash(const GpuBindGroupLayoutDesc& Key)
		{
			//Assume the LayoutBinding type is trivially comparable.
			uint32 Hash = FCrc::MemCrc32(Key.Layouts.GetData(), sizeof(LayoutBinding) * Key.Layouts.Num());
			return HashCombine(Hash, GetTypeHash(Key.GroupNumber));
		}

		BindingGroupIndex GroupNumber;
		TArray<LayoutBinding> Layouts;
	};

	class GpuBindGroupLayout : public GpuResource
	{
	public:
		GpuBindGroupLayout() : GpuResource(GpuResourceType::BindGroupLayout)
		{}
	};
}