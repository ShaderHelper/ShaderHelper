#include "CommonHeader.h"
#include "GpuApi/GpuApiInterface.h"
#include "GpuApi/GpuApiValidation.h"
#include "MetalCommon.h"
#include "MetalDevice.h"
#include "MetalCommandList.h"
#include "MetalMap.h"
#include "MetalTexture.h"
#include "MetalShader.h"
#include "MetalBuffer.h"
#include "MetalPipeline.h"
#include "MetalArgumentBuffer.h"

namespace FRAMEWORK
{
namespace GpuApi
{
		
	void InitApiEnv()
	{
        InitMetalCore();
        GetCommandListContext()->SetCommandBuffer(GCommandQueue.CommandBuffer());
	}

	void FlushGpu()
	{
        id<MTLCommandBuffer> CommandBuffer = GetCommandListContext()->GetCommandBuffer();
        [CommandBuffer commit];
        [CommandBuffer waitUntilCompleted];
        GetCommandListContext()->SetCommandBuffer(GCommandQueue.CommandBuffer());
	}

	void BeginFrame()
	{
        GCaptureScope.BeginScope();
	}

	void EndFrame()
	{
        FlushGpu();
        GCaptureScope.EndScope();
	}

	TRefCountPtr<GpuTexture> CreateTexture(const GpuTextureDesc& InTexDesc)
	{
        return AUX::StaticCastRefCountPtr<GpuTexture>(CreateMetalTexture2D(InTexDesc));
	}

    void* MapGpuTexture(GpuTexture* InGpuTexture, GpuResourceMapMode InMapMode, uint32& OutRowPitch)
	{
        MetalTexture* Texture = static_cast<MetalTexture*>(InGpuTexture);
        if (InMapMode == GpuResourceMapMode::Read_Only)
        {
            const uint32 BytesPerTexel = GetTextureFormatByteSize(InGpuTexture->GetFormat());
            const uint64 UnpaddedSize = InGpuTexture->GetWidth() * InGpuTexture->GetHeight() * BytesPerTexel;
            if (!Texture->ReadBackBuffer.IsValid())
            {
                Texture->ReadBackBuffer = CreateMetalBuffer(UnpaddedSize, GpuBufferUsage::Staging);
            }
            
            //Metal does not consider the alignment when copying back?
            OutRowPitch = InGpuTexture->GetWidth() * BytesPerTexel;
            
            mtlpp::BlitCommandEncoder BlitCommandEncoder = GetCommandListContext()->GetBlitCommandEncoder();
            MTLOrigin Origin = MTLOriginMake(0, 0, 0);
            MTLSize Size = MTLSizeMake(InGpuTexture->GetWidth(), InGpuTexture->GetHeight(), 1);
            [BlitCommandEncoder.GetPtr() copyFromTexture:Texture->GetResource() sourceSlice:0 sourceLevel:0 sourceOrigin:Origin sourceSize:Size toBuffer:Texture->ReadBackBuffer->GetResource() destinationOffset:0 destinationBytesPerRow:OutRowPitch destinationBytesPerImage:UnpaddedSize];
            BlitCommandEncoder.EndEncoding();
            
            FlushGpu();
            return Texture->ReadBackBuffer->GetContents();
        }
        else
        {
            //TODO
            check(false);
            return nullptr;
        }
	}

	void UnMapGpuTexture(GpuTexture* InGpuTexture)
	{

	}

    void* MapGpuBuffer(GpuBuffer* InGpuBuffer, GpuResourceMapMode InMapMode)
    {
        GpuBufferUsage Usage = InGpuBuffer->GetUsage();
        MetalBuffer* Buffer = static_cast<MetalBuffer*>(InGpuBuffer);
        void* Data = nullptr;
        if(InMapMode == GpuResourceMapMode::Write_Only)
        {
            if(EnumHasAnyFlags(Usage, GpuBufferUsage::Static))
            {
                
            }
            else
            {
                Data = Buffer->GetContents();
            }
        }
        else
        {
            
        }
        return Data;
    }

    void UnMapGpuBuffer(GpuBuffer* InGpuBuffer)
    {
        
    }

	TRefCountPtr<GpuShader> CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName, FString EntryPoint)
	{
        return AUX::StaticCastRefCountPtr<GpuShader>(CreateMetalShader(InType, MoveTemp(InSourceText), MoveTemp(InShaderName), MoveTemp(EntryPoint)));
	}

    TRefCountPtr<GpuShader> CreateShaderFromFile(FString FileName, ShaderType InType, FString EntryPoint, FString ExtraDeclaration)
	{
        return AUX::StaticCastRefCountPtr<GpuShader>(CreateMetalShader(MoveTemp(FileName), InType, MoveTemp(ExtraDeclaration), MoveTemp(EntryPoint)));
	}

