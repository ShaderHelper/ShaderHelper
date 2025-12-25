#include "CommonHeader.h"
#include "ShaderToyPassNode.h"
#include "Renderer/ShaderToyRenderComp.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/Property/PropertyData/PropertyUniformItem.h"
#include "RenderResource/PrintBuffer.h"
#include "Editor/AssetEditor/AssetEditor.h"
#include "AssetObject/Pins/Pins.h"
#include "UI/Widgets/Property/PropertyData/PropertyAssetItem.h"
#include "ShaderConductor.hpp"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"

#include <regex>

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<ShaderToyPassNode>("ShaderPass Node")
		.BaseClass<GraphNode>()
        .Data<&ShaderToyPassNode::ShaderAssetObj, MetaInfo::Property>(LOCALIZATION("Shader"))
		.Data<&ShaderToyPassNode::Format, MetaInfo::Property>(LOCALIZATION("Format"))
		.Data<&ShaderToyPassNode::iChannelDesc0>("iChannel0")
		.Data<&ShaderToyPassNode::iChannelDesc1>("iChannel1")
		.Data<&ShaderToyPassNode::iChannelDesc2>("iChannel2")
		.Data<&ShaderToyPassNode::iChannelDesc3>("iChannel3")
	)
    REFLECTION_REGISTER(AddClass<ShaderToyPassNodeOp>()
        .BaseClass<ShObjectOp>()
    )
	REFLECTION_REGISTER(AddClass<ShaderToyChannelDesc>()
		.Data<&ShaderToyChannelDesc::Filter, MetaInfo::Property>(LOCALIZATION("FilterMode"))
		.Data<&ShaderToyChannelDesc::Wrap, MetaInfo::Property>(LOCALIZATION("WrapMode"))
	)

	REGISTER_NODE_TO_GRAPH(ShaderToyPassNode, "ShaderToy Graph")

    MetaType* ShaderToyPassNodeOp::SupportType()
    {
        return GetMetaType<ShaderToyPassNode>();
    }

	void ShaderToyPassNodeOp::OnCancelSelect(ShObject* InObject)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if(!static_cast<ShaderToyPassNode*>(InObject)->IsDebugging) {
			ShEditor->SetDebuggableObject(nullptr);
		}
	}

    void ShaderToyPassNodeOp::OnSelect(ShObject* InObject)
    {
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->ShowProperty(InObject);
		ShEditor->SetDebuggableObject(static_cast<ShaderToyPassNode*>(InObject));
    }

	void ShaderToyPassNode::InitShaderAsset()
	{
		if(ShaderAssetObj)
		{
			ShaderAssetObj->OnDestroy.AddRaw(this, &ShaderToyPassNode::ClearBindingProperty);
			ShaderAssetObj->OnRefreshBuilder.AddRaw(this, &ShaderToyPassNode::RefreshProperty, true);
		}
	}

	void ShaderToyPassNode::InitCustomBindGroup()
	{
		if(ShaderAssetObj)
		{
			CustomBindLayout = ShaderAssetObj->CustomBindGroupLayoutBuilder.Build();
			CustomBindGroupBuilder = GpuBindGroupBuilder{ CustomBindLayout };
			if(CustomUniformBuffer.IsValid())
			{
				(*CustomBindGroupBuilder).SetUniformBuffer("CustomUniform", CustomUniformBuffer->GetGpuResource());
			}
			CustomBindGroup = (*CustomBindGroupBuilder).Build();
		}
	}

	ShaderToyPassNode::ShaderToyPassNode()
	: Format(ShaderToyFormat::B8G8R8A8_UNORM)
	{
		ObjectName = FText::FromString("ShaderPass");
	}

	ShaderToyPassNode::ShaderToyPassNode(FW::AssetPtr<StShader> InShaderAssetObj)
	: ShaderAssetObj(MoveTemp(InShaderAssetObj))
	, Format(ShaderToyFormat::B8G8R8A8_UNORM)
	{
		ObjectName = FText::FromString(ShaderAssetObj->GetFileName());
		InitShaderAsset();
		if(ShaderAssetObj)
		{
			CustomUniformBuffer = ShaderAssetObj->CustomUniformBufferBuilder.Build();
		}
		InitCustomBindGroup();
	}

	ShaderToyPassNode::~ShaderToyPassNode()
	{
		if(ShaderAssetObj)
		{
			ShaderAssetObj->OnDestroy.RemoveAll(this);
			ShaderAssetObj->OnRefreshBuilder.RemoveAll(this);
		}
	}

	GpuBindGroupBuilder ShaderToyPassNode::GetBuiltInBindGroupBuiler()
	{
		auto Builder = GpuBindGroupBuilder{ StShader::GetBuiltInBindLayout() }
			.SetExistingBinding(0, TSingleton<PrintBuffer>::Get().GetResource())
			.SetUniformBuffer("BuiltInUniform", StShader::GetBuiltInUb()->GetGpuResource());

		GpuTexturePin* iChannel0 = static_cast<GpuTexturePin*>(GetPin("iChannel0"));
		GpuTexturePin* iChannel1 = static_cast<GpuTexturePin*>(GetPin("iChannel1"));
		GpuTexturePin* iChannel2 = static_cast<GpuTexturePin*>(GetPin("iChannel2"));
		GpuTexturePin* iChannel3 = static_cast<GpuTexturePin*>(GetPin("iChannel3"));

		Builder.SetTexture("iChannel0_texture", iChannel0->GetValue());
		Builder.SetSampler("iChannel0_sampler", GpuResourceHelper::GetSampler({
			.Filter = (SamplerFilter)iChannelDesc0.Filter,
			.AddressU = (SamplerAddressMode)iChannelDesc0.Wrap,
			.AddressV = (SamplerAddressMode)iChannelDesc0.Wrap,
			.AddressW = (SamplerAddressMode)iChannelDesc0.Wrap
		}));
		Builder.SetTexture("iChannel1_texture", iChannel1->GetValue());
		Builder.SetSampler("iChannel1_sampler", GpuResourceHelper::GetSampler({
			.Filter = (SamplerFilter)iChannelDesc1.Filter,
			.AddressU = (SamplerAddressMode)iChannelDesc1.Wrap,
			.AddressV = (SamplerAddressMode)iChannelDesc1.Wrap,
			.AddressW = (SamplerAddressMode)iChannelDesc1.Wrap
		}));
		Builder.SetTexture("iChannel2_texture", iChannel2->GetValue());
		Builder.SetSampler("iChannel2_sampler", GpuResourceHelper::GetSampler({
			.Filter = (SamplerFilter)iChannelDesc2.Filter,
			.AddressU = (SamplerAddressMode)iChannelDesc2.Wrap,
			.AddressV = (SamplerAddressMode)iChannelDesc2.Wrap,
			.AddressW = (SamplerAddressMode)iChannelDesc2.Wrap
		}));
		Builder.SetTexture("iChannel3_texture", iChannel3->GetValue());
		Builder.SetSampler("iChannel3_sampler", GpuResourceHelper::GetSampler({
			.Filter = (SamplerFilter)iChannelDesc3.Filter,
			.AddressU = (SamplerAddressMode)iChannelDesc3.Wrap,
			.AddressV = (SamplerAddressMode)iChannelDesc3.Wrap,
			.AddressW = (SamplerAddressMode)iChannelDesc3.Wrap
		}));
		return Builder;
	}

	TRefCountPtr<FW::GpuBindGroup> ShaderToyPassNode::GetBuiltInBindGroup()
	{
		return GetBuiltInBindGroupBuiler().Build();
	}

	InvocationState ShaderToyPassNode::GetInvocationState()
	{
		auto RT = static_cast<GpuTexturePin*>(GetPin("RT"))->GetValue();
		
		return PixelState{
			.ViewPortDesc = {(float)RT->GetWidth(), (float)RT->GetHeight()},
			.Builders = BindingState{
				.GlobalBuilder = BindingBuilder{
					.BingGroupBuilder = GetBuiltInBindGroupBuiler(),
					.LayoutBuilder = StShader::GetBuiltInBindLayoutBuilder()
				},
				.PassBuilder = BindingBuilder{
					.BingGroupBuilder = *CustomBindGroupBuilder,
					.LayoutBuilder = ShaderAssetObj->CustomBindGroupLayoutBuilder,
				}
			},
			.PipelineDesc = PipelineDesc,
			.DrawFunction = [](GpuRenderPassRecorder* PassRecorder) {
				PassRecorder->DrawPrimitive(0, 3, 0, 1);
			}
		};
	}

	TRefCountPtr<GpuTexture> ShaderToyPassNode::OnStartDebugging()
	{
		AssetOp::OpenAsset(ShaderAssetObj);
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SShaderEditorBox* ShaderEditor = ShEditor->GetShaderEditor(ShaderAssetObj);
		ShaderEditor->Compile();
		//Render once immediately for debugging
		{
			TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop = false;
			ShEditor->GetRenderer()->Render();
			TSingleton<ShProjectManager>::Get().GetProject()->TimelineStop = true;
		}
		if (!AnyError || AssertError)
		{
			IsDebugging = true;
			auto PassOutput = static_cast<GpuTexturePin*>(GetPin("RT"));
			return PassOutput->GetValue();
		}
		else
		{
			return {};
		}
	}

	void ShaderToyPassNode::OnFinalizePixel(const FW::Vector2u& PixelCoord)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->InvokeDebuggerTabs();
		SShaderEditorBox* ShaderEditor = ShEditor->GetShaderEditor(ShaderAssetObj);
		ShaderEditor->DebugPixel(PixelCoord, GetInvocationState());
	}

	ShaderAsset* ShaderToyPassNode::GetShaderAsset() const
	{
		return ShaderAssetObj;
	}

	std::string ShaderToyPassNode::GetShaderToyCode() const
	{
		TArray<const char*> DxcArgs;
		DxcArgs.Add("/Od");
		DxcArgs.Add("-D");
		DxcArgs.Add("ENABLE_PRINT=0");
		DxcArgs.Add("-D");
		DxcArgs.Add("ENABLE_ASSERT=0");
		ShaderConductor::Compiler::Options SCOptions;
		SCOptions.DXCArgs = DxcArgs.GetData();
		SCOptions.numDXCArgs = DxcArgs.Num();
		GpuShaderModel Sm = ShaderAssetObj->Shader->GetShaderModelVer();
		SCOptions.shaderModel = { Sm.Major, Sm.Minor };

		ShaderConductor::Compiler::SourceDesc SourceDesc{};
		TArray<char> EntryPointAnsi{ TCHAR_TO_ANSI(*ShaderAssetObj->Shader->GetEntryPoint()), ShaderAssetObj->Shader->GetEntryPoint().Len() + 1 };
		SourceDesc.entryPoint = EntryPointAnsi.GetData();
		SourceDesc.stage = ShaderConductor::ShaderStage::PixelShader;
		auto SourceUTF8 = StringCast<UTF8CHAR>(*ShaderAssetObj->Shader->GetProcessedSourceText());
		SourceDesc.source = (char*)SourceUTF8.Get();

		SourceDesc.loadIncludeCallback = [this](const char* includeName) -> ShaderConductor::Blob {
			for (const FString& IncludeDir : ShaderAssetObj->Shader->GetIncludeDirs())
			{
				FString IncludedFile = FPaths::Combine(IncludeDir, includeName);
				if (IFileManager::Get().FileExists(*IncludedFile))
				{
					FString ShaderText;
					FFileHelper::LoadFileToString(ShaderText, *IncludedFile);
					ShaderText = GpuShaderPreProcessor{ ShaderText, ShaderAssetObj->Shader->GetShaderLanguage()}
						.ReplacePrintStringLiteral()
						.Finalize();
					auto SourceText = StringCast<UTF8CHAR>(*ShaderText);
					return ShaderConductor::Blob(SourceText.Get(), SourceText.Length() * sizeof(UTF8CHAR));
				}
			}
			return {};
		};

		ShaderConductor::Compiler::TargetDesc TargetDesc{
			.language = ShaderConductor::ShadingLanguage::Glsl,
			.version = "300"
		};

		auto Result = ShaderConductor::Compiler::Compile(SourceDesc, SCOptions, TargetDesc);
		std::string Glsl = {(char*)Result.target.Data(), Result.target.Size() };
		std::string ShaderToy;
		//Extract the valid shader segment
		std::regex StartPattern(R"(vec2\s+GPrivate_fragCoord\s*;)");
		std::smatch StartMatch;
		std::regex MainImagePattern(R"(void\s+mainImage\s*\([^)]*vec2\s+(\w+)\s*\)[^}]*\})");
		std::smatch MainImageMatch;
		std::regex MainPattern(R"(void\s+main\s*\(\s*\)\s*\{([^}]*)\})");
		std::smatch MainMatch;
		if (std::regex_search(Glsl, StartMatch, StartPattern) &&
			std::regex_search(Glsl, MainImageMatch, MainImagePattern))
		{
			size_t StartPos = StartMatch.position() + StartMatch.length();
			size_t EndPos = MainImageMatch.position() + MainImageMatch.length();
			ShaderToy = Glsl.substr(StartPos, EndPos - StartPos);
		}
		//Due to legalization by dxc, the shader may eventually be optimized to contain only the main function.
		else if (std::regex_search(Glsl, MainMatch, MainPattern))
		{
			std::string MainBody = MainMatch[1].str();
        	std::string NewMain = "void mainImage( out vec4 fragColor, in vec2 fragCoord )\n{" + MainBody + "}";
       		ShaderToy = NewMain;
			ShaderToy = std::regex_replace(ShaderToy, std::regex(R"(out_var_SV_Target)"), "fragColor");
			ShaderToy = std::regex_replace(ShaderToy, std::regex(R"(gl_FragCoord)"), "fragCoord");
		}
		//Remove ubo prefix name
		ShaderToy = std::regex_replace(ShaderToy, std::regex(R"(BuiltInUniform\.)"), "");
		ShaderToy = std::regex_replace(ShaderToy, std::regex(R"(CustomUniform\.)"), "");
		//Replace SPIRV-Cross texture sampler names with Shadertoy standard names
		ShaderToy = std::regex_replace(ShaderToy, std::regex(R"(SPIRV_Cross_Combined(iChannel\d+)\w+)"), "$1");
		//Find non-uniform global variables in the segment
		std::vector<std::string> GlobalVars;
		std::istringstream iss(ShaderToy);
		std::string line;
		bool inFunction = false;
		int braceLevel = 0;
		while (std::getline(iss, line))
		{
			std::string trimmedLine = std::regex_replace(line, std::regex(R"(^\s+|\s+$)"), "");
			if (trimmedLine.empty() ||
				trimmedLine.starts_with("//") ||
				trimmedLine.starts_with("/*") ||
				trimmedLine.starts_with("#"))
				continue;
			for (char c : trimmedLine)
			{
				if (c == '{') braceLevel++;
				else if (c == '}') braceLevel--;
			}
			if (trimmedLine.find('(') != std::string::npos && trimmedLine.find(')') != std::string::npos)
			{
				inFunction = true;
			}
			if (!inFunction && braceLevel == 0)
			{
				std::regex GlobalVarPattern(R"(^([a-zA-Z_]\w*)\s+([a-zA-Z_]\w*)\s*(?:=\s*[^;]+)?\s*;)");
				std::smatch match;
				if (std::regex_match(trimmedLine, match, GlobalVarPattern))
				{
					std::string typeName = match[1].str();
					std::string varName = match[2].str();
					if (typeName != "struct")
					{
						GlobalVars.push_back(varName);
					}
				}
			}
			if (braceLevel == 0)
			{
				inFunction = false;
			}
		}
		//Extract the global variable Initializations from "void main()"
		if (std::regex_search(Glsl, MainMatch, MainPattern))
		{
			std::string MainBody = MainMatch[1].str();
			std::map<std::string, std::string> GlobalInits;
			for (const std::string& VarName : GlobalVars)
			{
				std::string InitPattern = VarName + R"(\s*=\s*([^;]+);)";
				std::regex InitRegex(InitPattern);
				std::smatch InitMatch;
				if (std::regex_search(MainBody, InitMatch, InitRegex))
				{
					std::string InitValue = InitMatch[1].str();
					GlobalInits[VarName] = InitValue;
				}
			}
			for (const auto& [VarName, InitValue] : GlobalInits)
			{
				std::string DeclPattern = R"((\w+\s+))" + VarName + R"(\s*;)";
				std::regex DeclRegex(DeclPattern);
				std::string Replacement = "$1" + VarName + " = " + InitValue + ";";
				ShaderToy = std::regex_replace(ShaderToy, DeclRegex, Replacement);
			}
		}
		//Append custom uniform members
		if(CustomUniformBuffer)
		{
			FString MemberDecl;
			for(const auto& [MemberName, MemberInfo] : CustomUniformBuffer->GetMetaData().Members)
			{
				if(MemberInfo.HlslTypeName == "float")
				{
					float Val = CustomUniformBuffer->GetMember<float>(MemberName);
					MemberDecl += "const float " + MemberName + " = " + FString::Printf(TEXT("%f;"), Val);
				}
				else if(MemberInfo.HlslTypeName == "float2")
				{
					Vector2f Val = CustomUniformBuffer->GetMember<Vector2f>(MemberName);
					MemberDecl += "const vec2 " + MemberName + " = " + FString::Printf(TEXT("vec2(%f,%f);"), Val.x, Val.y);
				}
				else if(MemberInfo.HlslTypeName == "float3")
				{
					Vector3f Val = CustomUniformBuffer->GetMember<Vector3f>(MemberName);
					MemberDecl += "const vec3 " + MemberName + " = " + FString::Printf(TEXT("vec3(%f,%f,%f);"), Val.x, Val.y, Val.z);
				}
				else if(MemberInfo.HlslTypeName == "float4")
				{
					Vector4f Val = CustomUniformBuffer->GetMember<Vector4f>(MemberName);
					MemberDecl += "const vec4 " + MemberName + " = " + FString::Printf(TEXT("vec4(%f,%f,%f,%f);"), Val.x, Val.y, Val.z, Val.w);
				}
				else
				{
					AUX::Unreachable();
				}
				MemberDecl += "\n";
			}
			ShaderToy = TCHAR_TO_UTF8(*MemberDecl) + ShaderToy;
		}
		//Extract struct
		std::string ExtractedStructs;
		std::regex VersionPattern(R"(#version\s+\d+)");
		std::smatch VersionMatch;
		std::regex LayoutPattern(R"(layout\s*\(\s*std140\s*\))");
		std::smatch LayoutMatch;
		if (std::regex_search(Glsl, VersionMatch, VersionPattern) &&
			std::regex_search(Glsl, LayoutMatch, LayoutPattern))
		{
			size_t VersionEnd = VersionMatch.position() + VersionMatch.length();
			size_t LayoutStart = LayoutMatch.position();

			if (LayoutStart > VersionEnd)
			{
				std::string TargetSection = Glsl.substr(VersionEnd, LayoutStart - VersionEnd);

				std::regex StructPattern(R"(struct\s+(\w+)\s*\{[^}]*\}\s*;)");
				std::sregex_iterator StructBegin(TargetSection.begin(), TargetSection.end(), StructPattern);
				std::sregex_iterator StructEnd;

				for (std::sregex_iterator iter = StructBegin; iter != StructEnd; ++iter)
				{
					const std::smatch& match = *iter;
					std::string StructName = match[1].str();

					if (StructName != "Printer" && StructName != "PIn")
					{
						ExtractedStructs += match.str() + "\n";
					}
				}
			}
		}
		if (!ExtractedStructs.empty())
		{
			ShaderToy = ExtractedStructs + "\n" + ShaderToy;
		}
		//Flip y
		if (std::regex_search(ShaderToy, MainImageMatch, MainImagePattern))
		{
			std::string FragCoordParamName = MainImageMatch[1].str();
			std::regex FunctionBodyPattern(R"((void\s+mainImage\s*\([^)]*\)\s*\{)(\s*))");
			std::string Replacement = "$1$2" + FragCoordParamName + ".y = iResolution.y - " + FragCoordParamName + ".y;\n$2";
			ShaderToy = std::regex_replace(ShaderToy, FunctionBodyPattern, Replacement);
		}
		return ShaderToy;
	}

	void ShaderToyPassNode::OnEndDebuggging()
	{
		IsDebugging = false;
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if(SShaderEditorBox* ShaderEditor = ShEditor->GetShaderEditor(ShaderAssetObj))
		{
			ShaderEditor->ResetDebugger();
		}
		
        ShEditor->CloseDebuggerTabs();
	}

    void ShaderToyPassNode::InitPins()
    {
        auto Slot0 = NewShObject<GpuTexturePin>(this);
        Slot0->ObjectName = FText::FromString("iChannel0");
        Slot0->Direction = PinDirection::Input;
        auto Slot1 = NewShObject<GpuTexturePin>(this);
        Slot1->ObjectName = FText::FromString("iChannel1");
        Slot1->Direction = PinDirection::Input;
        auto Slot2 = NewShObject<GpuTexturePin>(this);
        Slot2->ObjectName = FText::FromString("iChannel2");
        Slot2->Direction = PinDirection::Input;
        auto Slot3 = NewShObject<GpuTexturePin>(this);
        Slot3->ObjectName = FText::FromString("iChannel3");
        Slot3->Direction = PinDirection::Input;
        
        auto PassOutput = NewShObject<GpuTexturePin>(this);
        PassOutput->ObjectName = FText::FromString("RT");
        PassOutput->Direction = PinDirection::Output;
        
        Pins = {PassOutput, Slot0, Slot1, Slot2, Slot3};
    }

	void ShaderToyPassNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
		
		Ar << ShaderAssetObj << Format;
		
		if(ShaderAssetObj)
		{
			if(Ar.IsLoading())
			{
				InitShaderAsset();
				CustomUniformBuffer = ShaderAssetObj->CustomUniformBufferBuilder.Build();
			}
			
			if(CustomUniformBuffer.IsValid() && Ar.IsSaving())
			{
				CustomUniformBufferData = {(uint8*)CustomUniformBuffer->GetReadableData(), (int)CustomUniformBuffer->GetSize()};
			}
			Ar << CustomUniformBufferData;
			
			if(Ar.IsSaving())
			{
				Ar << ShaderAssetObj->CustomUniformBufferBuilder;
			}
			else
			{
				FW::UniformBufferBuilder NodeCustomUniformBufferBuilder{FW::UniformBufferUsage::Persistant};
				Ar << NodeCustomUniformBufferBuilder;
				if(CustomUniformBuffer.IsValid() && !CustomUniformBufferData.IsEmpty())
				{
					//If the layout of the shader's uniform buffer does not match the node
					if(NodeCustomUniformBufferBuilder.GetMetaData().HlslDeclaration != ShaderAssetObj->CustomUniformBufferBuilder.GetMetaData().HlslDeclaration)
					{
						auto NodeCustomUniformBuffer = NodeCustomUniformBufferBuilder.Build();
						if(NodeCustomUniformBuffer.IsValid())
						{
							NodeCustomUniformBuffer->SetData(CustomUniformBufferData);
							CustomUniformBuffer->CopySameMember(*NodeCustomUniformBuffer);
						}
					}
					else
					{
						CustomUniformBuffer->SetData(CustomUniformBufferData);
					}
				}
				else
				{
					CustomUniformBufferData.Empty();
				}
				InitCustomBindGroup();
			}
		}
		
		Ar << iChannelDesc0 << iChannelDesc1 << iChannelDesc2 << iChannelDesc3;
	}

	TSharedPtr<SWidget> ShaderToyPassNode::ExtraNodeWidget()
	{
		return SNew(SBox).Padding(4.0f)
			[
				SNew(SViewport).ViewportInterface(Preview).ViewportSize(FVector2D{ 80, 80 })
			];
	}

    TArray<TSharedRef<PropertyData>> ShaderToyPassNode::PropertyDatasFromUniform(UniformBuffer* InUb, bool Enabled)
    {
        if(!InUb) return {};
        TArray<TSharedRef<PropertyData>> Datas;
        const UniformBufferMetaData& MetaData = InUb->GetMetaData();
        for(const auto& [MemberName, MemberInfo]: MetaData.Members)
        {
            TSharedPtr<PropertyItemBase> Property;
			auto Writable = TAttribute<bool>::CreateLambda([this] { return !IsDebugging; });
            if(MemberInfo.HlslTypeName == "float")
            {
                auto FloatProperty = MakeShared<PropertyUniformItem<float>>(this, MemberName, InUb->GetMember<float>(MemberName), Writable);
                Property = FloatProperty;
            }
            else if(MemberInfo.HlslTypeName == "float2")
            {
                auto Float2Porperty = MakeShared<PropertyUniformItem<Vector2f>>(this, MemberName, InUb->GetMember<Vector2f>(MemberName), Writable);
                Property = Float2Porperty;
            }
			else if(MemberInfo.HlslTypeName == "float3")
			{
				auto Float3Porperty = MakeShared<PropertyUniformItem<Vector3f>>(this, MemberName, InUb->GetMember<Vector3f>(MemberName), Writable);
				Property = Float3Porperty;
			}
			else if (MemberInfo.HlslTypeName == "float4")
			{
				auto Float4Porperty = MakeShared<PropertyUniformItem<Vector4f>>(this, MemberName, InUb->GetMember<Vector4f>(MemberName), Writable);
				Property = Float4Porperty;
			}
            else
            {
                check(false);
            }
            Property->SetEnabled(Enabled);
            Datas.Add(Property.ToSharedRef());
        }
        return Datas;
    }

    TArray<TSharedRef<PropertyData>> ShaderToyPassNode::PropertyDatasFromBinding()
    {
        auto BuiltInCategory = MakeShared<PropertyCategory>(this, LOCALIZATION("Builtin"));
        {
            const GpuBindGroupLayoutDesc& BuiltInLayoutDesc = StShader::GetBuiltInBindLayoutBuilder().GetDesc();
            for(const auto& [BindingName, Slot] : BuiltInLayoutDesc.CodegenBindingNameToSlot)
            {
                if(BuiltInLayoutDesc.GetBindingType(Slot) == BindingType::UniformBuffer)
                {
                    auto UniformCategory = MakeShared<PropertyCategory>(this, BindingName);
                    auto PropertyDataUniforms = PropertyDatasFromUniform(StShader::GetBuiltInUb(), false);
                    for(auto Data : PropertyDataUniforms)
                    {
                        UniformCategory->AddChild(Data);
                    }
                    BuiltInCategory->AddChild(MoveTemp(UniformCategory));
                }
            }
            auto SlotCategory = MakeShared<PropertyCategory>(this, "Slot");
            {
				auto PropertyDatas0 = GeneratePropertyDatas(this, GetMetaType<ShaderToyPassNode>()->GetMetaMemberData("iChannel0"), this, true);
				auto PropertyDatas1 = GeneratePropertyDatas(this, GetMetaType<ShaderToyPassNode>()->GetMetaMemberData("iChannel1"), this, true);
				auto PropertyDatas2 = GeneratePropertyDatas(this, GetMetaType<ShaderToyPassNode>()->GetMetaMemberData("iChannel2"), this, true);
				auto PropertyDatas3 = GeneratePropertyDatas(this, GetMetaType<ShaderToyPassNode>()->GetMetaMemberData("iChannel3"), this, true);
                SlotCategory->AddChilds(MoveTemp(PropertyDatas0));
				SlotCategory->AddChilds(MoveTemp(PropertyDatas1));
				SlotCategory->AddChilds(MoveTemp(PropertyDatas2));
				SlotCategory->AddChilds(MoveTemp(PropertyDatas3));
            }
            BuiltInCategory->AddChild(MoveTemp(SlotCategory));
        }
        
        auto CustomCategory = MakeShared<PropertyCategory>(this, LOCALIZATION("Custom"));
        {
            const GpuBindGroupLayoutDesc& CustomLayoutDesc = ShaderAssetObj->CustomBindGroupLayoutBuilder.GetDesc();
            for(const auto& [BindingName, Slot] : CustomLayoutDesc.CodegenBindingNameToSlot)
            {
                if(CustomLayoutDesc.GetBindingType(Slot) == BindingType::UniformBuffer)
                {
                    auto UniformCategory = MakeShared<PropertyCategory>(this, BindingName);
                    auto PropertyDataUniforms = PropertyDatasFromUniform(CustomUniformBuffer.Get(), true);
                    for(auto Data : PropertyDataUniforms)
                    {
                        UniformCategory->AddChild(Data);
                    }
                    CustomCategory->AddChild(MoveTemp(UniformCategory));
                }
            }
        }
        return {BuiltInCategory, CustomCategory};
    }

	bool ShaderToyPassNode::CanChangeProperty(PropertyData* InProperty)
	{
		if(InProperty->GetDisplayName().EqualTo(LOCALIZATION("Shader")))
		{
			if(ShaderAssetObj)
			{
				ShaderAssetObj->OnDestroy.RemoveAll(this);
				ShaderAssetObj->OnRefreshBuilder.RemoveAll(this);
			}
		}
		return true;
	}

    void ShaderToyPassNode::PostPropertyChanged(PropertyData* InProperty)
    {
		GraphNode::PostPropertyChanged(InProperty);
        
        //Shader asset changed.
        if(InProperty->IsOfType<PropertyAssetItem>() && InProperty->GetDisplayName().EqualTo(LOCALIZATION("Shader")))
        {
			ShaderAssetObj->OnDestroy.AddRaw(this, &ShaderToyPassNode::ClearBindingProperty);
			ShaderAssetObj->OnRefreshBuilder.AddRaw(this, &ShaderToyPassNode::RefreshProperty, true);
            RefreshProperty(false);
        }
		else if(IsProperyUniformItem(InProperty))
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			ShEditor->ForceRender();
		}
    }

    void ShaderToyPassNode::ClearBindingProperty()
    {
        CustomBindLayout.SafeRelease();
        CustomUniformBuffer.Reset();
        CustomBindGroup.SafeRelease();
        
        PropertyDatas.Empty();
        ShObject::GetPropertyDatas();
        GetOuterMost()->MarkDirty();
        
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->RefreshProperty();
    }

    void ShaderToyPassNode::RefreshProperty(bool bCopyUniformBuffer)
    {
        CustomBindLayout = ShaderAssetObj->CustomBindGroupLayoutBuilder.Build();
        CustomBindGroupBuilder = GpuBindGroupBuilder{ CustomBindLayout };
        
        auto NewCustomUniformBuffer = ShaderAssetObj->CustomUniformBufferBuilder.Build();
        if(NewCustomUniformBuffer.IsValid())
        {
            if(CustomUniformBuffer.IsValid() && bCopyUniformBuffer)
            {
                NewCustomUniformBuffer->CopySameMember(*CustomUniformBuffer);
            }
            CustomUniformBuffer = MoveTemp(NewCustomUniformBuffer);
			(*CustomBindGroupBuilder).SetUniformBuffer("CustomUniform", CustomUniformBuffer->GetGpuResource());
        }
        CustomBindGroup = (*CustomBindGroupBuilder).Build();
        
        PropertyDatas.Empty();
        GetPropertyDatas();
        GetOuterMost()->MarkDirty();
        
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->RefreshProperty();
    }

    TArray<TSharedRef<PropertyData>>* ShaderToyPassNode::GetPropertyDatas()
    {
        if(PropertyDatas.IsEmpty())
        {
            //Reflection data
            ShObject::GetPropertyDatas();
            
            //Custom binding
            if(ShaderAssetObj)
            {
                PropertyDatas.Append(PropertyDatasFromBinding());
            }
        }
        return &PropertyDatas;
    }

    ExecRet ShaderToyPassNode::Exec(GraphExecContext& Context)
	{
		if (!ShaderAssetObj.IsValid())
		{
            SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" does not specify the corresponding stShader."), *ObjectName.ToString());
            return {true, true};
		}
        
        if(ShaderAssetObj->GetPixelShader()->IsCompiled())
        {
            ShaderToyExecContext& ShaderToyContext = static_cast<ShaderToyExecContext&>(Context);

            auto PassOutput = static_cast<GpuTexturePin*>(GetPin("RT"));
            if(PassOutput->GetValue()->GetWidth() != (uint32)ShaderToyContext.iResolution.x ||
               PassOutput->GetValue()->GetHeight() != (uint32)ShaderToyContext.iResolution.y ||
				PassOutput->GetValue()->GetFormat() != (GpuTextureFormat)Format)
            {
                GpuTextureDesc Desc{ (uint32)ShaderToyContext.iResolution.x, (uint32)ShaderToyContext.iResolution.y, (GpuTextureFormat)Format, GpuTextureUsage::ShaderResource | GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared };
                TRefCountPtr<GpuTexture> PassOuputTex = GGpuRhi->CreateTexture(MoveTemp(Desc), GpuResourceState::RenderTargetWrite);
                Preview->SetViewPortRenderTexture(PassOuputTex);
                PassOutput->SetValue(PassOuputTex);
            }

            GpuRenderPassDesc PassDesc;
            PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ PassOutput->GetValue(), RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store });

			BindingContext Bindings;
			Bindings.SetGlobalBindGroup(GetBuiltInBindGroup());
			Bindings.SetGlobalBindGroupLayout(StShader::GetBuiltInBindLayout());
			Bindings.SetPassBindGroup(CustomBindGroup);
			Bindings.SetPassBindGroupLayout(CustomBindLayout);

			PipelineDesc = GpuRenderPipelineStateDesc{
				.CheckLayout = true,
                .Vs = ShaderAssetObj->GetVertexShader(),
                .Ps = ShaderAssetObj->GetPixelShader(),
				.Targets = {
                    { .TargetFormat = PassOutput->GetValue()->GetFormat() }
                }
            };
			Bindings.ApplyBindGroupLayout(PipelineDesc);

			TRefCountPtr<GpuRenderPipelineState> PipelineState;

			try
			{
				PipelineState = GGpuRhi->CreateRenderPipelineState(PipelineDesc);
			}
			catch (const std::runtime_error& e)
			{
				SH_LOG(LogShader, Error, TEXT("Execution failed: can not declare resource bindings in stshader.\n%s"), ANSI_TO_TCHAR(e.what()));
				return { true, true };
			}

			ShaderToyContext.RG->AddRenderPass(ObjectName.ToString(), MoveTemp(PassDesc), MoveTemp(Bindings),
				[PipelineState](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
					Bindings.ApplyBindGroup(PassRecorder);
					PassRecorder->SetRenderPipelineState(PipelineState);;
					PassRecorder->DrawPrimitive(0, 3, 0, 1);
				}
			);
			ShaderToyContext.RG->Execute();

			ShaderAssertInfo AssertInfo;
			TArray<FString> ShaderPrintLogs = TSingleton<PrintBuffer>::Get().GetPrintStrings(AssertInfo);
			TSingleton<PrintBuffer>::Get().Clear();
			if(AssertInfo.AssertString.IsEmpty())
			{
				for (const FString& PrintLog : ShaderPrintLogs)
				{
					SH_LOG(LogShader, Display, TEXT("%s:%s"), *ObjectName.ToString(), *PrintLog);
				}
			}
			else
			{
				int AddedLineNum = ShaderAssetObj->GetExtraLineNum();
				SH_LOG(LogShader, Error, TEXT("%s:%d:%s"), *ObjectName.ToString(), AssertInfo.LineNumber - AddedLineNum, *AssertInfo.AssertString);
				AssertError = true;
				return {true, true};
			}
        }
        
        if(!ShaderAssetObj->bCompilationSucceed)
        {
            return {true, false};
        }
        
        return {};
	}

}
