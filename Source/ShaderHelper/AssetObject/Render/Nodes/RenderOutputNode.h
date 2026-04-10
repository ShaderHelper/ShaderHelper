#pragma once
#include "Editor/AssetEditor/ShAssetEditor.h"
#include "AssetObject/Graph.h"
#include "AssetObject/Render/Render.h"

namespace SH
{
	class RenderOutputNodeOp : public ShPropertyOp
	{
		REFLECTION_TYPE(RenderOutputNodeOp)
	public:
		RenderOutputNodeOp() = default;

		FW::MetaType* SupportType() override;
		void OnCancelSelect(FW::ShObject* InObject) override;
		void OnSelect(FW::ShObject* InObject) override;
	};

	class RenderOutputNode : public FW::GraphNode
	{
		REFLECTION_TYPE(RenderOutputNode)
	public:
		RenderOutputNode();
		~RenderOutputNode();
		void Init() override;

	public:
		void InitPins();
		void Serialize(FArchive& Ar) override;
		FW::ExecRet Exec(FW::GraphExecContext& Context) override;
		void PostPropertyChanged(FW::PropertyData* InProperty) override;

		RenderFormat Format;
		int32 Layer;
		float AreaFraction;
	};
}