	TRefCountPtr<GpuBindGroup> CreateBindGroup(const GpuBindGroupDesc& InBindGroupDesc)
	{
        check(ValidateCreateBindGroup(InBindGroupDesc));
        return AUX::StaticCastRefCountPtr<GpuBindGroup>(CreateMetalBindGroup(InBindGroupDesc));
	}

	TRefCountPtr<GpuBindGroupLayout> CreateBindGroupLayout(const GpuBindGroupLayoutDesc& InBindGroupLayoutDesc)
	{
        check(ValidateCreateBindGroupLayout(InBindGroupLayoutDesc));
        return AUX::StaticCastRefCountPtr<GpuBindGroupLayout>(CreateMetalBindGroupLayout(InBindGroupLayoutDesc));
	}

    bool CrossCompileShader(GpuShader* InShader, FString& OutErrorInfo)
    {
        return CompileShaderFromHlsl(static_cast<MetalShader*>(InShader), OutErrorInfo);
    }

	TRefCountPtr<GpuPipelineState> CreateRenderPipelineState(const GpuPipelineStateDesc& InPipelineStateDesc)
	{
		check(ValidateCreateRenderPipelineState(InPipelineStateDesc));
        return AUX::StaticCastRefCountPtr<GpuPipelineState>(CreateMetalPipelineState(InPipelineStateDesc));
	}

	TRefCountPtr<GpuBuffer> CreateBuffer(uint32 ByteSize, GpuBufferUsage Usage)
	{
        return AUX::StaticCastRefCountPtr<GpuBuffer>(CreateMetalBuffer(ByteSize, Usage));
	}

	TRefCountPtr<GpuSampler> CreateSampler(const GpuSamplerDesc& InSamplerDesc)
	{
        return AUX::StaticCastRefCountPtr<GpuSampler>(CreateMetalSampler(InSamplerDesc));
	}

	void SetRenderPipelineState(GpuPipelineState* InPipelineState)
	{
        GetCommandListContext()->SetPipeline(static_cast<MetalPipelineState*>(InPipelineState));
	}

	void SetVertexBuffer(GpuBuffer* InVertexBuffer)
	{
        GetCommandListContext()->SetVertexBuffer(static_cast<MetalBuffer*>(InVertexBuffer));
	}

	void SetViewPort(const GpuViewPortDesc& InViewPortDesc)
	{
        mtlpp::Viewport Viewport{
            (double)InViewPortDesc.TopLeftX, (double)InViewPortDesc.TopLeftY,
            (double)InViewPortDesc.Width, (double)InViewPortDesc.Height,
            (double)InViewPortDesc.ZMin, (double)InViewPortDesc.ZMax
        };
        
        mtlpp::ScissorRect ScissorRect{0, 0, (uint32)InViewPortDesc.Width, (uint32)InViewPortDesc.Height};
        GetCommandListContext()->SetViewPort(MoveTemp(Viewport), MoveTemp(ScissorRect));
	}

	void SetBindGroups(GpuBindGroup* BindGroup0, GpuBindGroup* BindGroup1, GpuBindGroup* BindGroup2, GpuBindGroup* BindGroup3)
	{
        check(ValidateSetBindGroups(BindGroup0, BindGroup1, BindGroup2, BindGroup3));
        GetCommandListContext()->SetBindGroups(
            static_cast<MetalBindGroup*>(BindGroup0),
            static_cast<MetalBindGroup*>(BindGroup1),
            static_cast<MetalBindGroup*>(BindGroup2),
            static_cast<MetalBindGroup*>(BindGroup3)
        );
	}

	void DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount)
	{
        GetCommandListContext()->PrepareDrawingEnv();
        GetCommandListContext()->Draw(StartVertexLocation, VertexCount, StartInstanceLocation, InstanceCount);
	}

	void Submit()
	{
        id<MTLCommandBuffer> CommandBuffer = GetCommandListContext()->GetCommandBuffer();
        [CommandBuffer commit];
        GetCommandListContext()->SetCommandBuffer(GCommandQueue.CommandBuffer());
	}

	void BeginGpuCapture(const FString& SavedFileName)
	{

	}

	void EndGpuCapture()
	{

	}

	void BeginCaptureEvent(const FString& EventName)
	{

	}

	void EndCpatureEvent()
	{

	}

    void BeginRenderPass(const GpuRenderPassDesc& PassDesc, const FString& PassName)
    {
        mtlpp::RenderPassDescriptor RenderPassDesc = MapRenderPassDesc(PassDesc);
        GetCommandListContext()->SetRenderPassDesc(MoveTemp(RenderPassDesc), PassName);
        GetCommandListContext()->ClearBinding();
    }

    void EndRenderPass()
    {
        id<MTLRenderCommandEncoder> RenderCommandEncoder = GetCommandListContext()->GetRenderCommandEncoder();
        [RenderCommandEncoder endEncoding];
    }

	void* GetSharedHandle(GpuTexture* InGpuTexture)
	{
		return static_cast<MetalTexture*>(InGpuTexture)->GetSharedHandle();
	}

}
}
