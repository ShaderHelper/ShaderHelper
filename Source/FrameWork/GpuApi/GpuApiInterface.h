#pragma once
#include "GpuResource.h"
namespace FRAMEWORK
{
namespace GpuApi
{
	FRAMEWORK_API void InitApiEnv(); 

	//Wait for all gpu work to finish
	FRAMEWORK_API void FlushGpu();

	FRAMEWORK_API void StartRenderFrame();
	FRAMEWORK_API void EndRenderFrame();

	FRAMEWORK_API TRefCountPtr<GpuTexture> CreateGpuTexture(const GpuTextureDesc& InTexDesc);
	FRAMEWORK_API void* MapGpuTexture(TRefCountPtr<GpuTexture> InGpuTexture, GpuResourceMapMode InMapMode);
	FRAMEWORK_API void UnMapGpuTexture(TRefCountPtr<GpuTexture> InGpuTexture);

	FRAMEWORK_API TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName);
	FRAMEWORK_API bool CompilerShader(TRefCountPtr<GpuShader> InShader);

	FRAMEWORK_API TRefCountPtr<RenderPipelineState> CreateRenderPipelineState(const PipelineStateDesc& InPipelineStateDesc);
	FRAMEWORK_API void SetRenderPipelineState(TRefCountPtr<RenderPipelineState> InPipelineState);
}
}
