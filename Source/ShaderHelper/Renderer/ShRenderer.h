#pragma once
#include "Renderer/Renderer.h"
#include "GpuApi/GpuResource.h"
#include "App/PreviewViewPort.h"
namespace SH
{
	class ShRenderer : public Renderer
	{
	public:
		ShRenderer();
		
	public:
		virtual void* GetSharedHanldeFromFinalRT() const override;
		virtual void Render() override;

	private:
		void RenderBegin();
		void RenderEnd();

	public:
		PreviewViewPort* ViewPort;
			
	private:
		TRefCountPtr<GpuTexture> FinalRT;
		TRefCountPtr<GpuShader> VertexShader;
		TRefCountPtr<GpuShader> PixelShader;
		TRefCountPtr<RenderPipelineState> PipelineState;
		bool bCanGpuCapture;
	};
}


