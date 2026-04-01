#include "CommonHeader.h"
#include "Dx12RS.h"
#include "Dx12Device.h"
#include "Dx12Buffer.h"
#include "Dx12CommandRecorder.h"

namespace FW
{
	static D3D12_SHADER_VISIBILITY ResolveDx12ShaderVisibility(BindingShaderStage InStage)
	{
		if (InStage == BindingShaderStage::Vertex)
		{
			return D3D12_SHADER_VISIBILITY_VERTEX;
		}

		if (InStage == BindingShaderStage::Pixel)
		{
			return D3D12_SHADER_VISIBILITY_PIXEL;
		}
		
		return D3D12_SHADER_VISIBILITY_ALL;
	}

	D3D12_DESCRIPTOR_RANGE_TYPE BindingTypeToDescriptorRangeType(BindingType InType)
	{
		switch (InType)
		{
		case BindingType::Texture:
		case BindingType::TextureCube:
		case BindingType::Texture3D:
		case BindingType::CombinedTextureSampler:
		case BindingType::CombinedTextureCubeSampler:
		case BindingType::CombinedTexture3DSampler:
		case BindingType::StructuredBuffer:
		case BindingType::RawBuffer:
			return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		case BindingType::Sampler:			return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		case BindingType::RWStructuredBuffer:
		case BindingType::RWRawBuffer:
		case BindingType::RWTexture:
		case BindingType::RWTexture3D:
			return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		default:
			AUX::Unreachable();
		}
	}

