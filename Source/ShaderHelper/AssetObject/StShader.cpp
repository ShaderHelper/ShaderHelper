#include "CommonHeader.h"
#include "StShader.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "Common/Path/PathHelper.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"
#include "RenderResource/PrintBuffer.h"

using namespace FW;

namespace SH
{
const FString DefaultPixelShaderBody =
R"(void mainImage(out float4 fragColor,in float2 fragCoord)
{
    float2 uv = fragCoord / iResolution.xy;
	//Assert(uv.x > 0.9, "uv.x must be greater than 0.9");
    float3 col = 0.5 + 0.5*cos(iTime + uv.xyx + float3(0,2,4));
    
    fragColor = float4(uv,0,1);
    
    PrintAtMouse("fragColor:{0}", fragColor);
})";

const FString DefaultVertexShader =
R"(void MainVS(in uint VertID : SV_VertexID, out float4 Pos : SV_Position)
{
    float2 uv = float2(uint2(VertID, VertID << 1) & 2);
    Pos = float4(lerp(float2(-1, 1), float2(1, -1), uv), 0, 1);
})";


    REFLECTION_REGISTER(AddClass<StShader>("ShaderToy Shader")
                                .BaseClass<ShaderAsset>()
	)

	StShader::StShader()
	{
        EditorContent = DefaultPixelShaderBody;
	}

	StShader::~StShader()
	{

	}

	void StShader::Serialize(FArchive& Ar)
	{
		ShaderAsset::Serialize(Ar);
		
        Ar << CustomUniformBufferBuilder << CustomBindGroupLayoutBuilder;
	}

    void StShader::PostLoad()
    {
        AssetObject::PostLoad();
        
        Shader = GGpuRhi->CreateShaderFromSource({
            .Name = GetFileName() + "." + FileExtension(),
            .Source = GetFullContent(),
            .Type = ShaderType::PixelShader,
            .EntryPoint = "MainPS"
        });
		FString ErrorInfo, WarnInfo;
		bCompilationSucceed = GGpuRhi->CompileShader(Shader, ErrorInfo, WarnInfo);
    }

	GpuShader* StShader::GetVertexShader()
	{
		static TRefCountPtr<GpuShader> VertexShader;
		static int Init = [&] {
			VertexShader = GGpuRhi->CreateShaderFromSource({
				.Source = DefaultVertexShader,
				.Type = ShaderType::VertexShader,
				.EntryPoint = "MainVS"
			});
			FString ErrorInfo, WarnInfo;
			GGpuRhi->CompileShader(VertexShader, ErrorInfo, WarnInfo);
			return 0;
		}();
		
		return VertexShader;
	}

    UniformBuffer* StShader::GetBuiltInUb()
    {
        static TUniquePtr<UniformBuffer> BuiltInUb = GetBuiltInUbBuilder().Build();
        return BuiltInUb.Get();
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
				.AddFloat("iTime")
				.AddVector4f("iMouse");
            return 0;
        }();
        return BuiltInUbLayout;
    }

    GpuBindGroupLayoutBuilder& StShader::GetBuiltInBindLayoutBuilder()
    {
        static GpuBindGroupLayoutBuilder BuiltInBindLayout{ BindingContext::GlobalSlot };
        static int Init = [&] {
            BuiltInBindLayout
				.AddExistingBinding(0, BindingType::RWStorageBuffer, BindingShaderStage::Pixel)
                .AddUniformBuffer("BuiltInUniform", GetBuiltInUbBuilder().GetLayoutDeclaration(), BindingShaderStage::Pixel)
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

	int StShader::GetExtraLineNum() const
	{
		TArray<FString> AddedLines;
		int AddedLineNum = GetTemplateWithBinding().ParseIntoArrayLines(AddedLines, false) - 1;
		return AddedLineNum;
	}

	GpuShaderSourceDesc StShader::GetShaderDesc(const FString& InContent) const
	{
		FString FinalShaderSource = GetTemplateWithBinding() + InContent;
		FString ShaderName = GetFileName() + "." + FileExtension();
		auto Desc = GpuShaderSourceDesc{
			.Name = ShaderName,
			.Source = MoveTemp(FinalShaderSource),
			.Type = ShaderType::PixelShader,
			.EntryPoint = "MainPS"
		};
		return Desc;
	}

	FString StShader::GetFullContent() const
	{
        FString FullShader = GetTemplateWithBinding() + EditorContent;
		return FullShader;
	}

    void StShader::RefreshBuilder()
    {
        FW::UniformBufferBuilder NewCustomUniformBufferBuilder{FW::UniformBufferUsage::Persistant};
        FW::GpuBindGroupLayoutBuilder NewCustomBindGroupLayoutBuilder{ BindingContext::PassSlot };
        
        auto CustomUniformCategory = CustomCategory->GetData("CustomUniform");
        if(CustomUniformCategory)
        {
            TArray<TSharedRef<PropertyData>> UniformDatas;
            CustomUniformCategory->GetChildren(UniformDatas);
            for(auto UniformData : UniformDatas)
            {
                FString MemberName = UniformData->GetDisplayName();
                if(UniformData->IsOfType<PropertyScalarItem<float>>())
                {
                    NewCustomUniformBufferBuilder.AddFloat(MemberName);
                }
                else if(UniformData->IsOfType<PropertyVector2fItem>())
                {
                    NewCustomUniformBufferBuilder.AddVector2f(MemberName);
                }
				else if(UniformData->IsOfType<PropertyVector3fItem>())
				{
					NewCustomUniformBufferBuilder.AddVector3f(MemberName);
				}
				else if (UniformData->IsOfType<PropertyVector4fItem>())
				{
					NewCustomUniformBufferBuilder.AddVector4f(MemberName);
				}
            }
            NewCustomBindGroupLayoutBuilder.AddUniformBuffer("CustomUniform", NewCustomUniformBufferBuilder.GetLayoutDeclaration(), BindingShaderStage::Pixel);
        }
        
        CustomUniformBufferBuilder = NewCustomUniformBufferBuilder;
        CustomBindGroupLayoutBuilder = NewCustomBindGroupLayoutBuilder;
        
        OnRefreshBuilder.Broadcast();
    }

    bool StShader::HasBindingName(const FString& InName)
    {
        TArray<TSharedRef<PropertyData>> BindingDatas;
        
        auto BuiltInUniformCategory = BuiltInCategory->GetData("BuiltInUniform");
        BuiltInUniformCategory->GetChildren(BindingDatas);
        for(const auto& Data: BindingDatas)
        {
            if(Data->GetDisplayName() == InName)
            {
                return true;
            }
        }
        auto BuiltInSlotCategory = BuiltInCategory->GetData("Slot");
        BuiltInSlotCategory->GetChildren(BindingDatas);
        for(const auto& Data: BindingDatas)
        {
            if(Data->GetDisplayName() == InName)
            {
                return true;
            }
        }
        
        if(CustomCategory)
        {
            auto CustomUniformCategory = CustomCategory->GetData("CustomUniform");
            CustomUniformCategory->GetChildren(BindingDatas);
            for(const auto& Data: BindingDatas)
            {
                if(Data->GetDisplayName() == InName)
                {
                    return true;
                }
            }
        }
        
        return false;
    }

    TSharedPtr<PropertyData> StShader::CreateUniformPropertyData(const FString& InTypeName, const FString& UniformMemberName, bool Enabled)
    {
        auto TypeInfoWidget = SNew(STextBlock).TextStyle(&FAppCommonStyle::Get().GetWidgetStyle<FTextBlockStyle>("MinorText"));
        FText TypeInfo = FText::FromString(InTypeName);
        TypeInfoWidget->SetText(TypeInfo);
        
        TSharedPtr<PropertyItemBase> NewUniformProperty;
        if(InTypeName == "float")
        {
            NewUniformProperty = MakeShared<PropertyScalarItem<float>>(this, UniformMemberName);
        }
        else if(InTypeName == "float2")
        {
            NewUniformProperty = MakeShared<PropertyVector2fItem>(this, UniformMemberName);
        }
		else if(InTypeName == "float3")
		{
			NewUniformProperty = MakeShared<PropertyVector3fItem>(this, UniformMemberName);
		}
		else if (InTypeName == "float4")
		{
			NewUniformProperty = MakeShared<PropertyVector4fItem>(this, UniformMemberName);
		}
        else
        {
            check(false);
        }
        NewUniformProperty->SetEnabled(Enabled);
        NewUniformProperty->SetEmbedWidget(TypeInfoWidget);
        NewUniformProperty->SetCanApplyName([this](const FString& NewUniformMemberName){
            if(!HasBindingName(NewUniformMemberName))
            {
                return true;
            }
            else
            {
                MessageDialog::Open(MessageDialog::Ok, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("DuplicateBindingName"));
                return false;
            }
        });
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
        
        return NewUniformProperty;
    }
    
    void StShader::AddUniform(FString TypeName)
    {
        auto CustomUniformCategory = CustomCategory->GetData("CustomUniform");
        if (!CustomUniformCategory)
        {
            CustomUniformCategory = MakeShared<PropertyCategory>(this, "CustomUniform");
            CustomCategory->AddChild(CustomUniformCategory.ToSharedRef());
        }
        
        int Number = 0;
        FString NewUniformName = FString::Format(TEXT("Tmp{0}"), { Number++ });
        while(HasBindingName(NewUniformName))
        {
            NewUniformName = FString::Format(TEXT("Tmp{0}"), { Number++ });
        }
        
        auto NewUniformPropertyData = CreateUniformPropertyData(TypeName, NewUniformName, true);
        CustomUniformCategory->AddChild(NewUniformPropertyData.ToSharedRef());
        
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
            FUIAction{ FExecuteAction::CreateRaw(this, &StShader::AddUniform, FString("float")) }
        );
        MenuBuilder.AddMenuEntry(
            FText::FromString("float2"),
            FText::GetEmpty(),
            FSlateIcon(),
            FUIAction{ FExecuteAction::CreateRaw(this, &StShader::AddUniform, FString("float2")) }
        );
		MenuBuilder.AddMenuEntry(
			FText::FromString("float3"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction{ FExecuteAction::CreateRaw(this, &StShader::AddUniform, FString("float3")) }
		);
		MenuBuilder.AddMenuEntry(
			FText::FromString("float4"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction{ FExecuteAction::CreateRaw(this, &StShader::AddUniform, FString("float4")) }
		);
        return MenuBuilder.MakeWidget();
    }

    TArray<TSharedRef<PropertyData>> StShader::PropertyDatasFromUniform(const UniformBufferBuilder& InBuilder, bool Enabled)
    {
        TArray<TSharedRef<PropertyData>> Datas;
        const UniformBufferMetaData& MetaData = InBuilder.GetMetaData();
        for(const auto& [MemberName, MemberInfo]: MetaData.Members)
        {
            auto Property = CreateUniformPropertyData(MemberInfo.TypeName, MemberName, Enabled);
            Datas.Add(Property.ToSharedRef());
        }
        return Datas;
    }

    TArray<TSharedRef<PropertyData>> StShader::PropertyDatasFromBinding()
    {
        BuiltInCategory = MakeShared<PropertyCategory>(this, "Built In");
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
        return {BuiltInCategory.ToSharedRef(), CustomCategory.ToSharedRef()};
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
