#include "CommonHeader.h"
#include "GpuRhi.h"
#include "RenderResource/Shader/ClearShader.h"
#include "RenderResource/Shader/Shader.h"
#include "RenderResource/RenderPass/BlitPass.h"

namespace FW::GpuResourceHelper
{
	TRefCountPtr<GpuTexture> TempRenderTarget(GpuFormat InFormat, GpuTextureUsage InUsage)
	{
		GpuTextureDesc Desc{ 1, 1, InFormat,  InUsage};
		return GGpuRhi->CreateTexture(MoveTemp(Desc));
	}

	FRAMEWORK_API GpuSampler* GetSampler(const GpuSamplerDesc& InDesc)
	{
		static TArray<TRefCountPtr<GpuSampler>> GpuSamplerCache;
		if (auto* ExistingSampler = GpuSamplerCache.FindByPredicate([&](auto&& Item) { return InDesc == Item->GetDesc();}))
		{
			return *ExistingSampler;
		}
		auto NewSampler = GGpuRhi->CreateSampler(InDesc);
		GpuSamplerCache.Add(NewSampler);
		return NewSampler;
	}

	GpuTexture* GetGlobalBlackTex()
	{
        TArray<uint8> RawData = {0,0,0,255};
		GpuTextureDesc Desc{ 1, 1, GpuFormat::B8G8R8A8_UNORM, GpuTextureUsage::ShaderResource , RawData};
		static TRefCountPtr<GpuTexture> GlobalBlackTex = GGpuRhi->CreateTexture(MoveTemp(Desc));
		return GlobalBlackTex;
	}

	GpuTexture* GetGlobalBlackCubemapTex()
	{
		static TRefCountPtr<GpuTexture> GlobalBlackCubemap = []() {
			GpuTextureDesc Desc;
			Desc.Width = 1;
			Desc.Height = 1;
			Desc.Format = GpuFormat::B8G8R8A8_UNORM;
			Desc.Usage = GpuTextureUsage::ShaderResource;
			Desc.Dimension = GpuTextureDimension::TexCube;

			TRefCountPtr<GpuTexture> DefaultCubemap = GGpuRhi->CreateTexture(Desc, GpuResourceState::CopyDst);

			TArray<uint8> RawData = { 0, 0, 0, 255 };
			TRefCountPtr<GpuBuffer> UploadBuffer = GGpuRhi->CreateBuffer(
				{ (uint32)RawData.Num(), GpuBufferUsage::Upload },
				GpuResourceState::CopySrc
			);
			void* Mapped = GGpuRhi->MapGpuBuffer(UploadBuffer, GpuResourceMapMode::Write_Only);
			FMemory::Memcpy(Mapped, RawData.GetData(), RawData.Num());
			GGpuRhi->UnMapGpuBuffer(UploadBuffer);

			GpuCmdRecorder* CmdRecorder = GGpuRhi->BeginRecording(TEXT("InitDefaultCubemap"));
			for (uint32 FaceIndex = 0; FaceIndex < 6; ++FaceIndex)
			{
				CmdRecorder->CopyBufferToTexture(UploadBuffer, DefaultCubemap, FaceIndex, 0);
			}
			CmdRecorder->Barriers({ { DefaultCubemap, GpuResourceState::ShaderResourceRead } });
			GGpuRhi->EndRecording(CmdRecorder);
			GGpuRhi->Submit({ CmdRecorder });

			return DefaultCubemap;
		}();

		return GlobalBlackCubemap;
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
			if (Buffer->GetUsage() == GpuBufferUsage::RWStructured)
			{
				using ClearShaderType = ClearShader<BindingType::RWStructuredBuffer>;
				auto* ClearShader = GetShader<ClearShaderType>();
				Cs = ClearShader->GetComputeShader();
				BindGroupLayout = ClearShader->GetBindGroupLayout();
				BindGroup = ClearShader->GetBindGroup(InResource, BufferByteSize);
				ThreadGroupCountX = FMath::CeilToInt(float(BufferByteSize / 4) / 64);
			}
			else if (Buffer->GetUsage() == GpuBufferUsage::RWRaw)
			{
				using ClearShaderType = ClearShader<BindingType::RWRawBuffer>;
				auto* ClearShader = GetShader<ClearShaderType>();
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
		TRefCountPtr<GpuComputePipelineState> Pipeline = GpuPsoCacheManager::Get().CreateComputePipelineState(PipelineDesc);

		auto PassRecorder = CmdRecorder->BeginComputePass("ClearRWResource");
		{
			Bindings.ApplyBindGroup(PassRecorder);
			PassRecorder->SetComputePipelineState(Pipeline);
			PassRecorder->Dispatch(ThreadGroupCountX, 1, 1);
		}
		CmdRecorder->EndComputePass(PassRecorder);
	}

	void GenerateMipmap(RenderGraph& Graph, GpuTexture* InTexture)
	{
		uint32 NumMips = InTexture->GetNumMips();
		if (NumMips <= 1) return;

		GpuSampler* BilinearSampler = GetSampler({SamplerFilter::Bilinear});

		auto PrevView = GGpuRhi->CreateTextureView({InTexture, 0, 1});

		for (uint32 MipLevel = 1; MipLevel < NumMips; MipLevel++)
		{
			auto DstView = GGpuRhi->CreateTextureView({InTexture, MipLevel, 1});

			BlitPassInput Input;
			Input.InputView = PrevView;
			Input.InputTexSampler = BilinearSampler;
			Input.OutputView = DstView;

			AddBlitPass(Graph, Input);

			PrevView = DstView;
		}
	}
}
