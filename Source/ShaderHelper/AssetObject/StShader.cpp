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
R"(void mainImage(out float4 fragColor,in float2 fragCoord)
{
    float2 uv = fragCoord/iResolution.xy;
    float3 col = 0.5 + 0.5*cos(iTime + uv.xyx + float3(0,2,4));
    fragColor = float4(col,1);
})";

const FString DefaultVertexShader =
R"(void MainVS(in uint VertID : SV_VertexID, out float4 Pos : SV_Position)
{
    float2 uv = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), uv), 0, 1);
})";


    REFLECTION_REGISTER(AddClass<StShader>("ShaderToy Shader")
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

    void StShader::PostLoad()
    {
        AssetObject::PostLoad();
        
        VertexShader = GGpuRhi->CreateShaderFromSource(ShaderType::VertexShader, DefaultVertexShader, GetFileName() + "Vs", TEXT("MainVS"));
        FString ErrorInfo;
        GGpuRhi->CrossCompileShader(VertexShader, ErrorInfo);

        PixelShader = GGpuRhi->CreateShaderFromSource(ShaderType::PixelShader, GetFullPs(), GetFileName() + "Ps", TEXT("MainPS"));
        GGpuRhi->CrossCompileShader(PixelShader, ErrorInfo);
        
        CustomUniformBuffer = CustomUniformBufferBuilder.Build();
    }

    UniformBuffer* StShader::GetBuiltInUb()
    {
        static TUniquePtr<UniformBuffer> BuiltInUb = GetBuiltInUbBuilder().Build();
        return BuiltInUb.Get();
    }

    GpuBindGroup* StShader::GetBuiltInBindGroup()
    {
        static TRefCountPtr<GpuBindGroup> BuiltInBindGroup = GpuBindGrouprBuilder{ GetBuiltInBindLayout() }
            .SetUniformBuffer("Uniform", GetBuiltInUb()->GetGpuResource())
            .Build();
        return BuiltInBindGroup;
    }

    GpuBindGroupLayout* StShader::GetBuiltInBindLayout()
    {
        static TRefCountPtr<GpuBindGroupLayout> BuiltInBindLayout = GetBuiltInBindLayoutBuilder().Build();
        return BuiltInBindLayout;
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

    GpuBindGroupLayoutBuilder& StShader::GetBuiltInBindLayoutBuilder()
    {
        static GpuBindGroupLayoutBuilder BuiltInBindLayout{ 0 };
        static int Init = [&] {
            BuiltInBindLayout
                .AddUniformBuffer("Uniform", GetBuiltInUbBuilder().GetLayoutDeclaration(), BindingShaderStage::Pixel)
                .AddTexture("iChannel0", BindingShaderStage::Pixel)
                .AddSampler("iChannel0Sampler", BindingShaderStage::Pixel)
                .AddTexture("iChannel1", BindingShaderStage::Pixel)
                .AddSampler("iChannel1Sampler", BindingShaderStage::Pixel)
                .AddTexture("iChannel2", BindingShaderStage::Pixel)
                .AddSampler("iChannel2Sampler", BindingShaderStage::Pixel)
                .AddTexture("iChannel3", BindingShaderStage::Pixel)
                .AddSampler("iChannel3Sampler", BindingShaderStage::Pixel);
            return 0;
        }();
        return BuiltInBindLayout;
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
        return GetBuiltInBindLayoutBuilder().GetCodegenDeclaration() + CustomBindGroupLayoutBuilder.GetCodegenDeclaration();
    }

    FString StShader::GetTemplateWithBinding() const
    {
		FString Template;
		FFileHelper::LoadFileToString(Template, *(PathHelper::ShaderDir() / "ShaderHelper/StShaderTemplate.hlsl"));
        return GetBinding() + Template;
    }

	FString StShader::GetFullPs() const
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
                FString MemberName = UniformData->GetDisplayName();
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
        
        CustomUniformBuffer = CustomUniformBufferBuilder.Build();
        
        OnRefreshBuilder.ExecuteIfBound();
    }
    
    template<typename UniformType>
    void StShader::AddUniform()
    {
        static int32 AddNum;
        auto CustomUniformCategory = CustomCategory->GetData("Uniform");
        if (!CustomUniformCategory)
        {
            CustomUniformCategory = MakeShared<PropertyCategory>(this, "Uniform");
            CustomCategory->AddChild(CustomUniformCategory.ToSharedRef());
        }
        FString NewUniformName = FString::Format(TEXT("Tmp{0}"), { AddNum++ });
        
        auto TypeInfoWidget = SNew(STextBlock).TextStyle(&FAppCommonStyle::Get().GetWidgetStyle<FTextBlockStyle>("MinorText"));
        FText TypeInfo = FText::FromString(FString{ANSI_TO_TCHAR(UniformBufferMemberTypeString<UniformType>::Value.data())});
        TypeInfoWidget->SetText(TypeInfo);
        auto NewUniformProperty = MakeShared<PropertyItem<UniformType>>(this, NewUniformName);
        NewUniformProperty->SetEmbedWidget(TypeInfoWidget);
        NewUniformProperty->SetOnDisplayNameChanged([this](const FString&){
            RefreshBuilder();
            MarkDirty();
        });
        //Avoid self/cycle ref
        NewUniformProperty->SetOnDelete([this, self = &*NewUniformProperty]{
            self->Remove();
            if(self->GetParent()->GetChildrenNum() <= 0)
            {
                self->GetParent()->Remove();
            }
            RefreshBuilder();
            MarkDirty();
            static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
        });
        
        CustomUniformCategory->AddChild(MoveTemp(NewUniformProperty));
        
        RefreshBuilder();
        MarkDirty();
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->RefreshProperty();
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
        return MenuBuilder.MakeWidget();
    }

    TArray<TSharedRef<PropertyData>> StShader::PropertyDatasFromUniform(const UniformBufferBuilder& InBuilder, bool Enabled)
    {
        TArray<TSharedRef<PropertyData>> Datas;
        const UniformBufferMetaData& MetaData = InBuilder.GetMetaData();
        for(const auto& [MemberName, MemberInfo]: MetaData.Members)
        {
            auto TypeInfoWidget = SNew(STextBlock).TextStyle(&FAppCommonStyle::Get().GetWidgetStyle<FTextBlockStyle>("MinorText"));
            FText TypeInfo = FText::FromString(MemberInfo.TypeName);
            TypeInfoWidget->SetText(TypeInfo);
            TSharedPtr<PropertyItemBase> Property;
            if(MemberInfo.TypeName == "float")
            {
                Property = MakeShared<PropertyItem<float>>(this, MemberName);
            }
            else if(MemberInfo.TypeName == "float2")
            {
                Property = MakeShared<PropertyItem<Vector2f>>(this, MemberName);
            }
            else
            {
                check(false);
            }
            Property->SetEnabled(Enabled);
            Property->SetEmbedWidget(TypeInfoWidget);
            Property->SetOnDisplayNameChanged([this](const FString&){
                RefreshBuilder();
                MarkDirty();
            });
            if(Enabled) {
                Property->SetOnDelete([this, self = &*Property]{
                    self->Remove();
                    if(self->GetParent()->GetChildrenNum() <= 0)
                    {
                        self->GetParent()->Remove();
                    }
                    RefreshBuilder();
                    MarkDirty();
                    static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
                });
            }
            Datas.Add(Property.ToSharedRef());
        }
        return Datas;
    }

    TArray<TSharedRef<PropertyData>> StShader::PropertyDatasFromBinding()
    {
        auto BuiltInCategory = MakeShared<PropertyCategory>(this, "Built In");
        {
            const GpuBindGroupLayoutDesc& BuiltInLayoutDesc = GetBuiltInBindLayoutBuilder().GetLayoutDesc();
            for(const auto& [BindingName, Slot] : BuiltInLayoutDesc.CodegenBindingNameToSlot)
            {
                if(BuiltInLayoutDesc.GetBindingType(Slot) == BindingType::UniformBuffer)
                {
                    auto UniformCategory = MakeShared<PropertyCategory>(this, BindingName);
                    auto PropertyDataUniforms = PropertyDatasFromUniform(GetBuiltInUbBuilder(), false);
                    for(auto Data : PropertyDataUniforms)
                    {
                        UniformCategory->AddChild(Data);
                    }
                    BuiltInCategory->AddChild(MoveTemp(UniformCategory));
                }
            }
            auto SlotCategory = MakeShared<PropertyCategory>(this, "Slot");
            {
                auto iChannel0Item = MakeShared<PropertyItemBase>(this, "iChannel0");
                iChannel0Item->SetEnabled(false);
                auto iChannel1Item = MakeShared<PropertyItemBase>(this, "iChannel1");
                iChannel1Item->SetEnabled(false);
                auto iChannel2Item = MakeShared<PropertyItemBase>(this, "iChannel2");
                iChannel2Item->SetEnabled(false);
                auto iChannel3Item = MakeShared<PropertyItemBase>(this, "iChannel3");
                iChannel3Item->SetEnabled(false);
                SlotCategory->AddChild(MoveTemp(iChannel0Item));
                SlotCategory->AddChild(MoveTemp(iChannel1Item));
                SlotCategory->AddChild(MoveTemp(iChannel2Item));
                SlotCategory->AddChild(MoveTemp(iChannel3Item));
            }
            BuiltInCategory->AddChild(MoveTemp(SlotCategory));
        }
        
        CustomCategory = MakeShared<PropertyCategory>(this, "Custom");
        {
            const GpuBindGroupLayoutDesc& CustomLayoutDesc = CustomBindGroupLayoutBuilder.GetLayoutDesc();
            for(const auto& [BindingName, Slot] : CustomLayoutDesc.CodegenBindingNameToSlot)
            {
                if(CustomLayoutDesc.GetBindingType(Slot) == BindingType::UniformBuffer)
                {
                    auto UniformCategory = MakeShared<PropertyCategory>(this, BindingName);
                    auto PropertyDataUniforms = PropertyDatasFromUniform(CustomUniformBufferBuilder, true);
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
