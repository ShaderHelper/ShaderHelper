#include "CommonHeader.h"
#include "Material.h"
#include "Renderer/MaterialPreviewRenderer.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"
#include "UI/Widgets/Property/PropertyData/PropertyAssetItem.h"
#include "GpuApi/GpuSampler.h"

using namespace FW;

namespace SH
{
	namespace
	{
		TArray<GpuShaderLayoutBinding> GetShaderBindings(const ShaderAsset* InShader)
		{
			if (InShader && InShader->Shader && InShader->Shader->IsCompiled())
			{
				return InShader->Shader->GetLayout();
			}
			return {};
		}
	}

	REFLECTION_REGISTER(AddClass<Material>("Material")
		.BaseClass<AssetObject>()
		.Data<&Material::VertexShaderAsset, MetaInfo::Property>(LOCALIZATION("VertexShader"))
		.Data<&Material::PixelShaderAsset, MetaInfo::Property>(LOCALIZATION("PixelShader"))
	)

	Material::~Material()
	{
		UnbindShaderDelegates();
	}

	void Material::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);
		Ar << VertexShaderAsset;
		Ar << PixelShaderAsset;
		Ar << BindingMemberDefaults;
		Ar << BindingResourceDefaults;
		Ar << VertexInputDefaults;
		Ar << FillMode << CullMode;
		Ar << BlendEnable << SrcBlendFactor << DestBlendFactor << ColorBlendOp;
		Ar << DepthTestEnable << DepthCompare;
	}

	void Material::PostLoad()
	{
		AssetObject::PostLoad();
		RefreshShaderBindings();
		RebuildBindingMemberDefaults();
		RebuildBindingResourceDefaults();
		RebuildVertexInputDefaults();
	}

	FString Material::FileExtension() const
	{
		return "material";
	}

	TArray<TSharedRef<PropertyData>> Material::GeneratePropertyDatas()
	{
		TArray<TSharedRef<PropertyData>> Result = ShObject::GeneratePropertyDatas();
		Result.Append(BuildRenderStatePropertyDatas());
		Result.Append(BuildVertexInputPropertyDatas());
		Result.Append(BuildBindingDefaultPropertyDatas());
		return Result;
	}

	void Material::PostPropertyChanged(PropertyData* InProperty)
	{
		AssetObject::PostPropertyChanged(InProperty);

		bool bShaderChanged = InProperty->GetDisplayName().EqualTo(LOCALIZATION("VertexShader"))
			|| InProperty->GetDisplayName().EqualTo(LOCALIZATION("PixelShader"));

		if (bShaderChanged)
		{
			RefreshShaderBindings();
			OnShaderBindingsChanged();
		}
		else
		{
			NotifyMaterialChanged();
		}
	}

	void Material::RefreshShaderBindings()
	{
		UnbindShaderDelegates();

		if (VertexShaderAsset)
		{
			VertexShaderAsset->OnShaderRefreshed.AddRaw(this, &Material::OnShaderBindingsChanged);
			VertexShaderAsset->OnDestroy.AddRaw(this, &Material::OnShaderBindingsChanged);
		}

		if (PixelShaderAsset)
		{
			PixelShaderAsset->OnShaderRefreshed.AddRaw(this, &Material::OnShaderBindingsChanged);
			PixelShaderAsset->OnDestroy.AddRaw(this, &Material::OnShaderBindingsChanged);
		}
	}

	void Material::OnShaderBindingsChanged()
	{
		RebuildBindingMemberDefaults();
		RebuildBindingResourceDefaults();
		RebuildVertexInputDefaults();
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->RefreshProperty();
		NotifyMaterialChanged();
	}

	TRefCountPtr<GpuTexture> Material::GenerateThumbnail() const
	{
		return MaterialPreviewRenderer::RenderThumbnail(this, 128, MaterialPreviewPrimitive::Sphere);
	}

	void Material::NotifyMaterialChanged()
	{
		static_cast<ShaderHelperEditor*>(GApp->GetEditor())->GetAssetBrowser()->RefreshAssetThumbnail(GetPath());
		OnMaterialChanged.Broadcast();
	}

	void Material::UnbindShaderDelegates()
	{
		if (VertexShaderAsset)
		{
			VertexShaderAsset->OnShaderRefreshed.RemoveAll(this);
			VertexShaderAsset->OnDestroy.RemoveAll(this);
		}

		if (PixelShaderAsset)
		{
			PixelShaderAsset->OnShaderRefreshed.RemoveAll(this);
			PixelShaderAsset->OnDestroy.RemoveAll(this);
		}
	}

	void Material::RebuildBindingMemberDefaults()
	{
		TArray<MaterialBindingMemberDefault> OldDefaults = MoveTemp(BindingMemberDefaults);

		TArray<GpuShaderLayoutBinding> SeenUbs;

		auto UbLayoutsMatch = [](const TArray<GpuShaderUbMemberInfo>& A, const TArray<GpuShaderUbMemberInfo>& B) {
			if (A.Num() != B.Num()) return false;
			for (int32 i = 0; i < A.Num(); ++i)
			{
				if (A[i].Name != B[i].Name || A[i].Offset != B[i].Offset || A[i].Size != B[i].Size || A[i].Type != B[i].Type)
					return false;
			}
			return true;
		};

		auto PreserveOldValues = [&](MaterialBindingMemberDefault& NewDefault) {
			for (const auto& Old : OldDefaults)
			{
				if (Old.MemberName != NewDefault.MemberName) continue;
				FString Left, Right;
				bool bSplit = Old.BindingName.Split(TEXT("/"), &Left, &Right);
				if (Old.BindingName == NewDefault.BindingName ||
					(bSplit && (Left == NewDefault.BindingName || Right == NewDefault.BindingName)))
				{
					NewDefault.MatrixValue = Old.MatrixValue;
					FMemory::Memcpy(NewDefault.Values, Old.Values, sizeof(NewDefault.Values));
					break;
				}
			}
		};

		auto AddUbMembers = [&](const FString& DisplayName, const TArray<GpuShaderUbMemberInfo>& UbMembers, BindingShaderStage Stage) {
			for (const GpuShaderUbMemberInfo& Member : UbMembers)
			{
				MaterialBindingMemberDefault NewDefault;
				NewDefault.BindingName = DisplayName;
				NewDefault.MemberName = Member.Name;
				NewDefault.Type = Member.Type;
				NewDefault.Stage = Stage;
				PreserveOldValues(NewDefault);
				BindingMemberDefaults.Add(MoveTemp(NewDefault));
			}
		};

		auto CollectUbMembers = [&](const ShaderAsset* InShader) {
			for (const GpuShaderLayoutBinding& Binding : GetShaderBindings(InShader))
			{
				if (Binding.Type != BindingType::UniformBuffer) continue;

				auto* Seen = SeenUbs.FindByPredicate([&](const GpuShaderLayoutBinding& S) {
					return S.Group == Binding.Group && S.Slot == Binding.Slot;
				});
				if (Seen)
				{
					if (UbLayoutsMatch(Seen->UbMembers, Binding.UbMembers))
					{
						// Same layout: merge names if different, combine stages
						if (Seen->Name != Binding.Name)
						{
							FString OldName = Seen->Name;
							Seen->Name = OldName + TEXT("/") + Binding.Name;
							for (auto& D : BindingMemberDefaults)
							{
								if (D.BindingName == OldName) D.BindingName = Seen->Name;
							}
						}
						for (auto& D : BindingMemberDefaults)
						{
							if (D.BindingName == Seen->Name) D.Stage = D.Stage | Binding.Stage;
						}
					}
					else
					{
						// Different layout: keep separate with per-stage
						SeenUbs.Add(Binding);
						AddUbMembers(Binding.Name, Binding.UbMembers, Binding.Stage);
					}
					continue;
				}

				SeenUbs.Add(Binding);
				AddUbMembers(Binding.Name, Binding.UbMembers, Binding.Stage);
			}
		};

		CollectUbMembers(VertexShaderAsset.Get());
		CollectUbMembers(PixelShaderAsset.Get());
	}

	void Material::RebuildBindingResourceDefaults()
	{
		TArray<MaterialBindingResourceDefault> OldDefaults = MoveTemp(BindingResourceDefaults);

		TArray<GpuShaderLayoutBinding> SeenResBindings;

		auto CollectResourceBindings = [&](const ShaderAsset* InShader) {
			for (const GpuShaderLayoutBinding& Binding : GetShaderBindings(InShader))
			{
				switch (Binding.Type)
				{
				case BindingType::Texture: case BindingType::TextureCube: case BindingType::Texture3D:
				case BindingType::Sampler:
				case BindingType::CombinedTextureSampler: case BindingType::CombinedTextureCubeSampler: case BindingType::CombinedTexture3DSampler:
					break;
				default: continue;
				}

				// Check if already processed a binding at this (Group, Slot, Type)
				auto* Seen = SeenResBindings.FindByPredicate([&](const GpuShaderLayoutBinding& S) {
					return S.Group == Binding.Group && S.Slot == Binding.Slot && S.Type == Binding.Type;
				});
				if (Seen)
				{
					// Merge: update name and stage on existing default
					for (auto& D : BindingResourceDefaults)
					{
						if (D.BindingName == Seen->Name && D.BindingType == Seen->Type)
						{
							D.Stage = D.Stage | Binding.Stage;
							if (Seen->Name != Binding.Name)
								D.BindingName = Seen->Name + TEXT("/") + Binding.Name;
							break;
						}
					}
					if (Seen->Name != Binding.Name)
						Seen->Name = Seen->Name + TEXT("/") + Binding.Name;
					continue;
				}

				SeenResBindings.Add(Binding);

				MaterialBindingResourceDefault NewDefault;
				NewDefault.BindingName = Binding.Name;
				NewDefault.BindingType = Binding.Type;
				NewDefault.Stage = Binding.Stage;

				// Preserve old values if existed
				for (const auto& Old : OldDefaults)
				{
					FString Left, Right;
					bool bMergedName = Old.BindingName.Split(TEXT("/"), &Left, &Right);
					if (Old.BindingName == Binding.Name || (bMergedName && (Left == Binding.Name || Right == Binding.Name)))
					{
						NewDefault.TextureAsset = Old.TextureAsset;
						NewDefault.Filter = Old.Filter;
						NewDefault.AddressMode = Old.AddressMode;
						break;
					}
				}

				BindingResourceDefaults.Add(MoveTemp(NewDefault));
			}
		};

		CollectResourceBindings(VertexShaderAsset.Get());
		CollectResourceBindings(PixelShaderAsset.Get());
	}

	void Material::RebuildVertexInputDefaults()
	{
		TArray<MaterialVertexInputDefault> OldDefaults = MoveTemp(VertexInputDefaults);

		if (VertexShaderAsset && VertexShaderAsset->Shader && VertexShaderAsset->Shader->IsCompiled())
		{
			TArray<GpuShaderVertexInput> Inputs = VertexShaderAsset->Shader->GetVertexInputs();
			for (const GpuShaderVertexInput& Input : Inputs)
			{
				MaterialVertexInputDefault NewDefault;
				NewDefault.Location = Input.Location;
				NewDefault.Name = Input.Name;
				NewDefault.SemanticName = Input.SemanticName;
				NewDefault.SemanticIndex = Input.SemanticIndex;
				NewDefault.Type = Input.Type;

				// Preserve old Attribute assignment by Location match
				bool bPreserved = false;
				for (const auto& Old : OldDefaults)
				{
					if (Old.Location == Input.Location)
					{
						NewDefault.Attribute = Old.Attribute;
						bPreserved = true;
						break;
					}
				}

				VertexInputDefaults.Add(MoveTemp(NewDefault));
			}
		}
	}

	TArray<TSharedRef<PropertyData>> Material::BuildRenderStatePropertyDatas()
	{
		auto RenderStateCategory = MakeShared<PropertyCategory>(this, LOCALIZATION("RenderState"));
		TArray<TSharedRef<PropertyData>> Properties;
		Properties.Add(RenderStateCategory);

		// Rasterizer
		{
			auto RasterizerCategory = MakeShared<PropertyCategory>(this, LOCALIZATION("Rasterizer"));
			RenderStateCategory->AddChild(RasterizerCategory);

			// FillMode
			{
				auto CurrentName = MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(FillMode).data()));
				TMap<FString, TSharedPtr<void>> Entries;
				for (const auto& [V, S] : magic_enum::enum_entries<RasterizerFillMode>())
					Entries.Add(ANSI_TO_TCHAR(S.data()), MakeShared<RasterizerFillMode>(V));
				auto Item = MakeShared<PropertyEnumItem>(this, LOCALIZATION("FillMode"), CurrentName, Entries,
					[this](void* InValue) { FillMode = *static_cast<RasterizerFillMode*>(InValue); });
				RasterizerCategory->AddChild(Item);
			}

			// CullMode
			{
				auto CurrentName = MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(CullMode).data()));
				TMap<FString, TSharedPtr<void>> Entries;
				for (const auto& [V, S] : magic_enum::enum_entries<RasterizerCullMode>())
					Entries.Add(ANSI_TO_TCHAR(S.data()), MakeShared<RasterizerCullMode>(V));
				auto Item = MakeShared<PropertyEnumItem>(this, LOCALIZATION("CullMode"), CurrentName, Entries,
					[this](void* InValue) { CullMode = *static_cast<RasterizerCullMode*>(InValue); });
				RasterizerCategory->AddChild(Item);
			}
		}

		// Blend
		{
			auto BlendCategory = MakeShared<PropertyCategory>(this, LOCALIZATION("Blend"));
			RenderStateCategory->AddChild(BlendCategory);

			auto EnableItem = MakeShared<PropertyScalarItem<bool>>(this, LOCALIZATION("Enable"), &BlendEnable);
			BlendCategory->AddChild(EnableItem);

			// SrcFactor
			{
				auto CurrentName = MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(SrcBlendFactor).data()));
				TMap<FString, TSharedPtr<void>> Entries;
				for (const auto& [V, S] : magic_enum::enum_entries<BlendFactor>())
					Entries.Add(ANSI_TO_TCHAR(S.data()), MakeShared<BlendFactor>(V));
				auto Item = MakeShared<PropertyEnumItem>(this, LOCALIZATION("SrcFactor"), CurrentName, Entries,
					[this](void* InValue) { SrcBlendFactor = *static_cast<BlendFactor*>(InValue); });
				BlendCategory->AddChild(Item);
			}

			// DestFactor
			{
				auto CurrentName = MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(DestBlendFactor).data()));
				TMap<FString, TSharedPtr<void>> Entries;
				for (const auto& [V, S] : magic_enum::enum_entries<BlendFactor>())
					Entries.Add(ANSI_TO_TCHAR(S.data()), MakeShared<BlendFactor>(V));
				auto Item = MakeShared<PropertyEnumItem>(this, LOCALIZATION("DestFactor"), CurrentName, Entries,
					[this](void* InValue) { DestBlendFactor = *static_cast<BlendFactor*>(InValue); });
				BlendCategory->AddChild(Item);
			}

			// ColorBlendOp
			{
				auto CurrentName = MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(ColorBlendOp).data()));
				TMap<FString, TSharedPtr<void>> Entries;
				for (const auto& [V, S] : magic_enum::enum_entries<BlendOp>())
					Entries.Add(ANSI_TO_TCHAR(S.data()), MakeShared<BlendOp>(V));
				auto Item = MakeShared<PropertyEnumItem>(this, LOCALIZATION("BlendOp"), CurrentName, Entries,
					[this](void* InValue) { ColorBlendOp = *static_cast<BlendOp*>(InValue); });
				BlendCategory->AddChild(Item);
			}
		}

		// DepthStencil
		{
			auto DepthCategory = MakeShared<PropertyCategory>(this, LOCALIZATION("Depth"));
			RenderStateCategory->AddChild(DepthCategory);

			auto TestItem = MakeShared<PropertyScalarItem<bool>>(this, LOCALIZATION("DepthTest"), &DepthTestEnable);
			DepthCategory->AddChild(TestItem);

			// CompareMode
			{
				auto CurrentName = MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(DepthCompare).data()));
				TMap<FString, TSharedPtr<void>> Entries;
				for (const auto& [V, S] : magic_enum::enum_entries<CompareMode>())
					Entries.Add(ANSI_TO_TCHAR(S.data()), MakeShared<CompareMode>(V));
				auto Item = MakeShared<PropertyEnumItem>(this, LOCALIZATION("CompareMode"), CurrentName, Entries,
					[this](void* InValue) { DepthCompare = *static_cast<CompareMode*>(InValue); });
				DepthCategory->AddChild(Item);
			}
		}

		return Properties;
	}

	TArray<TSharedRef<PropertyData>> Material::BuildVertexInputPropertyDatas()
	{
		TArray<TSharedRef<PropertyData>> Properties;
		auto VertexInputCategory = MakeShared<PropertyCategory>(this, LOCALIZATION("VertexInput"));
		Properties.Add(VertexInputCategory);

		for (auto& Default : VertexInputDefaults)
		{
			// Build label based on shader language
			FText ItemLabel;
			GpuShaderLanguage VsLanguage = VertexShaderAsset->Shader->GetShaderLanguage();

			if (VsLanguage == GpuShaderLanguage::HLSL)
			{
				// HLSL: "SEMANTICNAME[INDEX] (Type)" or "SEMANTICNAME (Type)" if index is 0
				FString SemanticStr = Default.SemanticName;
				if (Default.SemanticIndex > 0)
				{
					SemanticStr += FString::FromInt(Default.SemanticIndex);
				}
				ItemLabel = FText::FromString(SemanticStr + TEXT(" (") + Default.Type + TEXT(")"));
			}
			else
			{
				// GLSL: Attribute name (Type)
				ItemLabel = FText::FromString(Default.Name + TEXT(" (") + Default.Type + TEXT(")"));
			}

			auto CurrentValueName = MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(Default.Attribute).data()));

			TMap<FString, TSharedPtr<void>> EnumEntries;
			for (const auto& [EntryValue, EntryStr] : magic_enum::enum_entries<BuiltInVertexAttribute>())
			{
				EnumEntries.Add(ANSI_TO_TCHAR(EntryStr.data()), MakeShared<BuiltInVertexAttribute>(EntryValue));
			}

			uint32 Location = Default.Location;
			auto EnumItem = MakeShared<PropertyEnumItem>(
				this, ItemLabel, CurrentValueName, EnumEntries,
				[this, Location](void* InValue) {
					BuiltInVertexAttribute NewValue = *static_cast<BuiltInVertexAttribute*>(InValue);
					for (auto& D : VertexInputDefaults)
					{
						if (D.Location == Location)
						{
							D.Attribute = NewValue;
							break;
						}
					}
				}
			);
			VertexInputCategory->AddChild(EnumItem);
		}

		return Properties;
	}

	TArray<TSharedRef<PropertyData>> Material::BuildBindingDefaultPropertyDatas()
	{
		auto BindingCategory = MakeShared<PropertyCategory>(this, LOCALIZATION("Bindings"));
		TArray<TSharedRef<PropertyData>> Properties;
		Properties.Add(BindingCategory);

		// Vertex/Pixel stage categories (lazily attached)
		auto VertexCategory = MakeShared<PropertyCategory>(this, LOCALIZATION("Vertex"));
		auto PixelCategory = MakeShared<PropertyCategory>(this, LOCALIZATION("Pixel"));
		bool bAddedVertex = false, bAddedPixel = false;

		auto GetStageParent = [&](BindingShaderStage Stage) -> TSharedRef<PropertyCategory> {
			if (Stage == BindingShaderStage::Vertex)
			{
				if (!bAddedVertex) { BindingCategory->AddChild(VertexCategory); bAddedVertex = true; }
				return VertexCategory;
			}
			if (Stage == BindingShaderStage::Pixel)
			{
				if (!bAddedPixel) { BindingCategory->AddChild(PixelCategory); bAddedPixel = true; }
				return PixelCategory;
			}
			return BindingCategory;
		};

		// UB member defaults - group by (BindingName, Stage)
		TMap<FString, TSharedRef<PropertyCategory>> UbCategories;
		for (auto& Default : BindingMemberDefaults)
		{
			FString CatKey = Default.BindingName;
			if (Default.Stage == BindingShaderStage::Vertex) CatKey += TEXT("|VS");
			else if (Default.Stage == BindingShaderStage::Pixel) CatKey += TEXT("|PS");

			if (!UbCategories.Contains(CatKey))
			{
				auto Parent = GetStageParent(Default.Stage);
				auto UbCat = MakeShared<PropertyCategory>(this, Default.BindingName);
				UbCategories.Add(CatKey, UbCat);
				Parent->AddChild(UbCat);
			}
			auto& UbCategory = UbCategories[CatKey];

			FText ItemLabel = FText::FromString(Default.MemberName + TEXT(" (") + Default.Type + TEXT(")"));

			if (IsShaderMatrix4x4Type(Default.Type))
			{
				auto CurrentValueName = MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(Default.MatrixValue).data()));

				TMap<FString, TSharedPtr<void>> EnumEntries;
				for (const auto& [EntryValue, EntryStr] : magic_enum::enum_entries<BuiltInMatrix4x4Value>())
				{
					EnumEntries.Add(ANSI_TO_TCHAR(EntryStr.data()), MakeShared<BuiltInMatrix4x4Value>(EntryValue));
				}

				FString BindingName = Default.BindingName;
				FString MemberName = Default.MemberName;
				auto EnumItem = MakeShared<PropertyEnumItem>(
					this, ItemLabel, CurrentValueName, EnumEntries,
					[this, BindingName, MemberName](void* InValue) {
						BuiltInMatrix4x4Value NewValue = *static_cast<BuiltInMatrix4x4Value*>(InValue);
						for (auto& D : BindingMemberDefaults)
						{
							if (D.BindingName == BindingName && D.MemberName == MemberName)
							{
								D.MatrixValue = NewValue;
								break;
							}
						}
					}
				);
				UbCategory->AddChild(EnumItem);
			}
			else if (Default.Type == TEXT("float"))
			{
				auto Item = MakeShared<PropertyScalarItem<float>>(this, ItemLabel, &Default.Values[0]);
				UbCategory->AddChild(Item);
			}
			else if (Default.Type == TEXT("int"))
			{
				auto Item = MakeShared<PropertyScalarItem<int32>>(this, ItemLabel, &Default.IntValues[0]);
				UbCategory->AddChild(Item);
			}
			else if (Default.Type == TEXT("uint"))
			{
				auto Item = MakeShared<PropertyScalarItem<int32>>(this, ItemLabel, reinterpret_cast<int32*>(&Default.UintValues[0]));
				Item->SetMinValue(0);
				UbCategory->AddChild(Item);
			}
			else if (IsShaderBoolType(Default.Type))
			{
				auto Item = MakeShared<PropertyScalarItem<int32>>(this, ItemLabel, &Default.IntValues[0]);
				Item->SetMinValue(0);
				Item->SetMaxValue(1);
				UbCategory->AddChild(Item);
			}
			else if (IsShaderVector2Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector2fItem>(this, ItemLabel, reinterpret_cast<Vector2f*>(Default.Values));
				UbCategory->AddChild(Item);
			}
			else if (IsShaderVector3Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector3fItem>(this, ItemLabel, reinterpret_cast<Vector3f*>(Default.Values));
				UbCategory->AddChild(Item);
			}
			else if (IsShaderVector4Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector4fItem>(this, ItemLabel, reinterpret_cast<Vector4f*>(Default.Values));
				UbCategory->AddChild(Item);
			}
			else if (IsShaderIntVector2Type(Default.Type) || IsShaderBoolVector2Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector2iItem>(this, ItemLabel, Default.IntValues);
				UbCategory->AddChild(Item);
			}
			else if (IsShaderIntVector3Type(Default.Type) || IsShaderBoolVector3Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector3iItem>(this, ItemLabel, Default.IntValues);
				UbCategory->AddChild(Item);
			}
			else if (IsShaderIntVector4Type(Default.Type) || IsShaderBoolVector4Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector4iItem>(this, ItemLabel, Default.IntValues);
				UbCategory->AddChild(Item);
			}
			else if (IsShaderUintVector2Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector2iItem>(this, ItemLabel, reinterpret_cast<int32*>(Default.UintValues));
				UbCategory->AddChild(Item);
			}
			else if (IsShaderUintVector3Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector3iItem>(this, ItemLabel, reinterpret_cast<int32*>(Default.UintValues));
				UbCategory->AddChild(Item);
			}
			else if (IsShaderUintVector4Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector4iItem>(this, ItemLabel, reinterpret_cast<int32*>(Default.UintValues));
				UbCategory->AddChild(Item);
			}
		}

		// Resource binding defaults
		for (auto& Default : BindingResourceDefaults)
		{
			auto Parent = GetStageParent(Default.Stage);

			switch (Default.BindingType)
			{
			case BindingType::Texture:
			case BindingType::TextureCube:
			case BindingType::Texture3D:
			{
				MetaType* TexMetaType = GetMetaType<Texture2D>();
				if (Default.BindingType == BindingType::TextureCube) TexMetaType = GetMetaType<TextureCube>();
				else if (Default.BindingType == BindingType::Texture3D) TexMetaType = GetMetaType<Texture3D>();

				auto Item = MakeShared<PropertyAssetItem>(
					this,
					FText::FromString(Default.BindingName),
					TexMetaType,
					&Default.TextureAsset
				);
				Parent->AddChild(Item);
				break;
			}
			case BindingType::Sampler:
			{
				auto SamplerCategory = MakeShared<PropertyCategory>(this, Default.BindingName);
				Parent->AddChild(SamplerCategory);

				// Filter mode
				{
					auto CurrentFilterName = MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(Default.Filter).data()));
					TMap<FString, TSharedPtr<void>> FilterEntries;
					for (const auto& [V, S] : magic_enum::enum_entries<SamplerFilter>())
						FilterEntries.Add(ANSI_TO_TCHAR(S.data()), MakeShared<SamplerFilter>(V));

					FString BindingName = Default.BindingName;
					auto FilterItem = MakeShared<PropertyEnumItem>(
						this, LOCALIZATION("FilterMode"), CurrentFilterName, FilterEntries,
						[this, BindingName](void* InValue) {
							SamplerFilter NewFilter = *static_cast<SamplerFilter*>(InValue);
							for (auto& D : BindingResourceDefaults)
							{
								if (D.BindingName == BindingName)
								{
									D.Filter = NewFilter;
									break;
								}
							}
						}
					);
					SamplerCategory->AddChild(FilterItem);
				}

				// Address mode
				{
					auto CurrentAddrName = MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(Default.AddressMode).data()));
					TMap<FString, TSharedPtr<void>> AddrEntries;
					for (const auto& [V, S] : magic_enum::enum_entries<SamplerAddressMode>())
						AddrEntries.Add(ANSI_TO_TCHAR(S.data()), MakeShared<SamplerAddressMode>(V));

					FString BindingName = Default.BindingName;
					auto AddrItem = MakeShared<PropertyEnumItem>(
						this, LOCALIZATION("WrapMode"), CurrentAddrName, AddrEntries,
						[this, BindingName](void* InValue) {
							SamplerAddressMode NewMode = *static_cast<SamplerAddressMode*>(InValue);
							for (auto& D : BindingResourceDefaults)
							{
								if (D.BindingName == BindingName)
								{
									D.AddressMode = NewMode;
									break;
								}
							}
						}
					);
					SamplerCategory->AddChild(AddrItem);
				}
				break;
			}
			case BindingType::CombinedTextureSampler:
			case BindingType::CombinedTextureCubeSampler:
			case BindingType::CombinedTexture3DSampler:
			{
				auto CombinedCategory = MakeShared<PropertyCategory>(this, Default.BindingName);
				Parent->AddChild(CombinedCategory);

				// Texture asset
				MetaType* TexMetaType = GetMetaType<Texture2D>();
				if (Default.BindingType == BindingType::CombinedTextureCubeSampler) TexMetaType = GetMetaType<TextureCube>();
				else if (Default.BindingType == BindingType::CombinedTexture3DSampler) TexMetaType = GetMetaType<Texture3D>();

				auto TexItem = MakeShared<PropertyAssetItem>(
					this,
					LOCALIZATION("Texture"),
					TexMetaType,
					&Default.TextureAsset
				);
				CombinedCategory->AddChild(TexItem);

				// Filter mode
				{
					auto CurrentFilterName = MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(Default.Filter).data()));
					TMap<FString, TSharedPtr<void>> FilterEntries;
					for (const auto& [V, S] : magic_enum::enum_entries<SamplerFilter>())
						FilterEntries.Add(ANSI_TO_TCHAR(S.data()), MakeShared<SamplerFilter>(V));

					FString BindingName = Default.BindingName;
					auto FilterItem = MakeShared<PropertyEnumItem>(
						this, LOCALIZATION("FilterMode"), CurrentFilterName, FilterEntries,
						[this, BindingName](void* InValue) {
							SamplerFilter NewFilter = *static_cast<SamplerFilter*>(InValue);
							for (auto& D : BindingResourceDefaults)
							{
								if (D.BindingName == BindingName)
								{
									D.Filter = NewFilter;
									break;
								}
							}
						}
					);
					CombinedCategory->AddChild(FilterItem);
				}

				// Address mode
				{
					auto CurrentAddrName = MakeShared<FString>(ANSI_TO_TCHAR(magic_enum::enum_name(Default.AddressMode).data()));
					TMap<FString, TSharedPtr<void>> AddrEntries;
					for (const auto& [V, S] : magic_enum::enum_entries<SamplerAddressMode>())
						AddrEntries.Add(ANSI_TO_TCHAR(S.data()), MakeShared<SamplerAddressMode>(V));

					FString BindingName = Default.BindingName;
					auto AddrItem = MakeShared<PropertyEnumItem>(
						this, LOCALIZATION("WrapMode"), CurrentAddrName, AddrEntries,
						[this, BindingName](void* InValue) {
							SamplerAddressMode NewMode = *static_cast<SamplerAddressMode*>(InValue);
							for (auto& D : BindingResourceDefaults)
							{
								if (D.BindingName == BindingName)
								{
									D.AddressMode = NewMode;
									break;
								}
							}
						}
					);
					CombinedCategory->AddChild(AddrItem);
				}
				break;
			}
			default:
				break;
			}
		}

		return Properties;
	}
}
