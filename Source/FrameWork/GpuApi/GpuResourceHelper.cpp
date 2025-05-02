#include "CommonHeader.h"
#include "GpuRhi.h"
#include "RenderResource/Shader/ClearShader.h"

namespace FW::GpuResourceHelper
{
	TRefCountPtr<GpuTexture> TempRenderTarget(GpuTextureFormat InFormat, GpuTextureUsage InUsage)
	{
		GpuTextureDesc Desc{ 1, 1, InFormat,  InUsage};
		return GGpuRhi->CreateTexture(MoveTemp(Desc));
	}

	GpuTexture* GetGlobalBlackTex()
	{
        TArray<uint8> RawData = {0,0,0,1};
		GpuTextureDesc Desc{ 1, 1, GpuTextureFormat::R8G8B8A8_UNORM, GpuTextureUsage::ShaderResource , RawData};
		static TRefCountPtr<GpuTexture> GlobalBlackTex = GGpuRhi->CreateTexture(MoveTemp(Desc));
		return GlobalBlackTex;
	}

	void ClearRWResource(GpuCmdRecorder* CmdRecorder, GpuResource* InResource)
	{
		uint32 ThreadGroupCountX = 1;
		TRefCountPtr<GpuShader> Cs;
		TRefCountPtr<GpuBindGroupLayout> BindGroupLayout;
		TRefCountPtr<GpuBindGroup> BindGroup;
		if(InResource->GetType() == GpuResourceType::Buffer)
		{
			GpuBuffer* Buffer = static_cast<GpuBuffer*>(InResource);
			uint32 BufferByteSize = Buffer->GetByteSize();
			if (Buffer->GetUsage() == GpuBufferUsage::RWStorage)
			{
				auto* ClearShader = GetClearShader<BindingType::RWStorageBuffer>();
				Cs = ClearShader->GetComputeShader();
				BindGroupLayout = ClearShader->GetBindGroupLayout();
				BindGroup = ClearShader->GetBindGroup(InResource, BufferByteSize);
				ThreadGroupCountX = FMath::CeilToInt(float(BufferByteSize / 4) / 64);
			}
		}

		check(Cs && BindGroupLayout && BindGroup);

		BindingContext Bindings;
		Bindings.SetShaderBindGroup(BindGroup);
		Bindings.SetShaderBindGroupLayout(BindGroupLayout);

		GpuComputePipelineStateDesc PipelineDesc{ .Cs = Cs };
		Bindings.ApplyBindGroupLayout(PipelineDesc);
		TRefCountPtr<GpuComputePipelineState> Pipeline = GGpuRhi->CreateComputePipelineState(PipelineDesc);

		auto PassRecorder = CmdRecorder->BeginComputePass("ClearRWResource");
		{
			Bindings.ApplyBindGroup(PassRecorder);
			PassRecorder->SetComputePipelineState(Pipeline);
			PassRecorder->Dispatch(ThreadGroupCountX, 1, 1);
		}
		CmdRecorder->EndComputePass(PassRecorder);
	}
}
