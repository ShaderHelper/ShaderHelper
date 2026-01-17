#include "CommonHeader.h"
#include "ShaderToyPreviousFrameNode.h"
#include "AssetObject/Pins/Pins.h"
#include "Renderer/ShaderToyRenderComp.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/Property/PropertyData/PropertyItem.h"

using namespace FW;

namespace SH
{
	REFLECTION_REGISTER(AddClass<ShaderToyPreviousFrameNode>("PreviousFrame Node")
		.BaseClass<GraphNode>()
	)
	REFLECTION_REGISTER(AddClass<ShaderToyPreviousFrameNodeOp>()
		.BaseClass<ShObjectOp>()
	)
	REGISTER_NODE_TO_GRAPH(ShaderToyPreviousFrameNode, "ShaderToy Graph")

	MetaType* ShaderToyPreviousFrameNodeOp::SupportType()
	{
		return GetMetaType<ShaderToyPreviousFrameNode>();
	}

	void ShaderToyPreviousFrameNodeOp::OnCancelSelect(FW::ShObject* InObject)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->RefreshProperty(true);
	}

	void ShaderToyPreviousFrameNodeOp::OnSelect(ShObject* InObject)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ShowProperty(InObject);
	}

	ShaderToyPreviousFrameNode::ShaderToyPreviousFrameNode()
	{
		ObjectName = LOCALIZATION("PreviousFrame");
	}

	ShaderToyPreviousFrameNode::~ShaderToyPreviousFrameNode()
	{

	}

	void ShaderToyPreviousFrameNode::Init()
	{
		InitPins();
	}

	void ShaderToyPreviousFrameNode::UpdatePassNodes()
	{
		AllPassNodes.Reset();
		const TArray<ObjectPtr<GraphNode>>& AllNodes = static_cast<ShaderToy*>(GetOuterMost())->GetNodes();
		for (const auto& Node : AllNodes)
		{
			if (Node->DynamicMetaType() == GetMetaType<ShaderToyPassNode>())
			{
				AllPassNodes.Add(MakeShared<FGuid>(Node->GetGuid()));
			}
		}
	}

	void ShaderToyPreviousFrameNode::InitPins()
	{
		auto OutputPin = NewShObject<GpuTexturePin>(this);
		OutputPin->ObjectName = FText::FromString("RT");
		OutputPin->Direction = PinDirection::Output;

		Pins = { MoveTemp(OutputPin) };
	}

	void ShaderToyPreviousFrameNode::Serialize(FArchive& Ar)
	{
		GraphNode::Serialize(Ar);
		Ar << PassNode;
	}

	void ShaderToyPreviousFrameNode::PostLoad()
	{
		GraphNode::PostLoad();
	}

	TSharedPtr<SWidget> ShaderToyPreviousFrameNode::ExtraNodeWidget()
	{
		return SNew(SBox).Padding(4)
			[
				SNew(SComboBox<TSharedPtr<FGuid>>)
				.OptionsSource(&AllPassNodes)
				.OnComboBoxOpening_Lambda([this] {
					UpdatePassNodes();
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FGuid> InItem, ESelectInfo::Type) {
					if (InItem && PassNode != *InItem)
					{
						PassNode = *InItem;
						GetOuterMost()->MarkDirty();
					}
				})
				.OnGenerateWidget_Lambda([this](TSharedPtr<FGuid> InItem) {
					FText NodeName = static_cast<ShaderToy*>(GetOuterMost())->GetNode(*InItem)->ObjectName;
					return SNew(STextBlock).Text(NodeName);
				})
				[
					SNew(STextBlock).Text_Lambda([this] {
						ShaderToyPassNode* PassNodeInstance = GetPassNode();
						FText NodeName;
						if (PassNodeInstance)
						{
							NodeName = PassNodeInstance->ObjectName;
						}
						return NodeName;
					})
				]
			];
	}

	TArray<TSharedRef<FW::PropertyData>>* ShaderToyPreviousFrameNode::GetPropertyDatas()
	{
		ShObject::GetPropertyDatas();
		auto PassNodePropertyItem = MakeShared<PropertyItemBase>(this, "Pass");
		PassNodePropertyItem->SetEmbedWidget(ExtraNodeWidget());
		PropertyDatas.Add(PassNodePropertyItem);
		return &PropertyDatas;
	}

	ExecRet ShaderToyPreviousFrameNode::Exec(GraphExecContext& Context)
	{
		GraphNode* PassNodeInstance = static_cast<ShaderToy*>(GetOuterMost())->GetNode(PassNode);
		if (!PassNodeInstance)
		{
			SH_LOG(LogGraph, Error, TEXT("Node:\"%s\" does not specify the corresponding ShaderPass Node."), *ObjectName.ToString());
			return { true, true };
		}
		ShaderToyExecContext& ShaderToyContext = static_cast<ShaderToyExecContext&>(Context);
		auto OutputPin = static_cast<GpuTexturePin*>(GetPin("RT"));
		auto PassNodeOutputPin = static_cast<GpuTexturePin*>(PassNodeInstance->GetPin("RT"));
		if (!PreFramePassTex ||
			OutputPin->GetValue()->GetWidth() != PassNodeOutputPin->GetValue()->GetWidth() ||
			OutputPin->GetValue()->GetHeight() != PassNodeOutputPin->GetValue()->GetHeight() ||
			OutputPin->GetValue()->GetFormat() != PassNodeOutputPin->GetValue()->GetFormat())
		{
			PreFramePassTex = GGpuRhi->CreateTexture({
				.Width = PassNodeOutputPin->GetValue()->GetWidth(),
				.Height = PassNodeOutputPin->GetValue()->GetHeight(),
				.Format = PassNodeOutputPin->GetValue()->GetFormat(),
				.Usage = GpuTextureUsage::ShaderResource | GpuTextureUsage::RenderTarget
			}, GpuResourceState::RenderTargetWrite);
			OutputPin->SetValue(PreFramePassTex);
		}

		BlitPassInput Input;
		if (ShaderToyContext.bResetPreviousFrame)
		{
			Input.InputTex = GpuResourceHelper::GetGlobalBlackTex();
		}
		else
		{
			Input.InputTex = PassNodeOutputPin->GetValue();
		}
		Input.InputTexSampler = GpuResourceHelper::GetSampler({});
		Input.OutputRenderTarget = PreFramePassTex;

		AddBlitPass(*ShaderToyContext.RG, MoveTemp(Input));

		return {};
	}

	ShaderToyPassNode* ShaderToyPreviousFrameNode::GetPassNode()
	{
		return static_cast<ShaderToyPassNode*>(static_cast<ShaderToy*>(GetOuterMost())->GetNode(PassNode).Get());
	}
}
