#pragma once
#include "GpuResourceCommon.h"
#include <Containers/SortedMap.h>

namespace FW
{

	using BindingGroupSlot = int32;
	using BindingSlot = int32;

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
		Sampler,
		RWStorageBuffer,
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
				Hash = HashCombine(Hash, ::GetTypeHash(K));
				Hash = HashCombine(Hash, GetTypeHash(V));
			}
			return Hash;
		}
        
        friend FArchive& operator<<(FArchive& Ar, GpuBindGroupLayoutDesc& BindLayoutDesc)
        {
            Ar << BindLayoutDesc.GroupNumber;
            Ar << BindLayoutDesc.Layouts;
            Ar << BindLayoutDesc.CodegenDeclaration;
            Ar << BindLayoutDesc.CodegenBindingNameToSlot;
            return Ar;
        }
        
        BindingType GetBindingType(BindingSlot InSlot) const
        {
            return Layouts[InSlot].Type;
        }
        
        bool HasBinding(const FString& BindingName) const
        {
            return CodegenBindingNameToSlot.Contains(BindingName);
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
		GpuBindGroupLayoutBuilder& AddExistingBinding(BindingSlot InSlot, BindingType Type, BindingShaderStage InStage = BindingShaderStage::All);

		//Add the codegen bindings, then get the CodegenDeclaration that will be injected into a shader.
		GpuBindGroupLayoutBuilder& AddUniformBuffer(const FString& BindingName, const FString& UniformBufferLayoutDeclaration, BindingShaderStage InStage = BindingShaderStage::All);
		GpuBindGroupLayoutBuilder& AddTexture(const FString& BindingName, BindingShaderStage InStage = BindingShaderStage::All);
		//TODO StaticSampler ? It will be embed into BingGroupLayout, vulkan has the same concept, but metal might not have.
		GpuBindGroupLayoutBuilder& AddSampler(const FString& BindingName, BindingShaderStage InStage = BindingShaderStage::All);

        const FString& GetCodegenDeclaration() const { return LayoutDesc.CodegenDeclaration; }
        const GpuBindGroupLayoutDesc& GetLayoutDesc() const { return LayoutDesc; }
		TRefCountPtr<GpuBindGroupLayout> Build();
        
        friend FArchive& operator<<(FArchive& Ar, GpuBindGroupLayoutBuilder& BindLayoutBuilder)
        {
            Ar << BindLayoutBuilder.AutoSlot;
            Ar << BindLayoutBuilder.LayoutDesc;
            return Ar;
        }

	private:
		BindingSlot AutoSlot;
		GpuBindGroupLayoutDesc LayoutDesc;
	};

}
