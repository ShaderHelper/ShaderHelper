#pragma once
#include "GpuResource.h"
namespace FRAMEWORK
{
namespace GpuApi
{
	FRAMEWORK_API void InitApiEnv(); 

	//Wait for all gpu work to finish
	FRAMEWORK_API void FlushGpu();

	FRAMEWORK_API void BeginFrame();
	FRAMEWORK_API void EndFrame();

	FRAMEWORK_API TRefCountPtr<GpuTexture> CreateGpuTexture(const GpuTextureDesc& InTexDesc);
	FRAMEWORK_API TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName, FString EntryPoint);
	FRAMEWORK_API TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc& InBindGroupDesc);
	FRAMEWORK_API TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc& InBindGroupLayoutDesc);
	FRAMEWORK_API TRefCountPtr<GpuPipelineState> CreateRenderPipelineState(const GpuPipelineStateDesc& InPipelineStateDesc);
	FRAMEWORK_API TRefCountPtr<GpuBuffer> CreateBuffer(uint32 ByteSize, GpuBufferUsage Usage);

	//Need OutRowPitch to correctly read or write data, because the mapped buffer actually contains the *padded* texture data.
	//RowPitch != Width x ElementByteSize
	//RowPitch = align(Width x ElementByteSize, RequiredAlignValue)
	FRAMEWORK_API void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch);
	FRAMEWORK_API void UnMapGpuTexture(GpuTexture* InGpuTexture);

	FRAMEWORK_API void* MapGpuBuffer(GpuBuffer* InGpuBuffer, GpuResourceMapMode InMapMode);
	FRAMEWORK_API void UnMapGpuBuffer(GpuBuffer* InGpuBuffer);
	
    //Accept each platform's own shader.
	FRAMEWORK_API bool CompileShader(GpuShader* InShader, FString& OutErrorInfo);
    //Accept hlsl.
    FRAMEWORK_API bool CrossCompileShader(GpuShader* InShader, FString& OutErrorInfo);

	FRAMEWORK_API void SetRenderPipelineState(GpuPipelineState* InPipelineState);
	
	FRAMEWORK_API void SetVertexBuffer(GpuBuffer* InVertexBuffer);
	FRAMEWORK_API void SetViewPort(const GpuViewPortDesc& InViewPortDesc);

	//Only support 4 BindGroups to adapt some mobile devices.
	FRAMEWORK_API void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3);

	FRAMEWORK_API void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount, PrimitiveType InType = PrimitiveType::Triangle);
	
	FRAMEWORK_API void Submit();

	FRAMEWORK_API void BeginGpuCapture(const FString& SavedFileName);
	FRAMEWORK_API void EndGpuCapture();
	FRAMEWORK_API void BeginCaptureEvent(const FString& EventName);
	FRAMEWORK_API void EndCpatureEvent();

	//The renderer for drawing ui in UE uses dx11/opengl as backend api, so need a shared resource.
	FRAMEWORK_API void* GetSharedHandle(GpuTexture* InGpuTexture);
    
    FRAMEWORK_API void BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName);
    FRAMEWORK_API void EndRenderPass();
}
}
