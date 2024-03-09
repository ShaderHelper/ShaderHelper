#pragma once
#include "GpuResourceCommon.h"
#include <Core/Containers/SortedMap.h>

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
		Texture,
		Sampler,
	};

	struct LayoutBinding
	{
		BindingType Type;
		BindingShaderStage Stage = BindingShaderStage::All;
		
		bool operator==(const LayoutBinding& Other) const
		{
			return Type == Other.Type && Stage == Other.Stage;
		}

		friend uint32 GetTypeHash(const LayoutBinding& Key)
		{
			return HashCombine(::GetTypeHash(Key.Type), ::GetTypeHash(Key.Stage));
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
				Hash = HashCombine(Hash, ::GetTypeHash(K));
				Hash = HashCombine(Hash, GetTypeHash(V));
			}
			return Hash;
		}

		BindingGroupSlot GroupNumber;
		TSortedMap<BindingSlot, LayoutBinding> Layouts;

		FString CodegenDeclaration;
		TMap<FString, BindingSlot> CodegenBindingNameToSlot;
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
		const FString& GetCodegenDeclaration() const { return Desc.CodegenDeclaration; }

	protected:
		GpuBindGroupLayoutDesc Desc;
	};

	class FRAMEWORK_API GpuBindGroupLayoutBuilder
	{
	public:
		GpuBindGroupLayoutBuilder(BindingGroupSlot InGroupSlot);
		//!! If have bindings that are not from codegen, must first call this method for each binding, to make sure the bindings are compact.
		GpuBindGroupLayoutBuilder& AddExistingBinding(BindingSlot InSlot, BindingType ResourceType, BindingShaderStage InStage = BindingShaderStage::All);

		//Add the codegen bindings, then get the CodegenDeclaration that will be injected into a shader.
		GpuBindGroupLayoutBuilder& AddUniformBuffer(const FString& BindingName, const FString& UniformBufferLayoutDeclaration, BindingShaderStage InStage = BindingShaderStage::All);
		GpuBindGroupLayoutBuilder& AddTexture(const FString& BindingName, BindingShaderStage InStage = BindingShaderStage::All);
		//TODO StaticSampler ? It will be embed into BingGroupLayout, vulkan has the same concept, but metal might not have.
		GpuBindGroupLayoutBuilder& AddSampler(const FString& BindingName, BindingShaderStage InStage = BindingShaderStage::All);

		TRefCountPtr<GpuBindGroupLayout> Build();

	private:
		BindingSlot AutoSlot;
		GpuBindGroupLayoutDesc LayoutDesc;
	};

}
