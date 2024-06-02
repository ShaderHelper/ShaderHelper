#include "CommonHeader.h"
#include "Dx12Texture.h"
#include "Dx12Device.h"
#include "Dx12CommandRecorder.h"
#include "Dx12Map.h"
#include "Dx12GpuRhiBackend.h"

namespace FRAMEWORK
{
	struct FlagSets {
		bool bShared;
		bool bRTV;
		bool bSRV;
	};

	Dx12Texture::Dx12Texture(TRefCountPtr<ID3D12Resource> InResource, GpuTextureDesc InDesc, void* InSharedHandle)
		: GpuTexture(MoveTemp(InDesc))
		, Resource(MoveTemp(InResource))
		, SharedHandle(InSharedHandle)
	{

	}

	static bool ValidateTexture(const GpuTextureDesc& InTexDesc)
	{
		if (InTexDesc.Width <= 0 || InTexDesc.Height <= 0) {
			SH_LOG(LogDx12, Error, TEXT("Invalid Texture dimensions %ux%u"), InTexDesc.Width, InTexDesc.Height);
			return false;
		}

		if (InTexDesc.NumMips != 1) {
			SH_LOG(LogDx12, Error, TEXT("Invalid Texture MipLevels. TODO: Support texture subresource"));
			return false;
		}

		if (InTexDesc.Depth > 1) {
			SH_LOG(LogDx12, Error, TEXT("Invalid Texture Depth. TODO: Support 3d texture"));
			return false;
		}
		
		return true;
	}

	static void GetDx12ResourceFlags(GpuTextureUsage InFlags, D3D12_RESOURCE_FLAGS& OutResourceFlag, FlagSets& OutFlags)
	{
		if (EnumHasAnyFlags(InFlags, GpuTextureUsage::RenderTarget)) {
			OutResourceFlag |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			OutFlags.bRTV = true;
		}

		if (EnumHasAnyFlags(InFlags, GpuTextureUsage::Shared)) {
			OutResourceFlag |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
			OutFlags.bShared = true;
		}

		if (EnumHasAnyFlags(InFlags, GpuTextureUsage::ShaderResource)) {
			OutFlags.bSRV = true;
		}
	}

	static D3D12_RESOURCE_STATES GetInitialResourceState(const FlagSets& InFlags, GpuResourceState& OutState)
	{
		bool bWritable = InFlags.bRTV;
		bool bReadable = InFlags.bSRV;
		check(!(bWritable && bReadable));

		if (bReadable)
		{
			if (InFlags.bSRV) {
				OutState = GpuResourceState::ShaderResourceRead;
				return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
			}

		}
		else
		{
			if (InFlags.bRTV) {
				OutState = GpuResourceState::RenderTargetWrite;
				return D3D12_RESOURCE_STATE_RENDER_TARGET;
			}
		}

		check(false);
		return D3D12_RESOURCE_STATE_COMMON;
	}

	static void CreateTextureView(const FlagSets& InFlags, Dx12Texture* InTexture)
	{

		if (InFlags.bSRV) {
			InTexture->SRV = AllocCpuCbvSrvUav();
			GDevice->CreateShaderResourceView(InTexture->GetResource(), nullptr, InTexture->SRV->GetHandle());
		}

		if (InFlags.bRTV) {
			InTexture->RTV = AllocRtv();
			GDevice->CreateRenderTargetView(InTexture->GetResource(), nullptr, InTexture->RTV->GetHandle());
		}
	}

	TRefCountPtr<Dx12Texture> CreateDx12Texture2D(const GpuTextureDesc& InTexDesc, GpuResourceState InitState)
	{
		if (!ValidateTexture(InTexDesc)) {
			return nullptr;
		}

		bool bHasInitialData = false;
		if (!InTexDesc.InitialData.IsEmpty())
		{
			bHasInitialData = true;
		}

		D3D12_RESOURCE_FLAGS ResourceFlags = D3D12_RESOURCE_FLAG_NONE;
		
		FlagSets Flags{};
		GetDx12ResourceFlags(InTexDesc.Usage, ResourceFlags, Flags);

		CD3DX12_RESOURCE_DESC TexDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			MapTextureFormat(InTexDesc.Format),
			InTexDesc.Width,
			InTexDesc.Height,
			InTexDesc.Depth,
			InTexDesc.NumMips,
			1,0,
			ResourceFlags
		);

		CD3DX12_HEAP_PROPERTIES HeapType{ D3D12_HEAP_TYPE_DEFAULT };
		D3D12_HEAP_FLAGS HeapFlag = Flags.bShared ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE;

