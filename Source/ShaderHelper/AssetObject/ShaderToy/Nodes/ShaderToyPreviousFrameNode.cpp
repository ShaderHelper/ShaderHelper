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
	REGISTER_NODE_TO_GRAPH(ShaderToyPreviousFrameNode, "ShaderToy")

	MetaType* ShaderToyPreviousFrameNodeOp::SupportType()
	{
		return GetMetaType<ShaderToyPreviousFrameNode>();
	}

	ShaderToyPreviousFrameNode::ShaderToyPreviousFrameNode()
	{
		ObjectName = LOCALIZATION("PreviousFrame");
	}

	ShaderToyPreviousFrameNode::ShaderToyPreviousFrameNode(ShaderToyPassNode* InPassNode)
		: PassNode(InPassNode)
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
				AllPassNodes.Add(MakeShared<ObserverObjectPtr<ShaderToyPassNode>>(static_cast<ShaderToyPassNode*>(Node.Get())));
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
				SNew(SComboBox<TSharedPtr<ObserverObjectPtr<ShaderToyPassNode>>>)
				.OptionsSource(&AllPassNodes)
				.OnComboBoxOpening_Lambda([this] {
					UpdatePassNodes();
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<ObserverObjectPtr<ShaderToyPassNode>> InItem, ESelectInfo::Type) {
					if (InItem && PassNode != *InItem)
					{
						PassNode = InItem->Get();
						GetOuterMost()->MarkDirty();
					}
				})
				.OnGenerateWidget_Lambda([this](TSharedPtr<ObserverObjectPtr<ShaderToyPassNode>> InItem) {
					FText NodeName;
					if (InItem && InItem->IsValid())
					{
						NodeName = InItem->Get()->ObjectName;
					}
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

	TArray<TSharedRef<FW::PropertyData>> ShaderToyPreviousFrameNode::GeneratePropertyDatas()
	{
		TArray<TSharedRef<FW::PropertyData>> Result = ShObject::GeneratePropertyDatas();
		auto PassNodePropertyItem = MakeShared<PropertyItemBase>(this, "Pass");
		PassNodePropertyItem->SetEmbedWidget(ExtraNodeWidget());
		Result.Add(PassNodePropertyItem);
		return Result;
	}

	ExecRet ShaderToyPreviousFrameNode::Exec(GraphExecContext& Context)
	{
		ShaderToyPassNode* PassNodeInstance = GetPassNode();
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
			Input.InputView = GpuResourceHelper::GetGlobalBlackTex()->GetDefaultView();
		}
		else
		{
			Input.InputView = PassNodeOutputPin->GetValue()->GetDefaultView();
		}
		Input.InputTexSampler = GpuResourceHelper::GetSampler({});
		Input.OutputView = PreFramePassTex->GetDefaultView();

		AddBlitPass(*ShaderToyContext.RG, MoveTemp(Input));

		return {};
	}

	ShaderToyPassNode* ShaderToyPreviousFrameNode::GetPassNode()
	{
		return PassNode.Get();
	}
}
