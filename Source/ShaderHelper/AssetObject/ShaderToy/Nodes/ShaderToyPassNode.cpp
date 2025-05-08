#include "CommonHeader.h"
#include "ShaderToyPassNode.h"
#include "Renderer/ShaderToyRenderComp.h"
#include <Widgets/SViewport.h>
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/Property/PropertyData/PropertyUniformItem.h"

using namespace FW;

namespace SH
{
    REFLECTION_REGISTER(AddClass<ShaderToyPassNode>("ShaderPass Node")
		.BaseClass<GraphNode>()
        .Data<&ShaderToyPassNode::Shader, MetaInfo::Property>("Shader")
	)
    REFLECTION_REGISTER(AddClass<ShaderToyPassNodeOp>()
        .BaseClass<ShObjectOp>()
    )

    MetaType* ShaderToyPassNodeOp::SupportType()
    {
        return GetMetaType<ShaderToyPassNode>();
    }

    void ShaderToyPassNodeOp::OnSelect(ShObject* InObject)
    {
        auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
        ShEditor->ShowProperty(InObject);
    }

	ShaderToyPassNode::ShaderToyPassNode()
	{
		ObjectName = FText::FromString("ShaderPass");
        Preview = MakeShared<PreviewViewPort>();
	}

    void ShaderToyPassNode::InitPins()
    {
        auto Slot0 = NewShObject<ChannelPin>(this);
        Slot0->ObjectName = FText::FromString("iChannel0");
        Slot0->Direction = PinDirection::Input;
        auto Slot1 = NewShObject<ChannelPin>(this);
        Slot1->ObjectName = FText::FromString("iChannel1");
        Slot1->Direction = PinDirection::Input;
        auto Slot2 = NewShObject<ChannelPin>(this);
        Slot2->ObjectName = FText::FromString("iChannel2");
        Slot2->Direction = PinDirection::Input;
        auto Slot3 = NewShObject<ChannelPin>(this);
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
		
        Ar << Shader;
        
        Ar << CustomUbSize;
        if(CustomUbSize > 0)
        {
            if(Ar.IsLoading())
            {
                CustomUniformBufferData = (uint8*)FMemory::Malloc(CustomUbSize);
            }
            Ar.Serialize(CustomUniformBufferData, CustomUbSize);
        }

	}

