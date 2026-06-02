#pragma once
#include "AssetObject/Render/ShaderOverrideHelper.h"
#include "AssetObject/Shader.h"
#include "AssetObject/Texture2D.h"
#include "AssetObject/TextureCube.h"
#include "AssetObject/Texture3D.h"
#include "GpuApi/GpuBuffer.h"
#include "GpuApi/GpuSampler.h"

namespace SH
{
	enum class BuiltInMatrix4x4Value : uint8
	{
		Model,
		View,
		Proj,
		ViewProj,
		MVP,
	};

	enum class BuiltInFloatValue : uint8
	{
		Time,
	};

	enum class BuiltInVector2Value : uint8
	{
		ViewportSize,
		MousePos,
	};

	enum class BuiltInVector3Value : uint8
	{
		CameraPos,
		CameraDir,
	};

	enum class BuiltInVertexAttribute : uint8
	{
		Position,
		Normal,
		UV0,
		UV1,
		UV2,
		UV3,
		Color,
		Tangent,
	};

	struct MaterialBindingMemberDefault
	{
		FString BindingName;
		FString MemberName;
		FString Type;
		FW::BindingShaderStage Stage = FW::BindingShaderStage::All;
		MaterialBindingValueSource ValueSource = MaterialBindingValueSource::Custom;
		BuiltInMatrix4x4Value MatrixValue = BuiltInMatrix4x4Value::Model;
		BuiltInFloatValue FloatValue = BuiltInFloatValue::Time;
		BuiltInVector2Value Vector2Value = BuiltInVector2Value::ViewportSize;
		BuiltInVector3Value Vector3Value = BuiltInVector3Value::CameraPos;
		uint32 Values[16];

		MaterialBindingMemberDefault()
		{
			FMemory::Memzero(Values);
		}

		friend FArchive& operator<<(FArchive& Ar, MaterialBindingMemberDefault& Default)
		{
			Ar << Default.BindingName << Default.MemberName << Default.Type << Default.Stage;
			Ar << Default.ValueSource << Default.MatrixValue << Default.FloatValue << Default.Vector2Value << Default.Vector3Value;
			for (int i = 0; i < 16; ++i) Ar << Default.Values[i];
			return Ar;
		}
	};

	struct MaterialBindingResourceDefault : ShaderResourceBindingState
	{
		FString BindingName;
		FW::BindingShaderStage Stage = FW::BindingShaderStage::All;

		friend FArchive& operator<<(FArchive& Ar, MaterialBindingResourceDefault& Default)
		{
			Ar << Default.BindingName << Default.Stage;
			Ar << static_cast<ShaderResourceBindingState&>(Default);
			return Ar;
		}
	};

	struct MaterialVertexInputDefault
	{
		uint32 Location = 0;
		FString Name;
		FString SemanticName;
		uint32 SemanticIndex = 0;
		FString Type;
		BuiltInVertexAttribute Attribute = BuiltInVertexAttribute::Position;

		friend FArchive& operator<<(FArchive& Ar, MaterialVertexInputDefault& Default)
		{
			Ar << Default.Location << Default.Name << Default.SemanticName << Default.SemanticIndex;
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
		FW::AssetPtr<Shader> VertexShaderAsset;
		FW::AssetPtr<Shader> PixelShaderAsset;
		TArray<MaterialBindingMemberDefault> BindingMemberDefaults;
		TArray<MaterialBindingResourceDefault> BindingResourceDefaults;
		TArray<MaterialVertexInputDefault> VertexInputDefaults;
		FSimpleMulticastDelegate OnMaterialChanged;

		// Render State
		FW::RasterizerFillMode FillMode = FW::RasterizerFillMode::Solid;
		FW::RasterizerCullMode CullMode = FW::RasterizerCullMode::Back;
		FW::RasterizerFrontFace FrontFace = FW::RasterizerFrontFace::Clockwise;
		FW::PrimitiveType Primitive = FW::PrimitiveType::TriangleList;
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
