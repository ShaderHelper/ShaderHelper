#include "CommonHeader.h"
#include "ShRenderer.h"

using namespace FW;

namespace SH
{
	ShRenderer::ShRenderer()
	{

	}

	void ShRenderer::Render()
	{
		for (auto RenderComp : RenderComps)
		{
			RenderComp->RenderBegin();
			RenderComp->RenderInternal();
			RenderComp->RenderEnd();
		}
	}

}