    void ShaderToyPassNode::PostLoad()
    {
        GraphNode::PostLoad();
        
        if(Shader)
        {
            Shader->OnDestroy = FSimpleDelegate::CreateRaw(this, &ShaderToyPassNode::ClearBindingProperty);
            Shader->OnRefreshBuilder = FSimpleDelegate::CreateRaw(this, &ShaderToyPassNode::RefreshProperty);
            
            CustomBindLayout = Shader->CustomBindGroupLayoutBuilder.Build();
            CustomUniformBuffer = Shader->CustomUniformBufferBuilder.Build();
            auto CustomBindGroupBuilder = GpuBindGrouprBuilder{ CustomBindLayout };
            if(CustomUniformBuffer.IsValid())
            {
                CustomUniformBuffer->SetData(CustomUniformBufferData);
                CustomUniformBufferData = (uint8*)CustomUniformBuffer->GetReadableData();
                CustomBindGroupBuilder.SetUniformBuffer("Uniform", CustomUniformBuffer->GetGpuResource());
            }
            CustomBindGroup = CustomBindGroupBuilder.Build();
        }
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
            if(MemberInfo.TypeName == "float")
            {
                auto FloatProperty = MakeShared<PropertyUniformItem<float>>(this, MemberName, InUb->GetMember<float>(MemberName));
                Property = FloatProperty;
            }
            else if(MemberInfo.TypeName == "float2")
            {
                auto Float2Porperty = MakeShared<PropertyUniformItem<Vector2f>>(this, MemberName, InUb->GetMember<Vector2f>(MemberName));
                Property = Float2Porperty;
            }
			else if (MemberInfo.TypeName == "float4")
			{
				auto Float4Porperty = MakeShared<PropertyUniformItem<Vector4f>>(this, MemberName, InUb->GetMember<Vector4f>(MemberName));
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
        auto BuiltInCategory = MakeShared<PropertyCategory>(this, "Built In");
        {
            const GpuBindGroupLayoutDesc& BuiltInLayoutDesc = StShader::GetBuiltInBindLayoutBuilder().GetLayoutDesc();
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
        
        auto CustomCategory = MakeShared<PropertyCategory>(this, "Custom");
        {
            const GpuBindGroupLayoutDesc& CustomLayoutDesc = Shader->CustomBindGroupLayoutBuilder.GetLayoutDesc();
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

    void ShaderToyPassNode::PostPropertyChanged(PropertyData* InProperty)
    {
        ShObject::PostPropertyChanged(InProperty);
        
        //Shader asset changed.
        if(InProperty->GetDisplayName() == "Shader")
        {
            Shader->OnDestroy = FSimpleDelegate::CreateRaw(this, &ShaderToyPassNode::ClearBindingProperty);
            Shader->OnRefreshBuilder = FSimpleDelegate::CreateRaw(this, &ShaderToyPassNode::RefreshProperty);
            RefreshProperty();
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

    void ShaderToyPassNode::RefreshProperty()
    {
        CustomBindLayout = Shader->CustomBindGroupLayoutBuilder.Build();
        auto CustomBindGroupBuilder = GpuBindGrouprBuilder{ CustomBindLayout };
        
        auto NewCustomUniformBuffer = Shader->CustomUniformBufferBuilder.Build();
        if(NewCustomUniformBuffer.IsValid())
        {
            if(CustomUniformBuffer.IsValid())
            {
                NewCustomUniformBuffer->CopySameMember(*CustomUniformBuffer);
            }
            CustomUniformBuffer = MoveTemp(NewCustomUniformBuffer);
            CustomUniformBufferData = (uint8*)CustomUniformBuffer->GetReadableData();
            CustomUbSize = CustomUniformBuffer->GetSize();
            CustomBindGroupBuilder.SetUniformBuffer("Uniform", CustomUniformBuffer->GetGpuResource());
        }
        CustomBindGroup = CustomBindGroupBuilder.Build();
        
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
            if(Shader)
            {
                PropertyDatas.Append(PropertyDatasFromBinding());
            }
        }
        return &PropertyDatas;
    }

    ExecRet ShaderToyPassNode::Exec(GraphExecContext& Context)
	{
		if (!Shader.IsValid())
		{
            SH_LOG(LogGraph, Error, TEXT("Node:%s does not specify the corresponding stShader."), *ObjectName.ToString());
            return {true, true};
		}
        
        if(Shader->PixelShader->IsCompiled())
        {
            ShaderToyExecContext& ShaderToyContext = static_cast<ShaderToyExecContext&>(Context);

            auto PassOutput = static_cast<GpuTexturePin*>(GetPin("RT"));
            if(PassOutput->GetValue()->GetWidth() != (uint32)ShaderToyContext.iResolution.x ||
               PassOutput->GetValue()->GetHeight() != (uint32)ShaderToyContext.iResolution.y)
            {
                GpuTextureDesc Desc{ (uint32)ShaderToyContext.iResolution.x, (uint32)ShaderToyContext.iResolution.y, GpuTextureFormat::B8G8R8A8_UNORM, GpuTextureUsage::ShaderResource | GpuTextureUsage::RenderTarget | GpuTextureUsage::Shared };
                TRefCountPtr<GpuTexture> PassOuputTex = GGpuRhi->CreateTexture(MoveTemp(Desc), GpuResourceState::RenderTargetWrite);
                Preview->SetViewPortRenderTexture(PassOuputTex);
                PassOutput->SetValue(PassOuputTex);
            }

            GpuRenderPassDesc PassDesc;
            PassDesc.ColorRenderTargets.Add(GpuRenderTargetInfo{ PassOutput->GetValue(), RenderTargetLoadAction::DontCare, RenderTargetStoreAction::Store });

			BindingContext Bindings;
			Bindings.SetGlobalBindGroup(StShader::GetBuiltInBindGroup());
			Bindings.SetGlobalBindGroupLayout(StShader::GetBuiltInBindLayout());
			Bindings.SetPassBindGroup(CustomBindGroup);
			Bindings.SetPassBindGroupLayout(CustomBindLayout);

            GpuRenderPipelineStateDesc PipelineDesc{
                .Vs = Shader->VertexShader,
                .Ps = Shader->PixelShader,
				.Targets = {
                    { .TargetFormat = PassOutput->GetValue()->GetFormat() }
                }
            };
			Bindings.ApplyBindGroupLayout(PipelineDesc);

            TRefCountPtr<GpuRenderPipelineState> PipelineState = GGpuRhi->CreateRenderPipelineState(PipelineDesc);

            ShaderToyContext.RG->AddRenderPass(ObjectName.ToString(), MoveTemp(PassDesc), MoveTemp(Bindings),
                [PipelineState](GpuRenderPassRecorder* PassRecorder, BindingContext& Bindings) {
					Bindings.ApplyBindGroup(PassRecorder);
                    PassRecorder->SetRenderPipelineState(PipelineState);;
                    PassRecorder->DrawPrimitive(0, 3, 0, 1);
                }
            );
        }
        
        if(!Shader->bCurPsCompilationSucceed)
        {
            return {true, false};
        }
        
        return {};
	}

}
