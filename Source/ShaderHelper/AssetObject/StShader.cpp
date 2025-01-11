#include "CommonHeader.h"
#include "StShader.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "Common/Path/PathHelper.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"

using namespace FW;

namespace SH
{
const FString DefaultPixelShaderBody =
R"(void mainImage(out float4 fragColor, in float2 fragCoord)
{
    float2 uv = fragCoord/iResolution.xy;
    float3 col = 0.5 + 0.5*cos(iTime + uv.xyx + float3(0,2,4));
    fragColor = float4(col,1);
})";

	GLOBAL_REFLECTION_REGISTER(AddClass<StShader>("ShaderToy Shader")
                                .BaseClass<AssetObject>()
	)

	StShader::StShader()
	{
        PixelShaderBody = DefaultPixelShaderBody;
	}

	StShader::~StShader()
	{

	}

	void StShader::Serialize(FArchive& Ar)
	{
		AssetObject::Serialize(Ar);

		Ar << PixelShaderBody;
        Ar << CustomUniformBufferBuilder << CustomBindGroupLayoutBuilder;
	}

    UniformBufferBuilder& StShader::GetBuiltInUbBuilder()
    {
        static UniformBufferBuilder BuiltInUbLayout{ UniformBufferUsage::Persistant };
        static int Init = [&] {
            BuiltInUbLayout
                .AddVector2f("iResolution")
                .AddFloat("iTime");
            return 0;
        }();
        return BuiltInUbLayout;
    }

    GpuBindGroupLayoutBuilder& StShader::GetBuiltInBindingLayoutBuilder()
    {
        static GpuBindGroupLayoutBuilder BuiltInBindingLayout{ 0 };
        static int Init = [&] {
            BuiltInBindingLayout
                .AddUniformBuffer("Uniform", GetBuiltInUbBuilder().GetLayoutDeclaration(), BindingShaderStage::Pixel);
            return 0;
        }();
        return BuiltInBindingLayout;
    }

	FString StShader::FileExtension() const
	{
		return "stShader";
	}

	const FSlateBrush* StShader::GetImage() const
	{
		return FShaderHelperStyle::Get().GetBrush("AssetBrowser.Shader");
	}

    FString StShader::GetBinding() const
    {
        return GetBuiltInBindingLayoutBuilder().GetCodegenDeclaration() + CustomBindGroupLayoutBuilder.GetCodegenDeclaration();
    }

    FString StShader::GetTemplateWithBinding() const
    {
		FString Template;
		FFileHelper::LoadFileToString(Template, *(PathHelper::ShaderDir() / "ShaderHelper/StShaderTemplate.hlsl"));
        return GetBinding() + Template;
    }

	FString StShader::GetFullShader() const
	{
		FString Template;
		FFileHelper::LoadFileToString(Template, *(PathHelper::ShaderDir() / "ShaderHelper/StShaderTemplate.hlsl"));
        FString FullShader = GetBinding() + Template + PixelShaderBody;
		return FullShader;
	}

    void StShader::RefreshBuilder()
    {
        FW::UniformBufferBuilder NewCustomUniformBufferBuilder{FW::UniformBufferUsage::Persistant};
        FW::GpuBindGroupLayoutBuilder NewCustomBindGroupLayoutBuilder{1};
        
        auto CustomUniformCategory = CustomCategory->GetData("Uniform");
        if(CustomUniformCategory)
        {
            TArray<TSharedRef<PropertyData>> UniformDatas;
            CustomUniformCategory->GetChildren(UniformDatas);
            for(auto UniformData : UniformDatas)
            {
                FString MemberName, _;
                UniformData->GetDisplayName().Split(" ", &MemberName, &_);
                if(UniformData->IsOfType<PropertyItem<float>>())
                {
                    NewCustomUniformBufferBuilder.AddFloat(MemberName);
                }
                else if(UniformData->IsOfType<PropertyItem<Vector2f>>())
                {
                    NewCustomUniformBufferBuilder.AddVector2f(MemberName);
                }
            }
            NewCustomBindGroupLayoutBuilder.AddUniformBuffer("Uniform", NewCustomUniformBufferBuilder.GetLayoutDeclaration(), BindingShaderStage::Pixel);
        }
        
        CustomUniformBufferBuilder = NewCustomUniformBufferBuilder;
        CustomBindGroupLayoutBuilder = NewCustomBindGroupLayoutBuilder;
    }
    
    template<typename UniformType>
    void StShader::AddUniform()
    {
        static int32 AddNum;
        auto CustomUniformCategory = CustomCategory->GetData("Uniform");
        if (!CustomUniformCategory)
        {
            CustomUniformCategory = MakeShared<PropertyCategory>("Uniform");
            CustomCategory->AddChild(CustomUniformCategory.ToSharedRef());
        }
        FString NewUniformName = FString::Format(TEXT("Tmp{0}"), { AddNum++ });
        FString TypeInfo = FString::Printf(TEXT(" (%s)"), ANSI_TO_TCHAR(UniformBufferMemberTypeString<UniformType>::Value.data()));
        auto NewUniformProperty = MakeShared<PropertyItem<UniformType>>(NewUniformName + TypeInfo);
        CustomUniformCategory->AddChild(MoveTemp(NewUniformProperty));
        RefreshBuilder();
        
        MarkDirty();
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->RefreshProperty();
    }

    void StShader::AddSlot()
    {
        
    }

    TSharedRef<SWidget> StShader::GetCategoryMenu()
    {
        FMenuBuilder MenuBuilder{ true, TSharedPtr<FUICommandList>() };
        MenuBuilder.AddMenuEntry(
            FText::FromString("float"),
            FText::GetEmpty(),
            FSlateIcon(),
            FUIAction{ FExecuteAction::CreateRaw(this, &StShader::AddUniform<float>) })
        ;
        MenuBuilder.AddMenuEntry(
            FText::FromString("float2"),
            FText::GetEmpty(),
            FSlateIcon(),
            FUIAction{ FExecuteAction::CreateRaw(this, &StShader::AddUniform<Vector2f>) }
        );
        MenuBuilder.AddMenuEntry(
            FText::FromString("slot"),
            FText::GetEmpty(),
            FSlateIcon(),
            FUIAction{ FExecuteAction::CreateRaw(this, &StShader::AddSlot) }
        );
        return MenuBuilder.MakeWidget();
    }

    TArray<TSharedRef<PropertyData>> StShader::PropertyDatasFromUniform(const UniformBufferBuilder& InBuilder)
    {
        TArray<TSharedRef<PropertyData>> Datas;
        const UniformBufferMetaData& MetaData = InBuilder.GetMetaData();
        for(const auto& [MemberName, MemberInfo]: MetaData.Members)
        {
            FString TypeInfo = FString::Printf(TEXT(" (%s)"), *MemberInfo.TypeName);
            if(MemberInfo.TypeName == "float")
            {
                auto UniformProperty = MakeShared<PropertyItem<float>>(MemberName + TypeInfo);
                Datas.Add(UniformProperty);
            }
            else if(MemberInfo.TypeName == "float2")
            {
                auto UniformProperty = MakeShared<PropertyItem<Vector2f>>(MemberName + TypeInfo);
                Datas.Add(UniformProperty);
            }
        }
        return Datas;
    }

    TArray<TSharedRef<PropertyData>> StShader::PropertyDatasFromBinding()
    {
        auto BuiltInCategory = MakeShared<PropertyCategory>("Built In");
        {
            const GpuBindGroupLayoutDesc& BuiltInLayoutDesc = GetBuiltInBindingLayoutBuilder().GetLayoutDesc();
            for(const auto& [BindingName, Slot] : BuiltInLayoutDesc.CodegenBindingNameToSlot)
            {
                if(BuiltInLayoutDesc.GetBindingType(Slot) == BindingType::UniformBuffer)
                {
                    auto UniformCategory = MakeShared<PropertyCategory>(BindingName);
                    auto PropertyDataUniforms = PropertyDatasFromUniform(GetBuiltInUbBuilder());
                    for(auto Data : PropertyDataUniforms)
                    {
                        UniformCategory->AddChild(Data);
                    }
                    BuiltInCategory->AddChild(MoveTemp(UniformCategory));
                }
            }
        }
        
        CustomCategory = MakeShared<PropertyCategory>("Custom");
        {
            const GpuBindGroupLayoutDesc& CustomLayoutDesc = CustomBindGroupLayoutBuilder.GetLayoutDesc();
            for(const auto& [BindingName, Slot] : CustomLayoutDesc.CodegenBindingNameToSlot)
            {
                if(CustomLayoutDesc.GetBindingType(Slot) == BindingType::UniformBuffer)
                {
                    auto UniformCategory = MakeShared<PropertyCategory>(BindingName);
                    auto PropertyDataUniforms = PropertyDatasFromUniform(CustomUniformBufferBuilder);
                    for(auto Data : PropertyDataUniforms)
                    {
                        UniformCategory->AddChild(Data);
                    }
                    CustomCategory->AddChild(MoveTemp(UniformCategory));
                }
            }
        }
        CustomCategory->SetAddMenuWidget(GetCategoryMenu());
        return {BuiltInCategory, CustomCategory.ToSharedRef()};
    }

    TArray<TSharedRef<PropertyData>>* StShader::GetPropertyDatas()
    {
        if(PropertyDatas.IsEmpty())
        {
            PropertyDatas = PropertyDatasFromBinding();
        }
        return &PropertyDatas;
    }

}
