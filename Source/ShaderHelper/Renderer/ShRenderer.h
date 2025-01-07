#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/RenderComponent.h"

namespace SH
{
	class ShRenderer : public FW::Renderer
	{
	public:
		ShRenderer();
		void RegisterRenderComp(FW::RenderComponent* InRenderComp) { RenderComps.Add(InRenderComp); }
		void UnRegisterRenderComp(FW::RenderComponent* InRenderComp) { RenderComps.Remove(InRenderComp); }
		void ClearRenderComp() { RenderComps.Empty(); }

		void Render() override;

	private:
		TArray<FW::RenderComponent*> RenderComps;
	};
}


