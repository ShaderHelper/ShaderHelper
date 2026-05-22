#pragma once
#include "Editor/PreviewViewPort.h"
#include "AssetObject/Render/Render.h"
#include "AssetObject/Render/SceneObject.h"
#include "Renderer/RenderComponent.h"
#include "Renderer/RenderGraph.h"
#include "RenderResource/RenderPass/BlitPass.h"

namespace SH
{
	struct RenderOutputDesc
	{
		FW::BlitPassInput Pass;
		int32 Layer;
	};

	struct RenderExecContext : FW::GraphExecContext
	{
		FW::RenderGraph* RG = nullptr;
		TRefCountPtr<FW::GpuTexture> FinalRT;
		FW::PreviewViewPort* ViewPort = nullptr;
		FW::Vector2f ViewportSize = FW::Vector2f(0, 0);
		FW::Vector2f MousePos = FW::Vector2f(0, 0);
		float Time = 0.0f;
		TArray<RenderOutputDesc> Outputs;
		const TArray<FW::ObjectPtr<SceneObject>>* SceneObjects = nullptr;
	};

	class RenderRenderComp : public FW::RenderComponent
	{
	public:
		RenderRenderComp(Render* InRenderGraph, FW::PreviewViewPort* InViewPort);
		~RenderRenderComp() override = default;

	public:
		void RenderBegin() override;
		void RenderInternal() override;
		void RenderEnd() override;

		RenderExecContext Context;

	private:
		Render* RenderGraphAsset;
	};
}
