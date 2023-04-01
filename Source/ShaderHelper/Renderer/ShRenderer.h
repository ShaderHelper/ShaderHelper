#pragma once
#include "Renderer/Renderer.h"
#include "GpuApi/GpuResource.h"
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
	private:
		TRefCountPtr<GpuTexture> FinalRT;
		TRefCountPtr<GpuShader> Shader;
		bool bCanGpuCapture;
	};
}