		D3D12_RESOURCE_STATES FinalState = InitState == GpuResourceState::Unknown ? GetInitialResourceState(Flags, InitState) : MapResourceState(InitState);
		D3D12_RESOURCE_STATES InitialState = bHasInitialData ? D3D12_RESOURCE_STATE_COPY_DEST : FinalState;

		//Fast Clear Optimization
		const float ClearColor[4] = { InTexDesc.ClearValues.X, InTexDesc.ClearValues.Y, InTexDesc.ClearValues.Z, InTexDesc.ClearValues.W };
		CD3DX12_CLEAR_VALUE ClearValues{ MapTextureFormat(InTexDesc.Format), ClearColor };

		TRefCountPtr<ID3D12Resource> TexResource;

		if(Flags.bRTV)
		{ 
			DxCheck(GDevice->CreateCommittedResource(&HeapType, HeapFlag,
				&TexDesc, InitialState, &ClearValues, IID_PPV_ARGS(TexResource.GetInitReference())));
		}
		else
		{
			DxCheck(GDevice->CreateCommittedResource(&HeapType, HeapFlag,
				&TexDesc, InitialState, nullptr, IID_PPV_ARGS(TexResource.GetInitReference())));
		}

		void* SharedHandle = nullptr;
		if (Flags.bShared)
		{
			GDevice->CreateSharedHandle(TexResource, nullptr, GENERIC_ALL, nullptr, &SharedHandle);
		}

		TRefCountPtr<Dx12Texture> RetTexture = new Dx12Texture{ MoveTemp(TexResource), InTexDesc, SharedHandle };
		RetTexture->State = bHasInitialData ? GpuResourceState::CopyDst : InitState;
		CreateTextureView(Flags, RetTexture);

		TRefCountPtr<Dx12Buffer> UploadBuffer;
		if (bHasInitialData) {
			const uint32 UploadBufferSize = (uint32)GetRequiredIntermediateSize(RetTexture->GetResource(), 0, 1);
			UploadBuffer = CreateDx12Buffer(D3D12_RESOURCE_STATE_COPY_SOURCE, UploadBufferSize, GpuBufferUsage::Dynamic);

			D3D12_SUBRESOURCE_DATA textureData = {};
			textureData.pData = &InTexDesc.InitialData[0];
			textureData.RowPitch = InTexDesc.Width * GetTextureFormatByteSize(InTexDesc.Format);
			textureData.SlicePitch = textureData.RowPitch * InTexDesc.Height;

			const CommonAllocationData& AllocationData = UploadBuffer->GetAllocation().GetAllocationData().Get<CommonAllocationData>();
			GpuCmdRecorder* CmdRecorder = GDx12GpuRhi->BeginRecording();
			{
				UpdateSubresources(static_cast<Dx12CmdRecorder*>(CmdRecorder)->GetCommandList(), RetTexture->GetResource(), AllocationData.UnderlyResource, 0, 0, 1, &textureData);
				CmdRecorder->Barrier(RetTexture, InitState);
			}
			GDx12GpuRhi->EndRecording(CmdRecorder);
			GDx12GpuRhi->Submit({ CmdRecorder });
		}
	
		return RetTexture;
	}

	TRefCountPtr<Dx12Sampler> CreateDx12Sampler(const GpuSamplerDesc& InSamplerDesc)
	{
		D3D12_SAMPLER_DESC DxSamplerDesc{};
		DxSamplerDesc.AddressU = MapTextureAddressMode(InSamplerDesc.AddressU);
		DxSamplerDesc.AddressV = MapTextureAddressMode(InSamplerDesc.AddressV);
		DxSamplerDesc.AddressW = MapTextureAddressMode(InSamplerDesc.AddressW);
		DxSamplerDesc.ComparisonFunc = MapComparisonFunc(InSamplerDesc.Compare);
		DxSamplerDesc.MipLODBias = 0;
		DxSamplerDesc.MaxAnisotropy = 1;
		DxSamplerDesc.MinLOD = 0;
		DxSamplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

		bool ComparisonEnable = InSamplerDesc.Compare != CompareMode::Never;

		switch (InSamplerDesc.Filer)
		{
		case SamplerFilter::Point:			
			DxSamplerDesc.Filter = ComparisonEnable ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT : D3D12_FILTER_MIN_MAG_MIP_POINT;
			break;
		case SamplerFilter::Bilinear:
			DxSamplerDesc.Filter = ComparisonEnable ? D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT : D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case SamplerFilter::Trilinear:
			DxSamplerDesc.Filter = ComparisonEnable ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		}

		TUniquePtr<CpuDescriptor> SamplerDescriptor = AllocSampler();
		GDevice->CreateSampler(&DxSamplerDesc, SamplerDescriptor->GetHandle());

		return new Dx12Sampler(MoveTemp(SamplerDescriptor));
	}

}
