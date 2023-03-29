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
	FRAMEWORK_API void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode);
	FRAMEWORK_API void UnMapGpuTexture(GpuTexture* InGpuTexture);

	FRAMEWORK_API TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName);
	FRAMEWORK_API bool CompilerShader(GpuShader* InShader);

	FRAMEWORK_API TRefCountPtr<RenderPipelineState> CreateRenderPipelineState(const PipelineStateDesc& InPipelineStateDesc);
	FRAMEWORK_API void BindRenderPipelineState(RenderPipelineState* InPipelineState);
	
	FRAMEWORK_API void BindVertexBuffer();
	FRAMEWORK_API void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount);
}
}