	Dx12BindGroupLayout::Dx12BindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc)
		: GpuBindGroupLayout(LayoutDesc)
	{
		auto SetRootParameterInfo = [&](BindingSlot Slot,const LayoutBinding& LayoutBindingEntry, D3D12_SHADER_VISIBILITY BindingVisibility)
		{
			//For the moment, all uniformbuffers in the layout are bound via root descriptor.
			if (LayoutBindingEntry.Type == BindingType::UniformBuffer)
			{
				CD3DX12_ROOT_PARAMETER1 DynamicBufferRootParameter;
				DynamicBufferRootParameter.InitAsConstantBufferView(Slot, Desc.GroupNumber, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, BindingVisibility);
				DynamicBufferRootParameters.Add(Slot, MoveTemp(DynamicBufferRootParameter));
			}
			else
			{
				CD3DX12_DESCRIPTOR_RANGE1 Range{};
				Range.Init(
					BindingTypeToDescriptorRangeType(LayoutBindingEntry.Type),
					1,
					Slot,
					LayoutDesc.GroupNumber);

				if (LayoutBindingEntry.Type == BindingType::CombinedTextureSampler || LayoutBindingEntry.Type == BindingType::CombinedTextureCubeSampler || LayoutBindingEntry.Type == BindingType::CombinedTexture3DSampler)
				{
					DescriptorTableRanges_CbvSrvUav.FindOrAdd(BindingVisibility).Add(MoveTemp(Range));

					CD3DX12_DESCRIPTOR_RANGE1 SamplerRange{};
					SamplerRange.Init(
						D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
						1,
						Slot,
						LayoutDesc.GroupNumber);
					DescriptorTableRanges_Sampler.FindOrAdd(BindingVisibility).Add(MoveTemp(SamplerRange));
				}
				else if (LayoutBindingEntry.Type == BindingType::Texture || LayoutBindingEntry.Type == BindingType::TextureCube || LayoutBindingEntry.Type == BindingType::Texture3D ||
					LayoutBindingEntry.Type == BindingType::RWStructuredBuffer || LayoutBindingEntry.Type == BindingType::StructuredBuffer ||
					LayoutBindingEntry.Type == BindingType::RWRawBuffer || LayoutBindingEntry.Type == BindingType::RawBuffer ||
					LayoutBindingEntry.Type == BindingType::RWTexture || LayoutBindingEntry.Type == BindingType::RWTexture3D)
				{
					DescriptorTableRanges_CbvSrvUav.FindOrAdd(BindingVisibility).Add(MoveTemp(Range));
				}
				else if (LayoutBindingEntry.Type == BindingType::Sampler)
				{
					DescriptorTableRanges_Sampler.FindOrAdd(BindingVisibility).Add(MoveTemp(Range));
				}
				else
				{
					AUX::Unreachable();
				}
			}
		};

		for (const auto& [Slot, LayoutBindingEntry] : Desc.Layouts)
		{
			SetRootParameterInfo(Slot, LayoutBindingEntry, ResolveDx12ShaderVisibility(LayoutBindingEntry.Stage));
		}

		for (const auto& [BindingVisibility, Ranges] : DescriptorTableRanges_CbvSrvUav)
		{
			CD3DX12_ROOT_PARAMETER1 DescriptorTableRootParameter;
			DescriptorTableRootParameter.InitAsDescriptorTable((uint32)Ranges.Num(), Ranges.GetData(), BindingVisibility);

			DescriptorTableRootParameters_CbvSrvUav.Add(BindingVisibility, MoveTemp(DescriptorTableRootParameter));
		}

		for (const auto& [BindingVisibility, Ranges] : DescriptorTableRanges_Sampler)
		{
			CD3DX12_ROOT_PARAMETER1 DescriptorTableRootParameter;
			DescriptorTableRootParameter.InitAsDescriptorTable((uint32)Ranges.Num(), Ranges.GetData(), BindingVisibility);

			DescriptorTableRootParameters_Sampler.Add(BindingVisibility, MoveTemp(DescriptorTableRootParameter));
		}
	}

	TRefCountPtr<Dx12BindGroupLayout> CreateDx12BindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc)
	{
		return new Dx12BindGroupLayout(LayoutDesc);
	}

	TRefCountPtr<Dx12BindGroup> CreateDx12BindGroup(const GpuBindGroupDesc& Desc)
	{
		return new Dx12BindGroup(Desc);
	}

	Dx12RootSignature* Dx12RootSignatureManager::GetRootSignature(const RootSignatureDesc& InDesc)
	{
		static Dx12RootSignatureManager Manager;

		if (auto RootSignaturePtr = Manager.RootSignatureCache.Find(InDesc))
		{
			return *RootSignaturePtr;
		}
		else
		{
			Dx12RootSignature* RootSignature = new Dx12RootSignature(InDesc);
			Manager.RootSignatureCache.Add(InDesc, RootSignature);
			return RootSignature;
		}
	}

	Dx12BindGroup::Dx12BindGroup(const GpuBindGroupDesc& InDesc)
		: GpuBindGroup(InDesc)
	{
		BindingGroupSlot GoupSlot = GetLayout()->GetGroupNumber();
		TMap<D3D12_SHADER_VISIBILITY, TArray<D3D12_CPU_DESCRIPTOR_HANDLE>> SrcDescriptorRange_CbvSrvUav;
		TMap<D3D12_SHADER_VISIBILITY, TArray<D3D12_CPU_DESCRIPTOR_HANDLE>> SrcDescriptorRange_Sampler;

		auto SetBindingTable = [&](BindingSlot Slot, const ResourceBinding& ResourceBindingEntry, D3D12_SHADER_VISIBILITY BindingVisibility)
		{
			GpuResource* BindingResource = ResourceBindingEntry.Resource;
			if (BindingResource->GetType() == GpuResourceType::Sampler)
			{
				Dx12Sampler* Sampler = static_cast<Dx12Sampler*>(BindingResource);
				SrcDescriptorRange_Sampler.FindOrAdd(BindingVisibility).Add(Sampler->GetCpuDescriptor()->GetHandle());
			}
			else if (BindingResource->GetType() == GpuResourceType::Texture)
			{
				Dx12Texture* Texture = static_cast<Dx12Texture*>(BindingResource);
				SrcDescriptorRange_CbvSrvUav.FindOrAdd(BindingVisibility).Add(Texture->GetDx12DefaultView()->GetSRV()->GetHandle());
			}
			else if (BindingResource->GetType() == GpuResourceType::TextureView)
			{
				Dx12TextureView* View = static_cast<Dx12TextureView*>(BindingResource);
				BindingType SlotType = GetLayout()->GetDesc().GetBindingType(Slot);
				if (SlotType == BindingType::RWTexture || SlotType == BindingType::RWTexture3D)
				{
					SrcDescriptorRange_CbvSrvUav.FindOrAdd(BindingVisibility).Add(View->GetUAV()->GetHandle());
				}
				else
				{
					SrcDescriptorRange_CbvSrvUav.FindOrAdd(BindingVisibility).Add(View->GetSRV()->GetHandle());
				}
			}
			else if (BindingResource->GetType() == GpuResourceType::Buffer)
			{
				Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(BindingResource);
				if (EnumHasAnyFlags(Buffer->GetUsage(), GpuBufferUsage::RWStructured | GpuBufferUsage::RWRaw))
				{
					SrcDescriptorRange_CbvSrvUav.FindOrAdd(BindingVisibility).Add(Buffer->UAV->GetHandle());
				}
				else if (EnumHasAnyFlags(Buffer->GetUsage(), GpuBufferUsage::Structured | GpuBufferUsage::Raw))
				{
					SrcDescriptorRange_CbvSrvUav.FindOrAdd(BindingVisibility).Add(Buffer->SRV->GetHandle());
				}
			}
			else if (BindingResource->GetType() == GpuResourceType::CombinedTextureSampler)
			{
				GpuCombinedTextureSampler* Combined = static_cast<GpuCombinedTextureSampler*>(BindingResource);
				GpuResource* Tex = Combined->GetTexture();
				if (Tex->GetType() == GpuResourceType::Texture)
				{
					Dx12Texture* DxTex = static_cast<Dx12Texture*>(Tex);
					SrcDescriptorRange_CbvSrvUav.FindOrAdd(BindingVisibility).Add(DxTex->GetDx12DefaultView()->GetSRV()->GetHandle());
				}
				else
				{
					Dx12TextureView* DxView = static_cast<Dx12TextureView*>(Tex);
					SrcDescriptorRange_CbvSrvUav.FindOrAdd(BindingVisibility).Add(DxView->GetSRV()->GetHandle());
				}
				Dx12Sampler* DxSampler = static_cast<Dx12Sampler*>(Combined->GetSampler());
				SrcDescriptorRange_Sampler.FindOrAdd(BindingVisibility).Add(DxSampler->GetCpuDescriptor()->GetHandle());
			}
		};

		for (const auto& [Slot, ResourceBindingEntry] : InDesc.Resources)
		{
			GpuResource* BindingResource = ResourceBindingEntry.Resource;
			BindingShaderStage RHIShaderStage = GetLayout()->GetDesc().Layouts[Slot].Stage;

			if (BindingResource->GetType() == GpuResourceType::Buffer)
			{
				Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(BindingResource);
				if (EnumHasAllFlags(Buffer->GetUsage(), GpuBufferUsage::Uniform))
				{
					DynamicBufferStorage.Add(Slot, Buffer);
					continue;
				}
			}

			SetBindingTable(Slot, ResourceBindingEntry, ResolveDx12ShaderVisibility(RHIShaderStage));
		}

		for (const auto& [DxVisibility, SrcRange] : SrcDescriptorRange_CbvSrvUav)
		{
			int NumDescriptors = SrcRange.Num();
			TUniquePtr<GpuDescriptorRange> GpuHeapRanges = AllocGpuCbvSrvUavRange(NumDescriptors);
			for (int Index = 0; Index < NumDescriptors; Index++)
			{
				GDevice->CopyDescriptorsSimple(1, GpuHeapRanges->GetCpuHandle(Index), SrcRange[Index], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			}
			DescriptorTableStorage_CbvSrvUav.Add(DxVisibility, MoveTemp(GpuHeapRanges));
		}

		for (const auto& [DxVisibility, SrcRange] : SrcDescriptorRange_Sampler)
		{
			int NumDescriptors = SrcRange.Num();
			TUniquePtr<GpuDescriptorRange> GpuHeapRanges = AllocGpuSamplerRange(NumDescriptors);
			for (int Index = 0; Index < NumDescriptors; Index++)
			{
				GDevice->CopyDescriptorsSimple(1, GpuHeapRanges->GetCpuHandle(Index), SrcRange[Index], D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
			}
			DescriptorTableStorage_Sampler.Add(DxVisibility, MoveTemp(GpuHeapRanges));
		}
	
	}

    void Dx12BindGroup::ApplyDrawBinding(ID3D12GraphicsCommandList* CommandList, Dx12RootSignature* RootSig)
    {
        Dx12BindGroupLayout* Layout = static_cast<Dx12BindGroupLayout*>(GetLayout());
		uint32 GroupSlot = Layout->GetGroupNumber();
        for (const auto& [Slot, LayoutBindingEntry] : Layout->GetDesc().Layouts)
        {
			if (LayoutBindingEntry.Type == BindingType::UniformBuffer)
			{
				D3D12_GPU_VIRTUAL_ADDRESS GpuAddr = GetDynamicBufferGpuAddr(Slot);
				uint32 RootParameterIndex = RootSig->GetDynamicBufferRootParameterIndex(Slot, GroupSlot);
				CommandList->SetGraphicsRootConstantBufferView(RootParameterIndex, GpuAddr);
			}
  
        }

		for (int Visibility = 0; Visibility <= D3D12_SHADER_VISIBILITY_MESH; Visibility++)
		{
			D3D12_SHADER_VISIBILITY DxVisibility = static_cast<D3D12_SHADER_VISIBILITY>(Visibility);
			if (TOptional<uint32> RootParameterIndex = RootSig->GetCbvSrvUavTableRootParameterIndex(DxVisibility, GroupSlot))
			{
				CommandList->SetGraphicsRootDescriptorTable(*RootParameterIndex, GetDescriptorTableStart_CbvSrvUav(DxVisibility));
			}

			if (TOptional<uint32> RootParameterIndex = RootSig->GetSamplerTableRootParameterIndex(DxVisibility, GroupSlot))
			{
				CommandList->SetGraphicsRootDescriptorTable(*RootParameterIndex, GetDescriptorTableStart_Sampler(DxVisibility));
			}
		}

    }

	void Dx12BindGroup::ApplyComputeBinding(ID3D12GraphicsCommandList* CommandList, Dx12RootSignature* RootSig)
	{
		Dx12BindGroupLayout* Layout = static_cast<Dx12BindGroupLayout*>(GetLayout());
		uint32 GroupSlot = Layout->GetGroupNumber();
		for (const auto& [Slot, LayoutBindingEntry] : Layout->GetDesc().Layouts)
		{
			if (LayoutBindingEntry.Type == BindingType::UniformBuffer && EnumHasAnyFlags(LayoutBindingEntry.Stage, BindingShaderStage::Compute))
			{
				D3D12_GPU_VIRTUAL_ADDRESS GpuAddr = GetDynamicBufferGpuAddr(Slot);
				uint32 RootParameterIndex = RootSig->GetDynamicBufferRootParameterIndex(Slot, GroupSlot);
				CommandList->SetComputeRootConstantBufferView(RootParameterIndex, GpuAddr);
			}
		}

		if (TOptional<uint32> RootParameterIndex = RootSig->GetCbvSrvUavTableRootParameterIndex(D3D12_SHADER_VISIBILITY_ALL, GroupSlot))
		{
			CommandList->SetComputeRootDescriptorTable(*RootParameterIndex, GetDescriptorTableStart_CbvSrvUav(D3D12_SHADER_VISIBILITY_ALL));
		}

		if (TOptional<uint32> RootParameterIndex = RootSig->GetSamplerTableRootParameterIndex(D3D12_SHADER_VISIBILITY_ALL, GroupSlot))
		{
			CommandList->SetComputeRootDescriptorTable(*RootParameterIndex, GetDescriptorTableStart_Sampler(D3D12_SHADER_VISIBILITY_ALL));
		}
	}

	Dx12RootSignature::Dx12RootSignature(const RootSignatureDesc& InDesc)
	{
		TArray<CD3DX12_ROOT_PARAMETER1> RootParameters;
		auto AddRootParameter = [&](const Dx12BindGroupLayout* Layout) {

			if (Layout != nullptr)
			{
				BindingGroupSlot CurGroupNumber = Layout->GetGroupNumber();
				//Root Descriptor
				for (const auto& [Slot, LayoutBindingEntry] : Layout->GetDesc().Layouts)
				{
					if (LayoutBindingEntry.Type == BindingType::UniformBuffer)
					{
						RootParameters.Add(Layout->GetDynamicBufferRootParameter(Slot));
						DynamicBufferToRootParameterIndex[CurGroupNumber].Add(Slot, RootParameters.Num() - 1);
					}
					
				}

				//Descriptor table
				for (int Visibility = 0; Visibility <= D3D12_SHADER_VISIBILITY_MESH; Visibility++)
				{
					D3D12_SHADER_VISIBILITY DxVisibility = static_cast<D3D12_SHADER_VISIBILITY>(Visibility);
					if (TOptional<CD3DX12_ROOT_PARAMETER1> RootParameter = Layout->GetDescriptorTableRootParameter_CbvSrvUav(DxVisibility))
					{
						RootParameters.Add(*RootParameter);
						CbvSrvUavTableToRootParameterIndex[CurGroupNumber].Add(DxVisibility, RootParameters.Num() - 1);
					}

					if (TOptional<CD3DX12_ROOT_PARAMETER1> RootParameter = Layout->GetDescriptorTableRootParameter_Sampler(DxVisibility))
					{
						RootParameters.Add(*RootParameter);
						SamplerTableToRootParameterIndex[CurGroupNumber].Add(DxVisibility, RootParameters.Num() - 1);
					}
				}
	
			}
	
		};

		AddRootParameter(InDesc.Layout0);
		AddRootParameter(InDesc.Layout1);
		AddRootParameter(InDesc.Layout2);
		AddRootParameter(InDesc.Layout3);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSignatureDesc;
		RootSignatureDesc.Init_1_1(
			(uint32)RootParameters.Num(),
			RootParameters.GetData(),
			0,
			nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
		);

		TRefCountPtr<ID3DBlob> Signature;
		TRefCountPtr<ID3DBlob> Error;
		DxCheck(D3D12SerializeVersionedRootSignature(&RootSignatureDesc, Signature.GetInitReference(), Error.GetInitReference()));
		DxCheck(GDevice->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(Resource.GetInitReference())));
	}

}
