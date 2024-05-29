#include "CommonHeader.h"

#include "MetalGpuRhiBackend.h"

#include "MetalArgumentBuffer.h"
#include "MetalBuffer.h"
#include "MetalCommandList.h"
#include "MetalCommon.h"
#include "MetalDevice.h"
#include "MetalMap.h"
#include "MetalPipeline.h"
#include "MetalShader.h"
#include "MetalTexture.h"

namespace FRAMEWORK
{
MetalGpuRhiBackend::MetalGpuRhiBackend() { }

MetalGpuRhiBackend::~MetalGpuRhiBackend() { }

void MetalGpuRhiBackend::InitApiEnv()
{
	InitMetalCore();
	GetCommandListContext()->SetCommandBuffer(GCommandQueue.CommandBuffer());
}

void MetalGpuRhiBackend::FlushGpu()
{
	id<MTLCommandBuffer> CommandBuffer = GetCommandListContext()->GetCommandBuffer();
	[CommandBuffer commit];
	[CommandBuffer waitUntilCompleted];
	GetCommandListContext()->SetCommandBuffer(GCommandQueue.CommandBuffer());
}

void MetalGpuRhiBackend::BeginFrame()
{
	GCaptureScope.BeginScope();
}

void MetalGpuRhiBackend::EndFrame()
{
	FlushGpu();
	GCaptureScope.EndScope();
}

TRefCountPtr<GpuTexture> MetalGpuRhiBackend::CreateTexture(const GpuTextureDesc &InTexDesc)
{
	return AUX::StaticCastRefCountPtr<GpuTexture>(CreateMetalTexture2D(InTexDesc));
}

TRefCountPtr<GpuShader> MetalGpuRhiBackend::CreateShaderFromSource(ShaderType InType, FString InSourceText, FString InShaderName, FString EntryPoint)
{
	return AUX::StaticCastRefCountPtr<GpuShader>(CreateMetalShader(InType, MoveTemp(InSourceText), MoveTemp(InShaderName), MoveTemp(EntryPoint)));
}

TRefCountPtr<GpuShader> MetalGpuRhiBackend::CreateShaderFromFile(FString FileName, ShaderType InType, FString EntryPoint, FString ExtraDeclaration)
{
	return AUX::StaticCastRefCountPtr<GpuShader>(CreateMetalShader(MoveTemp(FileName), InType, MoveTemp(ExtraDeclaration), MoveTemp(EntryPoint)));
}

TRefCountPtr<GpuBindGroup> MetalGpuRhiBackend::CreateBindGroup(const GpuBindGroupDesc &InBindGroupDesc)
{
	return AUX::StaticCastRefCountPtr<GpuBindGroup>(CreateMetalBindGroup(InBindGroupDesc));
}

TRefCountPtr<GpuBindGroupLayout> MetalGpuRhiBackend::CreateBindGroupLayout(const GpuBindGroupLayoutDesc &InBindGroupLayoutDesc)
{
	return AUX::StaticCastRefCountPtr<GpuBindGroupLayout>(CreateMetalBindGroupLayout(InBindGroupLayoutDesc));
}

TRefCountPtr<GpuPipelineState> MetalGpuRhiBackend::CreateRenderPipelineState(const GpuRenderPipelineStateDesc &InPipelineStateDesc)
{
	return AUX::StaticCastRefCountPtr<GpuPipelineState>(CreateMetalPipelineState(InPipelineStateDesc));
}

TRefCountPtr<GpuBuffer> MetalGpuRhiBackend::CreateBuffer(uint32 ByteSize, GpuBufferUsage Usage)
{
	return AUX::StaticCastRefCountPtr<GpuBuffer>(CreateMetalBuffer(ByteSize, Usage));
}

TRefCountPtr<GpuSampler> MetalGpuRhiBackend::CreateSampler(const GpuSamplerDesc &InSamplerDesc)
{
	return AUX::StaticCastRefCountPtr<GpuSampler>(CreateMetalSampler(InSamplerDesc));
}

void MetalGpuRhiBackend::SetTextureName(const FString &TexName, GpuTexture *InTexture)
{
	// not implemented.
}

void MetalGpuRhiBackend::SetBufferName(const FString &BufferName, GpuBuffer *InBuffer)
{
	// not implemented.
}

void *MetalGpuRhiBackend::MapGpuTexture(GpuTexture *InGpuTexture, GpuResourceMapMode InMapMode, uint32 &OutRowPitch)
{
	MetalTexture *Texture = static_cast<MetalTexture *>(InGpuTexture);
	if (InMapMode == GpuResourceMapMode::Read_Only) {
		const uint32 BytesPerTexel = GetTextureFormatByteSize(InGpuTexture->GetFormat());
		const uint64 UnpaddedSize = InGpuTexture->GetWidth() * InGpuTexture->GetHeight() * BytesPerTexel;
		if (!Texture->ReadBackBuffer.IsValid()) {
			Texture->ReadBackBuffer = CreateMetalBuffer(UnpaddedSize, GpuBufferUsage::Staging);
		}

		// Metal does not consider the alignment when copying back?
		OutRowPitch = InGpuTexture->GetWidth() * BytesPerTexel;

		mtlpp::BlitCommandEncoder BlitCommandEncoder = GetCommandListContext()->GetBlitCommandEncoder();
		MTLOrigin Origin = MTLOriginMake(0, 0, 0);
		MTLSize Size = MTLSizeMake(InGpuTexture->GetWidth(), InGpuTexture->GetHeight(), 1);
		[BlitCommandEncoder.GetPtr() copyFromTexture:Texture->GetResource() sourceSlice:0 sourceLevel:0 sourceOrigin:Origin sourceSize:Size toBuffer:Texture->ReadBackBuffer->GetResource() destinationOffset:0 destinationBytesPerRow:OutRowPitch destinationBytesPerImage:UnpaddedSize];
		BlitCommandEncoder.EndEncoding();

		FlushGpu();
		return Texture->ReadBackBuffer->GetContents();
	} else {
		// TODO
		check(false);
		return nullptr;
	}
}

void MetalGpuRhiBackend::UnMapGpuTexture(GpuTexture *InGpuTexture)
{
	// do nothing.
}

void *MetalGpuRhiBackend::MapGpuBuffer(GpuBuffer *InGpuBuffer, GpuResourceMapMode InMapMode)
{
	GpuBufferUsage Usage = InGpuBuffer->GetUsage();
	MetalBuffer *Buffer = static_cast<MetalBuffer *>(InGpuBuffer);
	void *Data = nullptr;
	if (InMapMode == GpuResourceMapMode::Write_Only) {
		if (EnumHasAnyFlags(Usage, GpuBufferUsage::Static)) {

		} else {
			Data = Buffer->GetContents();
		}
	} else {
	}
	return Data;
}

void MetalGpuRhiBackend::UnMapGpuBuffer(GpuBuffer *InGpuBuffer)
{
	// do nothing.
}

bool MetalGpuRhiBackend::CrossCompileShader(GpuShader *InShader, FString &OutErrorInfo)
{
	return CompileShaderFromHlsl(static_cast<MetalShader *>(InShader), OutErrorInfo);
}

void MetalGpuRhiBackend::SetRenderPipelineState(GpuPipelineState *InPipelineState)
{
	GetCommandListContext()->SetPipeline(static_cast<MetalPipelineState *>(InPipelineState));
}

void MetalGpuRhiBackend::SetVertexBuffer(GpuBuffer *InVertexBuffer)
{
	GetCommandListContext()->SetVertexBuffer(static_cast<MetalBuffer *>(InVertexBuffer));
}

void MetalGpuRhiBackend::SetViewPort(const GpuViewPortDesc &InViewPortDesc)
{
	mtlpp::Viewport Viewport {
		(double)InViewPortDesc.TopLeftX, (double)InViewPortDesc.TopLeftY,
		(double)InViewPortDesc.Width, (double)InViewPortDesc.Height,
		(double)InViewPortDesc.ZMin, (double)InViewPortDesc.ZMax
	};

	mtlpp::ScissorRect ScissorRect { 0, 0, (uint32)InViewPortDesc.Width, (uint32)InViewPortDesc.Height };
	GetCommandListContext()->SetViewPort(MoveTemp(Viewport), MoveTemp(ScissorRect));
}

void MetalGpuRhiBackend::SetBindGroups(GpuBindGroup *BindGroup0, GpuBindGroup *BindGroup1, GpuBindGroup *BindGroup2, GpuBindGroup *BindGroup3)
{
	GetCommandListContext()->SetBindGroups(
		static_cast<MetalBindGroup *>(BindGroup0),
		static_cast<MetalBindGroup *>(BindGroup1),
		static_cast<MetalBindGroup *>(BindGroup2),
		static_cast<MetalBindGroup *>(BindGroup3));
}

void MetalGpuRhiBackend::DrawPrimitive(uint32 StartVertexLocation, uint32 VertexCount, uint32 StartInstanceLocation, uint32 InstanceCount)
{
	GetCommandListContext()->PrepareDrawingEnv();
	GetCommandListContext()->Draw(StartVertexLocation, VertexCount, StartInstanceLocation, InstanceCount);
}

void MetalGpuRhiBackend::Submit()
{
	id<MTLCommandBuffer> CommandBuffer = GetCommandListContext()->GetCommandBuffer();
	[CommandBuffer commit];
	GetCommandListContext()->SetCommandBuffer(GCommandQueue.CommandBuffer());
}

void MetalGpuRhiBackend::BeginGpuCapture(const FString &SavedFileName)
{
	// not implemented.
}

void MetalGpuRhiBackend::EndGpuCapture()
{
	// not implemented.
}

void MetalGpuRhiBackend::BeginCaptureEvent(const FString &EventName)
{
	// not implemented.
}

void MetalGpuRhiBackend::EndCaptureEvent()
{
	// not implemented.
}

void *MetalGpuRhiBackend::GetSharedHandle(GpuTexture *InGpuTexture)
{
	return static_cast<MetalTexture *>(InGpuTexture)->GetSharedHandle();
}

void MetalGpuRhiBackend::BeginRenderPass(const GpuRenderPassDesc &PassDesc, const FString &PassName)
{
	mtlpp::RenderPassDescriptor RenderPassDesc = MapRenderPassDesc(PassDesc);
	GetCommandListContext()->SetRenderPassDesc(MoveTemp(RenderPassDesc), PassName);
	GetCommandListContext()->ClearBinding();
}

void MetalGpuRhiBackend::EndRenderPass()
{
	id<MTLRenderCommandEncoder> RenderCommandEncoder = GetCommandListContext()->GetRenderCommandEncoder();
	[RenderCommandEncoder endEncoding];
}
}
