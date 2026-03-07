#include "CommonHeader.h"
#include "StShader.h"
#include "ShaderHeader.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "Common/Path/PathHelper.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/MessageDialog/SMessageDialog.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"
#include "RenderResource/PrintBuffer.h"
#include "AssetManager/AssetManager.h"

using namespace FW;

namespace SH
{
const FString DefaultPixelShaderBody =
R"(void mainImage(out float4 fragColor,in float2 fragCoord)
{
    float2 uv = fragCoord / iResolution.xy;
    // Assert(uv.x > 0.9, "uv.x must be greater than 0.9");
    float3 col = 0.5 + 0.5*cos(iTime + uv.xyx + float3(0,2,4));
    
    fragColor = float4(uv,0,1);
    
    PrintAtMouse("fragColor:{0}", fragColor);
})";

const FString DefaultGLSLPixelShaderBody =
R"(void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec2 uv = fragCoord / iResolution.xy;
    // Assert(uv.x > 0.9, "uv.x must be greater than 0.9");
    vec3 col = 0.5 + 0.5*cos(iTime + uv.xyx + vec3(0,2,4));
    
    fragColor = vec4(uv,0,1);
    
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
							.Data<&StShader::Language, MetaInfo::Property>(LOCALIZATION("Language"))
	)

	StShader::StShader()
		: Language(GpuShaderLanguage::HLSL)
	{
        EditorContent = DefaultPixelShaderBody;
	}

	StShader::~StShader()
	{

	}

	void StShader::Serialize(FArchive& Ar)
	{
		ShaderAsset::Serialize(Ar);
		Ar << Language << CubeChannelMask;
        Ar << CustomUniformBufferBuilder << CustomBindGroupLayoutBuilder;
	}

    void StShader::PostLoad()
    {
        AssetObject::PostLoad();
        
        Shader = GGpuRhi->CreateShaderFromSource({
            .Name = GetShaderName(),
            .Source = GetFullContent(),
            .Type = ShaderType::PixelShader,
            .EntryPoint = "MainPS",
			.Language = Language,
			.IncludeDirs = GetIncludeDirs(),
			.IncludeHandler = [](const FString& IncludePath) -> FString {
				if (FPaths::GetExtension(IncludePath) == TEXT("header"))
				{
					AssetPtr<ShaderHeader> HeaderAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderHeader>(IncludePath);
					return HeaderAsset ? HeaderAsset->GetFullContent() : "";
				}
				FString Content;
				FFileHelper::LoadFileToString(Content, *IncludePath);
				return Content;
			},
        });
		FString ErrorInfo, WarnInfo;
		Shader->CompilerFlag |= GpuShaderCompilerFlag::SkipCache;
		bCompilationSucceed = GGpuRhi->CompileShader(Shader, ErrorInfo, WarnInfo);
		Shader->CompilerFlag &= ~GpuShaderCompilerFlag::SkipCache;
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

    UniformBufferBuilder& StShader::GetBuiltInUbBuilder()
    {
        static UniformBufferBuilder BuiltInUbLayout{ UniformBufferUsage::Persistant };
        static int Init = [&] {
			BuiltInUbLayout
				.AddVector3f("iResolution")
				.AddFloat("iTime")
				.AddFloat("iTimeDelta")
				.AddInt("iFrame")
				.AddVector3fArray("iChannelResolution", 4)
				.AddVector4f("iMouse")
				.AddInt("iFlipY");
            return 0;
        }();
        return BuiltInUbLayout;
    }

	GpuBindGroupLayoutBuilder StShader::GetBuiltInBindLayoutBuilder(uint8 CubeChannelMask) const
	{
		GpuBindGroupLayoutBuilder Builder{ BindingContext::GlobalSlot };
		Builder
			.AddExistingBinding(0, BindingType::RWRawBuffer, BindingShaderStage::Pixel)
			.AddUniformBuffer("BuiltInUniform", GetBuiltInUbBuilder(), BindingShaderStage::Pixel);
		for (int i = 0; i < 4; i++)
		{
			FString TexName = FString::Printf(TEXT("iChannel%d_texture"), i);
			FString SamplerName = FString::Printf(TEXT("iChannel%d_sampler"), i);
			if (CubeChannelMask & (1 << i))
				Builder.AddTextureCube(TexName, BindingShaderStage::Pixel);
			else
				Builder.AddTexture(TexName, BindingShaderStage::Pixel);
			Builder.AddSampler(SamplerName, BindingShaderStage::Pixel);
		}
		return Builder;
	}

	TRefCountPtr<GpuBindGroupLayout> StShader::GetBuiltInBindLayout(uint8 CubeChannelMask)
	{
		return GetBuiltInBindLayoutBuilder(CubeChannelMask).Build();
	}

	FString StShader::FileExtension() const
	{
		return "stShader";
	}

	const FSlateBrush* StShader::GetImage() const
	{
		return FShaderHelperStyle::Get().GetBrush("AssetBrowser.Shader");
	}

    FString StShader::GetBinding(uint8 CubeChannelMask) const
    {
        FString Result = GetBuiltInBindLayoutBuilder(CubeChannelMask).GetCodegenDeclaration(Language);
        if (Language == GpuShaderLanguage::GLSL)
        {
            for (int i = 0; i < 4; i++)
            {
                if (CubeChannelMask & (1 << i))
                    Result += FString::Printf(TEXT("#define iChannel%d samplerCube(iChannel%d_texture, iChannel%d_sampler)\n"), i, i, i);
                else
                    Result += FString::Printf(TEXT("#define iChannel%d sampler2D(iChannel%d_texture, iChannel%d_sampler)\n"), i, i, i);
            }
        }
        Result += CustomBindGroupLayoutBuilder.GetCodegenDeclaration(Language);
        return Result;
    }

    FString StShader::GetTemplateWithBinding(uint8 CubeChannelMask) const
    {
		FString Template, Header;
		if (Language == GpuShaderLanguage::HLSL)
		{
			FFileHelper::LoadFileToString(Template, *(PathHelper::ShaderDir() / "ShaderHelper/StShaderTemplate.hlsl"));
		}
		else
		{
			Header = "#version 450\n";
			FFileHelper::LoadFileToString(Template, *(PathHelper::ShaderDir() / "ShaderHelper/StShaderTemplate.glsl"));
		}
        return Header + GetBinding(CubeChannelMask) + Template;
    }

	int StShader::GetExtraLineNum() const
	{
		return GetExtraLineNum(CubeChannelMask);
	}

	int StShader::GetExtraLineNum(uint8 CubeChannelMask) const
	{
		TArray<FString> AddedLines;
		int AddedLineNum = GetTemplateWithBinding(CubeChannelMask).ParseIntoArrayLines(AddedLines, false) - 1;
		return AddedLineNum;
	}

	GpuShaderSourceDesc StShader::GetShaderDesc(const FString& InContent) const
	{
		return GetShaderDesc(InContent, CubeChannelMask);
	}

	GpuShaderSourceDesc StShader::GetShaderDesc(const FString& InContent, uint8 CubeChannelMask) const
	{
		FString FinalShaderSource = GetTemplateWithBinding(CubeChannelMask) + InContent;
		auto Desc = GpuShaderSourceDesc{
			.Name = GetShaderName(),
			.Source = MoveTemp(FinalShaderSource),
			.Type = ShaderType::PixelShader,
			.EntryPoint = "MainPS",
			.Language = Language,
			.IncludeDirs = GetIncludeDirs(),
			.IncludeHandler = [](const FString& IncludePath) -> FString {
				if (FPaths::GetExtension(IncludePath) == TEXT("header"))
				{
					FScopeLock ScopeLock(&GAssetCS);
					AssetPtr<ShaderHeader> HeaderAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<ShaderHeader>(IncludePath);
					return HeaderAsset ? HeaderAsset->GetFullContent() : "";
				}
				FString Content;
				FFileHelper::LoadFileToString(Content, *IncludePath);
				return Content;
			},
		};
		return Desc;
	}

	FString StShader::GetFullContent() const
	{
        FString FullShader = GetTemplateWithBinding(CubeChannelMask) + EditorContent;
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
                FString MemberName = UniformData->GetDisplayName().ToString();
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
            NewCustomBindGroupLayoutBuilder.AddUniformBuffer("CustomUniform", NewCustomUniformBufferBuilder, BindingShaderStage::Pixel);
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
            if(Data->GetDisplayName().ToString() == InName)
            {
                return true;
            }
        }
        auto BuiltInSlotCategory = BuiltInCategory->GetData("Slot");
        BuiltInSlotCategory->GetChildren(BindingDatas);
        for(const auto& Data: BindingDatas)
        {
            if(Data->GetDisplayName().ToString() == InName)
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
                if(Data->GetDisplayName().ToString() == InName)
                {
                    return true;
                }
            }
        }
        
        return false;
    }

    TSharedPtr<PropertyData> StShader::CreateUniformPropertyData(const TAttribute<FText>& InTypeName, const FString& UniformMemberName, bool Enabled)
    {
        auto TypeInfoWidget = SNew(STextBlock).TextStyle(&FAppCommonStyle::Get().GetWidgetStyle<FTextBlockStyle>("MinorText"));
        TypeInfoWidget->SetText(InTypeName);
        
        TSharedPtr<PropertyItemBase> NewUniformProperty;
        if(InTypeName.Get().ToString() == "float")
        {
            NewUniformProperty = MakeShared<PropertyScalarItem<float>>(this, UniformMemberName);
        }
		else if (InTypeName.Get().ToString() == "int")
		{
			NewUniformProperty = MakeShared<PropertyScalarItem<int32>>(this, UniformMemberName);
		}
        else if(InTypeName.Get().ToString() == "float2" || InTypeName.Get().ToString() == "vec2")
        {
            NewUniformProperty = MakeShared<PropertyVector2fItem>(this, UniformMemberName);
        }
		else if(InTypeName.Get().ToString() == "float3" || InTypeName.Get().ToString() == "vec3")
		{
			NewUniformProperty = MakeShared<PropertyVector3fItem>(this, UniformMemberName);
		}
		else if (InTypeName.Get().ToString() == "float4" || InTypeName.Get().ToString() == "vec4")
		{
			NewUniformProperty = MakeShared<PropertyVector4fItem>(this, UniformMemberName);
		}
        else
        {
			NewUniformProperty = MakeShared<PropertyItemBase>(this, UniformMemberName);
        }
        NewUniformProperty->SetEnabled(Enabled);
        NewUniformProperty->SetEmbedWidget(TypeInfoWidget);
        NewUniformProperty->SetCanApplyName([this](const FText& NewUniformMemberName){
            if(!HasBindingName(NewUniformMemberName.ToString()))
            {
                return true;
            }
            else
            {
                MessageDialog::Open(MessageDialog::Ok, MessageDialog::Sad, GApp->GetEditor()->GetMainWindow(), LOCALIZATION("DuplicateBindingName"));
                return false;
            }
        });
        NewUniformProperty->SetOnDisplayNameChanged([this](const FText&){
            RefreshBuilder();
            MarkDirty();
        });
        //Avoid self/cycle ref
        NewUniformProperty->SetOnDelete([this, Self = &*NewUniformProperty]{
			Self->Remove();
            if(Self->GetParent()->GetChildrenNum() <= 0)
            {
				Self->GetParent()->Remove();
            }
            RefreshBuilder();
            MarkDirty();
            static_cast<ShaderHelperEditor*>(GApp->GetEditor())->RefreshProperty();
        });
        
        return NewUniformProperty;
    }
    
    void StShader::AddUniform(TAttribute<FText> TypeName)
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
		auto Float = TAttribute<FText>::Create([] { return FText::FromString("float"); });
        MenuBuilder.AddMenuEntry(
			Float,
            FText::GetEmpty(),
            FSlateIcon(),
            FUIAction{ FExecuteAction::CreateRaw(this, &StShader::AddUniform, Float) }
        );
		auto Float2 = TAttribute<FText>::Create([this] { return Language == GpuShaderLanguage::HLSL ? FText::FromString("float2") : FText::FromString("vec2"); });
        MenuBuilder.AddMenuEntry(
			Float2,
            FText::GetEmpty(),
            FSlateIcon(),
            FUIAction{ FExecuteAction::CreateRaw(this, &StShader::AddUniform, Float2) }
        );
		auto Float3 = TAttribute<FText>::Create([this] { return Language == GpuShaderLanguage::HLSL ? FText::FromString("float3") : FText::FromString("vec3"); });
		MenuBuilder.AddMenuEntry(
			Float3,
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction{ FExecuteAction::CreateRaw(this, &StShader::AddUniform, Float3) }
		);
		auto Float4 = TAttribute<FText>::Create([this] { return Language == GpuShaderLanguage::HLSL ? FText::FromString("float4") : FText::FromString("vec4"); });
		MenuBuilder.AddMenuEntry(
			Float4,
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction{ FExecuteAction::CreateRaw(this, &StShader::AddUniform, Float4) }
		);
	/*	TAttribute<FText> Array = FText::FromString("Array");
		MenuBuilder.AddMenuEntry(
			Array,
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction{ FExecuteAction::CreateRaw(this, &StShader::AddUniform, Array) }
		);*/
        return MenuBuilder.MakeWidget();
    }

    TArray<TSharedRef<PropertyData>> StShader::PropertyDatasFromUniform(const UniformBufferBuilder& InBuilder, bool Enabled)
    {
        TArray<TSharedRef<PropertyData>> Datas;
        const UniformBufferMetaData& MetaData = InBuilder.GetMetaData();
        for(const auto& [MemberName, MemberInfo]: MetaData.Members)
        {
			auto TypeName = TAttribute<FText>::Create([MemberInfo, this] {
				return FText::FromString(MemberInfo.GetTypeName(Language));
			});
            auto Property = CreateUniformPropertyData(TypeName, MemberName, Enabled);
            Datas.Add(Property.ToSharedRef());
        }
        return Datas;
    }

    TArray<TSharedRef<PropertyData>> StShader::PropertyDatasFromBinding()
    {
        BuiltInCategory = MakeShared<PropertyCategory>(this, LOCALIZATION("Builtin"));
        {
            {
                auto UniformCategory = MakeShared<PropertyCategory>(this, "BuiltInUniform");
                auto PropertyDataUniforms = PropertyDatasFromUniform(GetBuiltInUbBuilder(), false);
                for(auto Data : PropertyDataUniforms)
                {
                    UniformCategory->AddChild(Data);
                }
                BuiltInCategory->AddChild(MoveTemp(UniformCategory));
            }
            auto SlotCategory = MakeShared<PropertyCategory>(this, "Slot");
            {
                TMap<FString, TSharedPtr<void>> SlotTypeEntries;
                SlotTypeEntries.Add("Texture2d", MakeShared<ShaderToySlotType>(ShaderToySlotType::Texture2D));
                SlotTypeEntries.Add("TextureCube", MakeShared<ShaderToySlotType>(ShaderToySlotType::TextureCube));

                for (int i = 0; i < 4; i++)
                {
                    FString ChannelName = FString::Printf(TEXT("iChannel%d"), i);
                    auto EnumValueName = MakeShared<FString>((CubeChannelMask & (1 << i)) ? "TextureCube" : "Texture2d");
                    auto Setter = [this, i](void* NewValue) {
                        ShaderToySlotType NewType = *static_cast<ShaderToySlotType*>(NewValue);
                        if (NewType == ShaderToySlotType::TextureCube)
                            CubeChannelMask |= (1 << i);
                        else
                            CubeChannelMask &= ~(1 << i);
                    };
                    auto ChannelItem = MakeShared<PropertyEnumItem>(this, FText::FromString(ChannelName), EnumValueName, SlotTypeEntries, Setter);
                    SlotCategory->AddChild(MoveTemp(ChannelItem));
                }
            }
            BuiltInCategory->AddChild(MoveTemp(SlotCategory));
        }
        
        CustomCategory = MakeShared<PropertyCategory>(this, LOCALIZATION("Custom"));
        {
            const GpuBindGroupLayoutDesc& CustomLayoutDesc = CustomBindGroupLayoutBuilder.GetDesc();
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

	bool StShader::CanChangeProperty(FW::PropertyData* InProperty)
	{
		if (InProperty->IsOfType<PropertyEnumItem>() && InProperty->GetDisplayName().EqualTo(LOCALIZATION("Language")))
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			auto Ret = MessageDialog::Open(MessageDialog::OkCancel, MessageDialog::Shocked, ShEditor->GetMainWindow(), LOCALIZATION("ShaderLanguageTip"));
			if (Ret == MessageDialog::MessageRet::Cancel)
			{
				return false;
			}
		}
		return true;
	
	}
	void StShader::PostPropertyChanged(FW::PropertyData* InProperty)
	{
		if (InProperty->IsOfType<PropertyEnumItem>() && InProperty->GetDisplayName().EqualTo(LOCALIZATION("Language")))
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			GpuShaderLanguage NewLanguage = *static_cast<GpuShaderLanguage*>(static_cast<PropertyEnumItem*>(InProperty)->GetEnum());
			if (NewLanguage == GpuShaderLanguage::HLSL)
			{
				ShEditor->GetShaderEditor(this)->SetText(FText::FromString(DefaultPixelShaderBody));
			}
			else if(NewLanguage == GpuShaderLanguage::GLSL)
			{
				ShEditor->GetShaderEditor(this)->SetText(FText::FromString(DefaultGLSLPixelShaderBody));
			}
		}
		else if (InProperty->IsOfType<PropertyEnumItem>())
		{
			FString DisplayName = InProperty->GetDisplayName().ToString();
			if (DisplayName.StartsWith("iChannel"))
			{
				OnRefreshBuilder.Broadcast();
				MarkDirty();
			}
		}
	}

	TArray<TSharedRef<PropertyData>>* StShader::GetPropertyDatas()
    {
		if (PropertyDatas.IsEmpty())
		{
			ShObject::GetPropertyDatas();
			PropertyDatas.Append(PropertyDatasFromBinding());
		}
		return &PropertyDatas;
    }

}
