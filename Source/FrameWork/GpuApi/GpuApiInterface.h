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
	
	FRAMEWORK_API void BindVertexBuffer(GpuBuffer* InVertexBuffer);
	FRAMEWORK_API void SetViewPort(const GpuViewPortDesc& InViewPortDesc);
	FRAMEWORK_API void SetRenderTarget(GpuTexture* InGpuTexture);
	FRAMEWORK_API void SetClearColorValue(Vector4f ClearColor);
	FRAMEWORK_API void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount, PrimitiveType InType = PrimitiveType::Triangle);
	
	FRAMEWORK_API void Submit();

	FRAMEWORK_API void BeginGpuCapture(const FString& SavedFileName);
	FRAMEWORK_API void EndGpuCapture();
	FRAMEWORK_API void BeginCaptureEvent(const FString& EventName);
	FRAMEWORK_API void EndCpatureEvent();

	//The renderer for drawing ui in UE uses dx11 as backend api, so need a shared resource.
	FRAMEWORK_API void* GetSharedHandle(GpuTexture* InGpuTexture);
}
}
