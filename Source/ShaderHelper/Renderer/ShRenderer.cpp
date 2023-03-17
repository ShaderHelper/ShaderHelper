#include "CommonHeader.h"
#include "ShRenderer.h"
#include "GpuApi/GpuApiInterface.h"
namespace SH
{

	ShRenderer::ShRenderer()
	{
		GpuApi::InitApiEnv();
	}

	void* ShRenderer::GetSharedHanldeFromFinalRT() const
	{
		return nullptr;
	}

	void ShRenderer::RenderBegin()
	{

	}

	void ShRenderer::Render()
	{
		RenderBegin();
		
		RenderEnd();
	}

	void ShRenderer::RenderEnd()
	{
		GpuApi::FlushGpu();
	}

}