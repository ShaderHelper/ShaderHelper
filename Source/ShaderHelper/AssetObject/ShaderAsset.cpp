#include "CommonHeader.h"
#include "ShaderAsset.h"
#include "AssetObject/ShaderHeader.h"
#include "ProjectManager/ShProjectManager.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"

using namespace FW;

namespace SH
{
	namespace
	{
		TSharedRef<PropertyData> CreateBindingInfoProperty(ShObject* InOwner, const FString& InBindingName, const FString& InBindingInfo)
		{
			auto Property = MakeShared<PropertyItemBase>(InOwner, InBindingName);
			Property->SetEnabled(false);
			Property->SetEmbedWidget(
				SNew(STextBlock)
				.TextStyle(&FAppCommonStyle::Get().GetWidgetStyle<FTextBlockStyle>("MinorText"))
				.Text(FText::FromString(InBindingInfo))
			);
			return Property;
		}
	}

	REFLECTION_REGISTER(AddClass<ShaderAsset>("ShaderAsset")
								.BaseClass<AssetObject>()
	)

	ShaderAsset::ShaderAsset()
	{

	}

	void ShaderAsset::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);
		
		Ar << EditorContent;
		SavedEditorContent = EditorContent;
	}

	TArray<FString> ShaderAsset::GetIncludeDirs() const
	{
		return {
			PathHelper::ShaderDir(),
			TSingleton<ShProjectManager>::Get().GetActiveContentDirectory(),
			FPaths::GetPath(GetPath()),
		};
	}

	FString ShaderAsset::LoadIncludeFile(const FString& IncludePath)
	{
		if (FPaths::GetExtension(IncludePath) == TEXT("header"))
		{
			FScopeLock ScopeLock(&GAssetCS);
			AssetPtr<ShaderHeader> HeaderAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderHeader>(IncludePath);
			return HeaderAsset ? HeaderAsset->GetFullContent() : "";
		}

		FString Content;
		FFileHelper::LoadFileToString(Content, *IncludePath);
		return Content;
	}

	FString ShaderAsset::GetShaderName() const
	{
		return GetFileName() + "." + FileExtension();
	}

	AssetPtr<ShaderAsset> ShaderAsset::FindIncludeAsset(const FString& InShaderName)
	{
		if (InShaderName.IsEmpty())
		{
			return nullptr;
		}
		
		if (InShaderName == GetShaderName())
		{
			return this;
		}
		
		// Try to find in include directories
		for (const FString& IncludeDir : GetIncludeDirs())
		{
			FString FullPath = FPaths::Combine(IncludeDir, InShaderName);
			if (TSingleton<AssetManager>::Get().IsValidAsset(FullPath))
			{
				return TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderAsset>(FullPath);
			}
		}
		
		return nullptr;
	}

	TArray<TSharedRef<PropertyData>> ShaderAsset::BuildBindingPropertyDatas(GpuShader* InShader) const
	{
		auto BindingCategory = MakeShared<PropertyCategory>(const_cast<ShaderAsset*>(this), LOCALIZATION("Bindings"));
		TArray<TSharedRef<PropertyData>> BindingProperties;
		BindingProperties.Add(BindingCategory);
		if (!InShader || !InShader->IsCompiled())
		{
			return BindingProperties;
		}

		TArray<GpuShaderLayoutBinding> Bindings = InShader->GetLayout();
		for (const GpuShaderLayoutBinding& Binding : Bindings)
		{
			const FString BindingTypeName = ANSI_TO_TCHAR(magic_enum::enum_name(Binding.Type).data());
			if (Binding.Type == BindingType::UniformBuffer && Binding.UbMembers.Num() > 0)
			{
				auto UniformBufferCategory = MakeShared<PropertyCategory>(const_cast<ShaderAsset*>(this), Binding.Name);
				for (const GpuShaderUbMemberInfo& Member : Binding.UbMembers)
				{
					UniformBufferCategory->AddChild(CreateBindingInfoProperty(const_cast<ShaderAsset*>(this), Member.Name, Member.Type));
				}
				BindingCategory->AddChild(UniformBufferCategory);
				continue;
			}

			auto BindingProperty = CreateBindingInfoProperty(const_cast<ShaderAsset*>(this), Binding.Name, BindingTypeName);
			BindingCategory->AddChild(BindingProperty);
		}
		return BindingProperties;
	}
}
