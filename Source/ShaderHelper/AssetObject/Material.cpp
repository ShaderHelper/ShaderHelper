#include "CommonHeader.h"
#include "Material.h"
#include "AssetObject/Render/ShaderOverrideHelper.h"
#include "Renderer/MaterialPreviewRenderer.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/AssetBrowser/SAssetBrowser.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"
#include "UI/Widgets/Property/PropertyData/PropertyMatrixItem.h"
#include "UI/Widgets/Property/PropertyData/PropertyAssetItem.h"

using namespace FW;

namespace SH
{
	namespace
	{
		TArray<GpuShaderLayoutBinding> GetShaderBindings(GpuShader* InShader)
		{
			if (InShader && InShader->IsCompiled())
			{
				return InShader->GetLayout();
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
		Ar << FillMode << CullMode << Primitive;
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
			if (InProperty->IsOfType<PropertyAssetItem>())
			{
				const FString DisplayName = InProperty->GetDisplayName().ToString();
				const bool bIsRWResourceAsset = BindingResourceDefaults.ContainsByPredicate([&](const MaterialBindingResourceDefault& Default) {
					return IsRWResourceType(GetResourceOverrideType(Default.BindingType)) && Default.BindingName == DisplayName;
				});
				if (bIsRWResourceAsset)
				{
					static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
				}
			}
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
					if (Old.Type == NewDefault.Type)
					{
						NewDefault.ValueSource = Old.ValueSource;
						NewDefault.MatrixValue = Old.MatrixValue;
						NewDefault.FloatValue = Old.FloatValue;
						NewDefault.Vector2Value = Old.Vector2Value;
						NewDefault.Vector3Value = Old.Vector3Value;
						FMemory::Memcpy(NewDefault.Values, Old.Values, sizeof(NewDefault.Values));
					}
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

		auto CollectUbMembers = [&](GpuShader* InShader) {
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

		CollectUbMembers(VertexShaderAsset ? VertexShaderAsset->GetCompiledShader(ShaderType::Vertex) : nullptr);
		CollectUbMembers(PixelShaderAsset ? PixelShaderAsset->GetCompiledShader(ShaderType::Pixel) : nullptr);
	}

	void Material::RebuildBindingResourceDefaults()
	{
		TArray<MaterialBindingResourceDefault> OldDefaults = MoveTemp(BindingResourceDefaults);

		TArray<GpuShaderLayoutBinding> SeenResBindings;

		auto CollectResourceBindings = [&](GpuShader* InShader) {
			for (const GpuShaderLayoutBinding& Binding : GetShaderBindings(InShader))
			{
				if (Binding.Name == TEXT("GPrivate_Printer") && Binding.Type == BindingType::RWRawBuffer)
				{
					continue;
				}
				if (!IsResourceOverrideBinding(Binding.Type)) continue;

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
							D.StructuredStride = Binding.StructuredStride;
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
				NewDefault.StructuredStride = Binding.StructuredStride;

				// Preserve old values if existed
				for (const auto& Old : OldDefaults)
				{
					FString Left, Right;
					bool bMergedName = Old.BindingName.Split(TEXT("/"), &Left, &Right);
					bool MatchedName = Old.BindingName == Binding.Name || (bMergedName && (Left == Binding.Name || Right == Binding.Name));
					if (Old.BindingType == Binding.Type && MatchedName)
					{
						CopyShaderResourceBindingState(NewDefault, Old);
						break;
					}
				}

				BindingResourceDefaults.Add(MoveTemp(NewDefault));
			}
		};

		CollectResourceBindings(VertexShaderAsset ? VertexShaderAsset->GetCompiledShader(ShaderType::Vertex) : nullptr);
		CollectResourceBindings(PixelShaderAsset ? PixelShaderAsset->GetCompiledShader(ShaderType::Pixel) : nullptr);
	}

	void Material::RebuildVertexInputDefaults()
	{
		TArray<MaterialVertexInputDefault> OldDefaults = MoveTemp(VertexInputDefaults);

		GpuShader* VsShader = VertexShaderAsset ? VertexShaderAsset->GetCompiledShader(ShaderType::Vertex) : nullptr;
		if (VsShader)
		{
			TArray<GpuShaderVertexInput> Inputs = VsShader->GetVertexInputs();
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
				auto Item = MakePropertyEnumItem<RasterizerFillMode>(this, LOCALIZATION("FillMode"), FillMode,
					[this](RasterizerFillMode InValue) { FillMode = InValue; });
				RasterizerCategory->AddChild(Item);
			}

			// CullMode
			{
				auto Item = MakePropertyEnumItem<RasterizerCullMode>(this, LOCALIZATION("CullMode"), CullMode,
					[this](RasterizerCullMode InValue) { CullMode = InValue; });
				RasterizerCategory->AddChild(Item);
			}

			// PrimitiveType
			{
				auto Item = MakePropertyEnumItem<PrimitiveType>(this, LOCALIZATION("PrimitiveType"), Primitive,
					[this](PrimitiveType InValue) { Primitive = InValue; });
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
				auto Item = MakePropertyEnumItem<BlendFactor>(this, LOCALIZATION("SrcFactor"), SrcBlendFactor,
					[this](BlendFactor InValue) { SrcBlendFactor = InValue; });
				BlendCategory->AddChild(Item);
			}

			// DestFactor
			{
				auto Item = MakePropertyEnumItem<BlendFactor>(this, LOCALIZATION("DestFactor"), DestBlendFactor,
					[this](BlendFactor InValue) { DestBlendFactor = InValue; });
				BlendCategory->AddChild(Item);
			}

			// ColorBlendOp
			{
				auto Item = MakePropertyEnumItem<BlendOp>(this, LOCALIZATION("BlendOp"), ColorBlendOp,
					[this](BlendOp InValue) { ColorBlendOp = InValue; });
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
				auto Item = MakePropertyEnumItem<CompareMode>(this, LOCALIZATION("CompareMode"), DepthCompare,
					[this](CompareMode InValue) { DepthCompare = InValue; });
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
			GpuShaderLanguage VsLanguage = VertexShaderAsset->Language;

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

			uint32 Location = Default.Location;
			auto EnumItem = MakePropertyEnumItem<BuiltInVertexAttribute>(
				this, ItemLabel, Default.Attribute,
				[this, Location](BuiltInVertexAttribute NewValue) {
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
		auto FindMemberDefault = [this](const FString& BindingName, const FString& MemberName, BindingShaderStage Stage) -> MaterialBindingMemberDefault* {
			for (auto& D : BindingMemberDefaults)
			{
				if (D.BindingName == BindingName && D.MemberName == MemberName && D.Stage == Stage)
				{
					return &D;
				}
			}
			return nullptr;
		};
		auto AddValueSourceSwitchMenu = [&](const TSharedRef<PropertyItemBase>& Item, const MaterialBindingMemberDefault& Default, bool bShowCustomEntry, bool bShowBuiltInEntry) {
			FString BindingName = Default.BindingName;
			FString MemberName = Default.MemberName;
			BindingShaderStage Stage = Default.Stage;
			PropertyData* EditProperty = &Item.Get();
			Item->SetContextMenuExtender([this, BindingName, MemberName, Stage, EditProperty, FindMemberDefault, bShowCustomEntry, bShowBuiltInEntry](FMenuBuilder& MenuBuilder) {
				auto AddSourceEntry = [&](const FText& Label, MaterialBindingValueSource Source) {
					MenuBuilder.AddMenuEntry(
						Label,
						FText::GetEmpty(),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateLambda([this, BindingName, MemberName, Stage, Source, EditProperty, FindMemberDefault] {
								if (MaterialBindingMemberDefault* D = FindMemberDefault(BindingName, MemberName, Stage))
								{
									if (D->ValueSource != Source)
									{
										EditProperty->BeginEdit();
										D->ValueSource = Source;
										PostPropertyChanged(EditProperty);
										EditProperty->EndEdit();
										static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
									}
								}
							}),
							FCanExecuteAction(),
							FIsActionChecked::CreateLambda([BindingName, MemberName, Stage, Source, FindMemberDefault] {
								if (MaterialBindingMemberDefault* D = FindMemberDefault(BindingName, MemberName, Stage))
								{
									return D->ValueSource == Source;
								}
								return false;
							})
						),
						NAME_None,
						EUserInterfaceActionType::ToggleButton
					);
				};
				if (bShowCustomEntry)
				{
					AddSourceEntry(LOCALIZATION("Custom"), MaterialBindingValueSource::Custom);
				}
				if (bShowBuiltInEntry)
				{
					AddSourceEntry(LOCALIZATION("Builtin"), MaterialBindingValueSource::BuiltIn);
				}
			});
		};
		auto MakeBuiltInMatrixItem = [&](const FText& ItemLabel, MaterialBindingMemberDefault& Default) -> TSharedRef<PropertyItemBase> {
			FString BindingName = Default.BindingName;
			FString MemberName = Default.MemberName;
			BindingShaderStage Stage = Default.Stage;
			auto EnumItem = MakePropertyEnumItem<BuiltInMatrix4x4Value>(
				this, ItemLabel, Default.MatrixValue,
				[this, BindingName, MemberName, Stage, FindMemberDefault](BuiltInMatrix4x4Value NewValue) {
					if (MaterialBindingMemberDefault* D = FindMemberDefault(BindingName, MemberName, Stage))
					{
						D->MatrixValue = NewValue;
					}
				}
			);
			AddValueSourceSwitchMenu(EnumItem, Default, true, true);
			return EnumItem;
		};
		auto MakeBuiltInFloatItem = [&](const FText& ItemLabel, MaterialBindingMemberDefault& Default) -> TSharedRef<PropertyItemBase> {
			FString BindingName = Default.BindingName;
			FString MemberName = Default.MemberName;
			BindingShaderStage Stage = Default.Stage;
			auto EnumItem = MakePropertyEnumItem<BuiltInFloatValue>(
				this, ItemLabel, Default.FloatValue,
				[this, BindingName, MemberName, Stage, FindMemberDefault](BuiltInFloatValue NewValue) {
					if (MaterialBindingMemberDefault* D = FindMemberDefault(BindingName, MemberName, Stage))
					{
						D->FloatValue = NewValue;
					}
				}
			);
			AddValueSourceSwitchMenu(EnumItem, Default, true, true);
			return EnumItem;
		};
		auto MakeBuiltInVector2Item = [&](const FText& ItemLabel, MaterialBindingMemberDefault& Default) -> TSharedRef<PropertyItemBase> {
			FString BindingName = Default.BindingName;
			FString MemberName = Default.MemberName;
			BindingShaderStage Stage = Default.Stage;
			auto EnumItem = MakePropertyEnumItem<BuiltInVector2Value>(
				this, ItemLabel, Default.Vector2Value,
				[this, BindingName, MemberName, Stage, FindMemberDefault](BuiltInVector2Value NewValue) {
					if (MaterialBindingMemberDefault* D = FindMemberDefault(BindingName, MemberName, Stage))
					{
						D->Vector2Value = NewValue;
					}
				}
			);
			AddValueSourceSwitchMenu(EnumItem, Default, true, true);
			return EnumItem;
		};
		auto MakeBuiltInVector3Item = [&](const FText& ItemLabel, MaterialBindingMemberDefault& Default) -> TSharedRef<PropertyItemBase> {
			FString BindingName = Default.BindingName;
			FString MemberName = Default.MemberName;
			BindingShaderStage Stage = Default.Stage;
			auto EnumItem = MakePropertyEnumItem<BuiltInVector3Value>(
				this, ItemLabel, Default.Vector3Value,
				[this, BindingName, MemberName, Stage, FindMemberDefault](BuiltInVector3Value NewValue) {
					if (MaterialBindingMemberDefault* D = FindMemberDefault(BindingName, MemberName, Stage))
					{
						D->Vector3Value = NewValue;
					}
				}
			);
			AddValueSourceSwitchMenu(EnumItem, Default, true, true);
			return EnumItem;
		};
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
				if (Default.ValueSource == MaterialBindingValueSource::BuiltIn)
				{
					UbCategory->AddChild(MakeBuiltInMatrixItem(ItemLabel, Default));
				}
				else
				{
					auto Item = MakeShared<PropertyMatrix4x4fItem>(this, ItemLabel, reinterpret_cast<float*>(Default.Values));
					AddValueSourceSwitchMenu(Item, Default, true, true);
					UbCategory->AddChild(Item);
				}
			}
			else if (IsShaderFloatType(Default.Type))
			{
				if (Default.ValueSource == MaterialBindingValueSource::BuiltIn)
				{
					UbCategory->AddChild(MakeBuiltInFloatItem(ItemLabel, Default));
				}
				else
				{
					auto Item = MakeShared<PropertyScalarItem<float>>(this, ItemLabel, reinterpret_cast<float*>(Default.Values));
					AddValueSourceSwitchMenu(Item, Default, true, true);
					UbCategory->AddChild(Item);
				}
			}
			else if (IsShaderIntType(Default.Type))
			{
				auto Item = MakeShared<PropertyScalarItem<int32>>(this, ItemLabel, reinterpret_cast<int32*>(Default.Values));
				UbCategory->AddChild(Item);
			}
			else if (IsShaderUintType(Default.Type))
			{
				auto Item = MakeShared<PropertyScalarItem<int32>>(this, ItemLabel, reinterpret_cast<int32*>(&Default.Values[0]));
				Item->SetMinValue(0);
				UbCategory->AddChild(Item);
			}
			else if (IsShaderBoolType(Default.Type))
			{
				auto Item = MakeShared<PropertyScalarItem<int32>>(this, ItemLabel, reinterpret_cast<int32*>(Default.Values));
				Item->SetMinValue(0);
				Item->SetMaxValue(1);
				UbCategory->AddChild(Item);
			}
			else if (IsShaderVector2Type(Default.Type))
			{
				if (Default.ValueSource == MaterialBindingValueSource::BuiltIn)
				{
					UbCategory->AddChild(MakeBuiltInVector2Item(ItemLabel, Default));
				}
				else
				{
					auto Item = MakeShared<PropertyVector2fItem>(this, ItemLabel, reinterpret_cast<Vector2f*>(Default.Values));
					AddValueSourceSwitchMenu(Item, Default, true, true);
					UbCategory->AddChild(Item);
				}
			}
			else if (IsShaderVector3Type(Default.Type))
			{
				if (Default.ValueSource == MaterialBindingValueSource::BuiltIn)
				{
					UbCategory->AddChild(MakeBuiltInVector3Item(ItemLabel, Default));
				}
				else
				{
					auto Item = MakeShared<PropertyVector3fItem>(this, ItemLabel, reinterpret_cast<Vector3f*>(Default.Values));
					AddValueSourceSwitchMenu(Item, Default, false, true);
					UbCategory->AddChild(Item);
				}
			}
			else if (IsShaderVector4Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector4fItem>(this, ItemLabel, reinterpret_cast<Vector4f*>(Default.Values));
				UbCategory->AddChild(Item);
			}
			else if (IsShaderIntVector2Type(Default.Type) || IsShaderBoolVector2Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector2iItem>(this, ItemLabel, reinterpret_cast<int32*>(Default.Values));
				UbCategory->AddChild(Item);
			}
			else if (IsShaderIntVector3Type(Default.Type) || IsShaderBoolVector3Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector3iItem>(this, ItemLabel, reinterpret_cast<int32*>(Default.Values));
				UbCategory->AddChild(Item);
			}
			else if (IsShaderIntVector4Type(Default.Type) || IsShaderBoolVector4Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector4iItem>(this, ItemLabel, reinterpret_cast<int32*>(Default.Values));
				UbCategory->AddChild(Item);
			}
			else if (IsShaderUintVector2Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector2iItem>(this, ItemLabel, reinterpret_cast<int32*>(Default.Values));
				UbCategory->AddChild(Item);
			}
			else if (IsShaderUintVector3Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector3iItem>(this, ItemLabel, reinterpret_cast<int32*>(Default.Values));
				UbCategory->AddChild(Item);
			}
			else if (IsShaderUintVector4Type(Default.Type))
			{
				auto Item = MakeShared<PropertyVector4iItem>(this, ItemLabel, reinterpret_cast<int32*>(Default.Values));
				UbCategory->AddChild(Item);
			}
		}

		// Resource binding defaults
		for (auto& Default : BindingResourceDefaults)
		{
			auto Parent = GetStageParent(Default.Stage);

			switch (Default.BindingType)
			{
			case BindingType::StructuredBuffer:
			case BindingType::RWStructuredBuffer:
			case BindingType::RawBuffer:
			case BindingType::RWRawBuffer:
			case BindingType::TypedBuffer:
			case BindingType::RWTypedBuffer:
			{
				auto Item = MakeBufferPropertyItem(
					this,
					FText::FromString(Default.BindingName),
					Default.BindingType,
					Default.StructuredStride,
					Default.BufferByteSize,
					Default.BufferFormat
				);
				Parent->AddChild(Item);
				break;
			}
			case BindingType::Texture:
			case BindingType::TextureCube:
			case BindingType::Texture3D:
			case BindingType::RWTexture:
			case BindingType::RWTexture3D:
			{
				auto Item = MakeTextureResourcePropertyItem(
					this,
					FText::FromString(Default.BindingName),
					Default,
					GetResourceOverrideType(Default.BindingType),
					false
				);
				Parent->AddChild(Item);
				break;
			}
			case BindingType::Sampler:
			{
				Parent->AddChild(MakeSamplerPropertyItem(this, FText::FromString(Default.BindingName), Default));
				break;
			}
			case BindingType::CombinedTextureSampler:
			case BindingType::CombinedTextureCubeSampler:
			case BindingType::CombinedTexture3DSampler:
			{
				auto TexItem = MakeTextureResourcePropertyItem(
					this,
					FText::FromString(Default.BindingName),
					Default,
					GetResourceOverrideType(Default.BindingType),
					false
				);
				Parent->AddChild(TexItem);
				AppendSamplerPropertyChildren(this, *TexItem, Default);
				break;
			}
			default:
				break;
			}
		}

		return Properties;
	}
}
