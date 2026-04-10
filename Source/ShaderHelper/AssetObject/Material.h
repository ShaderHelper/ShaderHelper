#pragma once
#include "AssetObject/PixelShader.h"
#include "AssetObject/VertexShader.h"
#include "AssetObject/Texture2D.h"
#include "AssetObject/TextureCube.h"
#include "AssetObject/Texture3D.h"
#include "GpuApi/GpuSampler.h"

namespace SH
{
	enum class BuiltInMatrix4x4Value : uint8
	{
		None,
		BuiltInModel,
		BuiltInViewProj,
	};

	enum class BuiltInVertexAttribute : uint8
	{
		None,
		BuiltInPosition,
		BuiltInNormal,
		BuiltInUV,
		BuiltInColor,
	};

	struct MaterialBindingMemberDefault
	{
		FString BindingName;
		FString MemberName;
		FString Type;
		BuiltInMatrix4x4Value MatrixValue = BuiltInMatrix4x4Value::None;
		union {
			float Values[4];
			int32 IntValues[4];
			uint32 UintValues[4];
		};

		MaterialBindingMemberDefault() { FMemory::Memzero(Values); }

		friend FArchive& operator<<(FArchive& Ar, MaterialBindingMemberDefault& Default)
		{
			Ar << Default.BindingName << Default.MemberName << Default.Type << Default.MatrixValue;
			for (int i = 0; i < 4; ++i) Ar << Default.UintValues[i];
			return Ar;
		}
	};

	struct MaterialBindingResourceDefault
	{
		FString BindingName;
		FW::BindingType BindingType = FW::BindingType::Texture;

		// Texture/RWTexture
		FW::AssetPtr<FW::AssetObject> TextureAsset;

		// Sampler
		FW::SamplerFilter Filter = FW::SamplerFilter::Bilinear;
		FW::SamplerAddressMode AddressMode = FW::SamplerAddressMode::Wrap;

		friend FArchive& operator<<(FArchive& Ar, MaterialBindingResourceDefault& Default)
		{
			Ar << Default.BindingName << Default.BindingType;
			Ar << Default.TextureAsset;
			Ar << Default.Filter << Default.AddressMode;
			return Ar;
		}
	};

	struct MaterialVertexInputDefault
	{
		uint32 Location = 0;
		FString SemanticName;
		uint32 SemanticIndex = 0;
		FString Type;
		BuiltInVertexAttribute Attribute = BuiltInVertexAttribute::None;

		friend FArchive& operator<<(FArchive& Ar, MaterialVertexInputDefault& Default)
		{
			Ar << Default.Location << Default.SemanticName << Default.SemanticIndex;
			Ar << Default.Type << Default.Attribute;
			return Ar;
		}
	};

	class Material : public FW::AssetObject
	{
		REFLECTION_TYPE(Material)
	public:
		Material() = default;
		~Material() override;

		void Serialize(FArchive& Ar) override;
		void PostLoad() override;
		FString FileExtension() const override;
		TArray<TSharedRef<FW::PropertyData>> GeneratePropertyDatas() override;
		void PostPropertyChanged(FW::PropertyData* InProperty) override;
		void NotifyMaterialChanged();
		void RefreshShaderBindings();

	public:
		FW::AssetPtr<VertexShader> VertexShaderAsset;
		FW::AssetPtr<PixelShader> PixelShaderAsset;
		TArray<MaterialBindingMemberDefault> BindingMemberDefaults;
		TArray<MaterialBindingResourceDefault> BindingResourceDefaults;
		TArray<MaterialVertexInputDefault> VertexInputDefaults;
		FSimpleMulticastDelegate OnMaterialChanged;

		// Render State
		FW::RasterizerFillMode FillMode = FW::RasterizerFillMode::Solid;
		FW::RasterizerCullMode CullMode = FW::RasterizerCullMode::Back;
		bool BlendEnable = false;
		FW::BlendFactor SrcBlendFactor = FW::BlendFactor::SrcAlpha;
		FW::BlendFactor DestBlendFactor = FW::BlendFactor::InvSrcAlpha;
		FW::BlendOp ColorBlendOp = FW::BlendOp::Add;
		bool DepthTestEnable = true;
		FW::CompareMode DepthCompare = FW::CompareMode::Less;

	protected:
		TRefCountPtr<FW::GpuTexture> GenerateThumbnail() const override;

	private:
		void UnbindShaderDelegates();
		void OnShaderBindingsChanged();
		void RebuildBindingMemberDefaults();
		void RebuildBindingResourceDefaults();
		void RebuildVertexInputDefaults();
		TArray<TSharedRef<FW::PropertyData>> BuildBindingDefaultPropertyDatas();
		TArray<TSharedRef<FW::PropertyData>> BuildRenderStatePropertyDatas();
		TArray<TSharedRef<FW::PropertyData>> BuildVertexInputPropertyDatas();
	};
}
