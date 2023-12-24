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
		Texture,
		Sampler,
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

		LayoutBinding* FindBinding(BindingSlot InSlot)
		{
			return Layouts.FindByPredicate([InSlot](const LayoutBinding& LayoutEntry) {
				return InSlot == LayoutEntry.Slot;
			});
		}

		BindingGroupSlot GroupNumber;
		TArray<LayoutBinding> Layouts;

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
		FString GetCodegenDeclaration() const { return Desc.CodegenDeclaration; }

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
		GpuBindGroupLayoutBuilder& AddSampler(const FString& BindingName, BindingShaderStage InStage = BindingShaderStage::All);

		TRefCountPtr<GpuBindGroupLayout> Build();

	private:
		BindingSlot AutoSlot;
		GpuBindGroupLayoutDesc LayoutDesc;
	};

}
