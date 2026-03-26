#include "CommonHeader.h"
#include "GpuRhi.h"
#include "RenderResource/Shader/ClearShader.h"
#include "RenderResource/Shader/Shader.h"
#include "Common/Path/PathHelper.h"
#include "GpuPipelineState.h"

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
		GpuTextureDesc Desc{ 1, 1, GpuFormat::R8G8B8A8_UNORM, GpuTextureUsage::ShaderResource , RawData};
		static TRefCountPtr<GpuTexture> GlobalBlackTex = GGpuRhi->CreateTexture(MoveTemp(Desc));
		return GlobalBlackTex;
	}

	GpuTexture* GetGlobalBlackCubemapTex()
	{
		static TRefCountPtr<GpuTexture> GlobalBlackCubemap = []() {
			GpuTextureDesc Desc;
			Desc.Width = 1;
			Desc.Height = 1;
			Desc.Format = GpuFormat::R8G8B8A8_UNORM;
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

	GpuTexture* GetGlobalBlackVolumeTex()
	{
		TArray<uint8> RawData = {0, 0, 0, 255};
		GpuTextureDesc Desc{};
		Desc.Width = 1;
		Desc.Height = 1;
		Desc.Depth = 1;
		Desc.Format = GpuFormat::R8G8B8A8_UNORM;
		Desc.Usage = GpuTextureUsage::ShaderResource;
		Desc.Dimension = GpuTextureDimension::Tex3D;
		Desc.InitialData = RawData;
		static TRefCountPtr<GpuTexture> GlobalBlackVolumeTex = GGpuRhi->CreateTexture(MoveTemp(Desc));
		return GlobalBlackVolumeTex;
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

		GpuComputePipelineStateDesc PipelineDesc{ 
			.Cs = Cs,
			.BindGroupLayout2 = BindGroupLayout
		};
		TRefCountPtr<GpuComputePipelineState> Pipeline = GpuPsoCacheManager::Get().CreateComputePipelineState(PipelineDesc);

		auto PassRecorder = CmdRecorder->BeginComputePass("ClearRWResource");
		{
			PassRecorder->SetBindGroups(nullptr, nullptr, BindGroup, nullptr);
			PassRecorder->SetComputePipelineState(Pipeline);
			PassRecorder->Dispatch(ThreadGroupCountX, 1, 1);
		}
		CmdRecorder->EndComputePass(PassRecorder);
	}

	struct MipResources
	{
		static constexpr uint32 BindGroupSlot = 2;
		static constexpr uint32 InputTexBinding = 0;
		static constexpr uint32 SamplerBinding = 1;
		static constexpr uint32 OutputTexBinding = 2;
		static constexpr uint32 ParamsBinding = 3;

		TRefCountPtr<GpuShader> Cs2D;
		TRefCountPtr<GpuShader> Cs3D;
		TRefCountPtr<GpuBindGroupLayout> Layout2D;
		TRefCountPtr<GpuBindGroupLayout> Layout3D;
	};

	static const MipResources& GetMipResources()
	{
		static MipResources Resources = [] {
			MipResources Result;

			Result.Layout2D = GpuBindGroupLayoutBuilder{ MipResources::BindGroupSlot }
				.AddExistingBinding(MipResources::InputTexBinding, BindingType::Texture, BindingShaderStage::Compute)
				.AddExistingBinding(MipResources::SamplerBinding, BindingType::Sampler, BindingShaderStage::Compute)
				.AddExistingBinding(MipResources::OutputTexBinding, BindingType::RWTexture, BindingShaderStage::Compute)
				.AddExistingBinding(MipResources::ParamsBinding, BindingType::UniformBuffer, BindingShaderStage::Compute)
				.Build();

			Result.Layout3D = GpuBindGroupLayoutBuilder{ MipResources::BindGroupSlot }
				.AddExistingBinding(MipResources::InputTexBinding, BindingType::Texture3D, BindingShaderStage::Compute)
				.AddExistingBinding(MipResources::SamplerBinding, BindingType::Sampler, BindingShaderStage::Compute)
				.AddExistingBinding(MipResources::OutputTexBinding, BindingType::RWTexture3D, BindingShaderStage::Compute)
				.AddExistingBinding(MipResources::ParamsBinding, BindingType::UniformBuffer, BindingShaderStage::Compute)
				.Build();

			Result.Cs2D = GGpuRhi->CreateShaderFromFile({
				.FileName = PathHelper::ShaderDir() / "Mip2D.hlsl",
				.Type = ShaderType::ComputeShader,
				.EntryPoint = "MainCS"
			});

			Result.Cs3D = GGpuRhi->CreateShaderFromFile({
				.FileName = PathHelper::ShaderDir() / "Mip3D.hlsl",
				.Type = ShaderType::ComputeShader,
				.EntryPoint = "MainCS"
			});

			FString ErrorInfo, WarnInfo;
			GGpuRhi->CompileShader(Result.Cs2D, ErrorInfo, WarnInfo);
			check(ErrorInfo.IsEmpty());
			GGpuRhi->CompileShader(Result.Cs3D, ErrorInfo, WarnInfo);
			check(ErrorInfo.IsEmpty());

			return Result;
		}();

		return Resources;
	}

	static void DispatchMip2D(GpuCmdRecorder* CmdRecorder,
		GpuTextureView* SrcView, GpuTextureView* DstView, GpuSampler* Sampler, uint32 DstWidth, uint32 DstHeight)
	{
		const MipResources& Mip = GetMipResources();

		TRefCountPtr<GpuBuffer> ParamBuf = GGpuRhi->CreateBuffer({
			.ByteSize = 256,
			.Usage = GpuBufferUsage::Uniform | GpuBufferUsage::Temporary
		});
		void* Mapped = GGpuRhi->MapGpuBuffer(ParamBuf, GpuResourceMapMode::Write_Only);
		FMemory::Memzero(Mapped, 256);
		uint32* Params = (uint32*)Mapped;
		Params[0] = DstWidth;
		Params[1] = DstHeight;
		GGpuRhi->UnMapGpuBuffer(ParamBuf);

		TRefCountPtr<GpuBindGroup> BindGroup = GpuBindGroupBuilder{ Mip.Layout2D }
			.SetExistingBinding(MipResources::InputTexBinding, SrcView)
			.SetExistingBinding(MipResources::SamplerBinding, Sampler)
			.SetExistingBinding(MipResources::OutputTexBinding, DstView)
			.SetExistingBinding(MipResources::ParamsBinding, ParamBuf)
			.Build();

		CmdRecorder->Barriers({
			{SrcView, GpuResourceState::ShaderResourceRead},
			{DstView, GpuResourceState::UnorderedAccess}
		});

		GpuComputePipelineStateDesc PipelineDesc{ 
			.Cs = Mip.Cs2D,
			.BindGroupLayout2 = Mip.Layout2D
		};
		TRefCountPtr<GpuComputePipelineState> Pipeline = GpuPsoCacheManager::Get().CreateComputePipelineState(PipelineDesc);

		auto PassRecorder = CmdRecorder->BeginComputePass("Mip2DPass");
		{
			PassRecorder->SetBindGroups(nullptr, nullptr, BindGroup, nullptr);
			PassRecorder->SetComputePipelineState(Pipeline);
			PassRecorder->Dispatch(
				FMath::CeilToInt((float)DstWidth / 8.0f),
				FMath::CeilToInt((float)DstHeight / 8.0f),
				1
			);
		}
		CmdRecorder->EndComputePass(PassRecorder);
	}

	static void DispatchMip3D(GpuCmdRecorder* CmdRecorder,
		GpuTextureView* SrcView, GpuTextureView* DstView, GpuSampler* Sampler, uint32 DstWidth, uint32 DstHeight, uint32 DstDepth)
	{
		const MipResources& Mip = GetMipResources();

		TRefCountPtr<GpuBuffer> ParamBuf = GGpuRhi->CreateBuffer({
			.ByteSize = 256,
			.Usage = GpuBufferUsage::Uniform | GpuBufferUsage::Temporary
		});
		void* Mapped = GGpuRhi->MapGpuBuffer(ParamBuf, GpuResourceMapMode::Write_Only);
		FMemory::Memzero(Mapped, 256);
		uint32* Params = (uint32*)Mapped;
		Params[0] = DstWidth;
		Params[1] = DstHeight;
		Params[2] = DstDepth;
		GGpuRhi->UnMapGpuBuffer(ParamBuf);

		TRefCountPtr<GpuBindGroup> BindGroup = GpuBindGroupBuilder{ Mip.Layout3D }
			.SetExistingBinding(MipResources::InputTexBinding, SrcView)
			.SetExistingBinding(MipResources::SamplerBinding, Sampler)
			.SetExistingBinding(MipResources::OutputTexBinding, DstView)
			.SetExistingBinding(MipResources::ParamsBinding, ParamBuf)
			.Build();

		CmdRecorder->Barriers({
			{SrcView, GpuResourceState::ShaderResourceRead},
			{DstView, GpuResourceState::UnorderedAccess}
		});

		GpuComputePipelineStateDesc PipelineDesc{ 
			.Cs = Mip.Cs3D,
			.BindGroupLayout2 = Mip.Layout3D
		};
		TRefCountPtr<GpuComputePipelineState> Pipeline = GpuPsoCacheManager::Get().CreateComputePipelineState(PipelineDesc);

		auto PassRecorder = CmdRecorder->BeginComputePass("Mip3DPass");
		{
			PassRecorder->SetBindGroups(nullptr, nullptr, BindGroup, nullptr);
			PassRecorder->SetComputePipelineState(Pipeline);
			PassRecorder->Dispatch(
				FMath::CeilToInt((float)DstWidth / 4.0f),
				FMath::CeilToInt((float)DstHeight / 4.0f),
				FMath::CeilToInt((float)DstDepth / 4.0f)
			);
		}
		CmdRecorder->EndComputePass(PassRecorder);
	}

	void GenerateMipmap(GpuTexture* InTexture)
	{
		uint32 NumMips = InTexture->GetNumMips();
		if (NumMips <= 1) return;
		GpuResourceState InitialState = InTexture->GetSubResourceState(0, 0);

		GpuSampler* BilinearSampler = GetSampler({SamplerFilter::Bilinear});
		GpuTextureDimension Dimension = InTexture->GetResourceDesc().Dimension;

		if (Dimension == GpuTextureDimension::Tex3D)
		{
			GpuCmdRecorder* CmdRecorder = GGpuRhi->BeginRecording(TEXT("GenerateMipmap3D"));

			for (uint32 MipLevel = 1; MipLevel < NumMips; MipLevel++)
			{
				auto SrcView = GGpuRhi->CreateTextureView({InTexture, MipLevel - 1, 1});
				auto DstView = GGpuRhi->CreateTextureView({InTexture, MipLevel, 1});

				uint32 DstWidth  = FMath::Max(InTexture->GetWidth()  >> MipLevel, 1u);
				uint32 DstHeight = FMath::Max(InTexture->GetHeight() >> MipLevel, 1u);
				uint32 DstDepth  = FMath::Max(InTexture->GetDepth()  >> MipLevel, 1u);

				DispatchMip3D(CmdRecorder, SrcView, DstView, BilinearSampler, DstWidth, DstHeight, DstDepth);
			}

			CmdRecorder->Barriers({ {InTexture, InitialState} });
			GGpuRhi->EndRecording(CmdRecorder);
			GGpuRhi->Submit({ CmdRecorder });
		}
		else
		{
			GpuCmdRecorder* CmdRecorder = GGpuRhi->BeginRecording(TEXT("GenerateMipmap2D"));

			if (Dimension == GpuTextureDimension::TexCube)
			{
				for (uint32 Face = 0; Face < 6; Face++)
				{
					for (uint32 MipLevel = 1; MipLevel < NumMips; MipLevel++)
					{
						auto SrcView = GGpuRhi->CreateTextureView({InTexture, MipLevel - 1, 1, Face, 1});
						auto DstView = GGpuRhi->CreateTextureView({InTexture, MipLevel, 1, Face, 1});

						uint32 DstWidth  = FMath::Max(InTexture->GetWidth()  >> MipLevel, 1u);
						uint32 DstHeight = FMath::Max(InTexture->GetHeight() >> MipLevel, 1u);

						DispatchMip2D(CmdRecorder, SrcView, DstView, BilinearSampler, DstWidth, DstHeight);
					}
				}
			}
			else
			{
				for (uint32 MipLevel = 1; MipLevel < NumMips; MipLevel++)
				{
					auto SrcView = GGpuRhi->CreateTextureView({InTexture, MipLevel - 1, 1});
					auto DstView = GGpuRhi->CreateTextureView({InTexture, MipLevel, 1});

					uint32 DstWidth  = FMath::Max(InTexture->GetWidth()  >> MipLevel, 1u);
					uint32 DstHeight = FMath::Max(InTexture->GetHeight() >> MipLevel, 1u);

					DispatchMip2D(CmdRecorder, SrcView, DstView, BilinearSampler, DstWidth, DstHeight);
				}
			}

			CmdRecorder->Barriers({ {InTexture, InitialState} });
			GGpuRhi->EndRecording(CmdRecorder);
			GGpuRhi->Submit({ CmdRecorder });
		}
	}
}
