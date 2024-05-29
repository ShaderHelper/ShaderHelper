#include "CommonHeader.h"
#include "Dx12RS.h"
#include "Dx12Device.h"
#include "Dx12Buffer.h"
#include "Dx12CommandRecorder.h"

namespace FRAMEWORK
{
	D3D12_DESCRIPTOR_RANGE_TYPE BindingTypeToDescriptorRangeType(BindingType InType)
	{
		switch (InType)
		{
		case BindingType::Texture:			return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		case BindingType::Sampler:			return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		default:
			check(false);
			return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
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
				CD3DX12_ROOT_PARAMETER DynamicBufferRootParameter;
				DynamicBufferRootParameter.InitAsConstantBufferView(Slot, Desc.GroupNumber, BindingVisibility);
				DynamicBufferRootParameters.Add(Slot, MoveTemp(DynamicBufferRootParameter));
			}
			else
			{
				CD3DX12_DESCRIPTOR_RANGE Range{};
				Range.RangeType = BindingTypeToDescriptorRangeType(LayoutBindingEntry.Type);
				Range.NumDescriptors = 1;
				Range.BaseShaderRegister = Slot;
				Range.RegisterSpace = LayoutDesc.GroupNumber;

				if (LayoutBindingEntry.Type == BindingType::Texture)
				{
					DescriptorTableRanges_CbvSrvUav.FindOrAdd(BindingVisibility).Add(MoveTemp(Range));
				}
				else if (LayoutBindingEntry.Type == BindingType::Sampler)
				{
					DescriptorTableRanges_Sampler.FindOrAdd(BindingVisibility).Add(MoveTemp(Range));
				}
				else
				{
					check(false);
				}
				
			}
		};

		for (const auto& [Slot, LayoutBindingEntry] : Desc.Layouts)
		{
			BindingShaderStage RHIShaderStage = LayoutBindingEntry.Stage;
			
			//we dont just set it always to D3D12_SHADER_VISIBILITY_ALL, which might produce inconsistent result on different backend and have some performance impacts
			if (EnumHasAllFlags(RHIShaderStage, BindingShaderStage::All))
			{
				SetRootParameterInfo(Slot, LayoutBindingEntry, D3D12_SHADER_VISIBILITY_ALL);
			}
			else
			{
				if (EnumHasAnyFlags(RHIShaderStage, BindingShaderStage::Vertex))
				{
					SetRootParameterInfo(Slot, LayoutBindingEntry, D3D12_SHADER_VISIBILITY_VERTEX);
				}

				if (EnumHasAnyFlags(RHIShaderStage, BindingShaderStage::Pixel))
				{
					SetRootParameterInfo(Slot, LayoutBindingEntry, D3D12_SHADER_VISIBILITY_PIXEL);
				}
			}
		}

		for (const auto& [BindingVisibility, Ranges] : DescriptorTableRanges_CbvSrvUav)
		{
			CD3DX12_ROOT_PARAMETER DescriptorTableRootParameter;
			DescriptorTableRootParameter.InitAsDescriptorTable((uint32)Ranges.Num(), Ranges.GetData(), BindingVisibility);

			DescriptorTableRootParameters_CbvSrvUav.Add(BindingVisibility, MoveTemp(DescriptorTableRootParameter));
		}

		for (const auto& [BindingVisibility, Ranges] : DescriptorTableRanges_Sampler)
		{
			CD3DX12_ROOT_PARAMETER DescriptorTableRootParameter;
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

		auto SetBindingWithVisibility = [&](BindingSlot Slot, const ResourceBinding& ResourceBindingEntry, D3D12_SHADER_VISIBILITY BindingVisibility)
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
				SrcDescriptorRange_CbvSrvUav.FindOrAdd(BindingVisibility).Add(Texture->SRV->GetHandle());
			}
		};

		for (const auto& [Slot, ResourceBindingEntry] : InDesc.Resources)
		{
			GpuResource* BindingResource = ResourceBindingEntry.Resource;
			BindingShaderStage RHIShaderStage = GetLayout()->GetDesc().Layouts[Slot].Stage;

			if (BindingResource->GetType() == GpuResourceType::Buffer)
			{
				Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(BindingResource);
				DynamicBufferStorage.Add(Slot, Buffer);
			}
			else
			{
				if (EnumHasAllFlags(RHIShaderStage, BindingShaderStage::All))
				{
					SetBindingWithVisibility(Slot, ResourceBindingEntry, D3D12_SHADER_VISIBILITY_ALL);
				}
				else
				{
					if (EnumHasAnyFlags(RHIShaderStage, BindingShaderStage::Vertex))
					{
						SetBindingWithVisibility(Slot, ResourceBindingEntry, D3D12_SHADER_VISIBILITY_VERTEX);
					}

					if (EnumHasAnyFlags(RHIShaderStage, BindingShaderStage::Pixel))
					{
						SetBindingWithVisibility(Slot, ResourceBindingEntry, D3D12_SHADER_VISIBILITY_PIXEL);
					}
				}
			}
		}

		for (const auto& [DxVisibility, SrcRange] : SrcDescriptorRange_CbvSrvUav)
		{
			uint32 NumDescriptors = SrcRange.Num();
			TUniquePtr<GpuDescriptorRange> GpuHeapRanges = AllocGpuCbvSrvUavRange(NumDescriptors);
			GDevice->CopyDescriptorsSimple(NumDescriptors, GpuHeapRanges->GetCpuHandle(), *SrcRange.GetData(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			DescriptorTableStorage_CbvSrvUav.Add(DxVisibility, MoveTemp(GpuHeapRanges));
		}

		for (const auto& [DxVisibility, SrcRange] : SrcDescriptorRange_Sampler)
		{
			uint32 NumDescriptors = SrcRange.Num();
			TUniquePtr<GpuDescriptorRange> GpuHeapRanges = AllocGpuSamplerRange(NumDescriptors);
			GDevice->CopyDescriptorsSimple(NumDescriptors, GpuHeapRanges->GetCpuHandle(), *SrcRange.GetData(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
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
			if (RootSig->GetCbvSrvUavTableToRootParameterIndex(DxVisibility, GroupSlot))
			{
				uint32 RootParameterIndex = *RootSig->GetCbvSrvUavTableToRootParameterIndex(DxVisibility, GroupSlot);
				CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex, GetDescriptorTableStart_CbvSrvUav(DxVisibility));
			}

			if (RootSig->GetSamplerTableToRootParameterIndex(DxVisibility, GroupSlot))
			{
				uint32 RootParameterIndex = *RootSig->GetSamplerTableToRootParameterIndex(DxVisibility, GroupSlot);
				CommandList->SetGraphicsRootDescriptorTable(RootParameterIndex, GetDescriptorTableStart_Sampler(DxVisibility));
			}
		}

    }

	Dx12RootSignature::Dx12RootSignature(const RootSignatureDesc& InDesc)
	{
		TArray<CD3DX12_ROOT_PARAMETER> RootParameters;
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
					if (Layout->GetDescriptorTableRootParameter_CbvSrvUav(DxVisibility))
					{
						RootParameters.Add(*Layout->GetDescriptorTableRootParameter_CbvSrvUav(DxVisibility));
						CbvSrvUavTableToRootParameterIndex[CurGroupNumber].Add(DxVisibility, RootParameters.Num() - 1);
					}

					if (Layout->GetDescriptorTableRootParameter_Sampler(DxVisibility))
					{
						RootParameters.Add(*Layout->GetDescriptorTableRootParameter_Sampler(DxVisibility));
						SamplerTableToRootParameterIndex[CurGroupNumber].Add(DxVisibility, RootParameters.Num() - 1);
					}
				}
	
			}
	
		};

		AddRootParameter(InDesc.Layout0);
		AddRootParameter(InDesc.Layout1);
		AddRootParameter(InDesc.Layout2);
		AddRootParameter(InDesc.Layout3);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSignatureDesc = { 
			(uint32)RootParameters.Num(), RootParameters.GetData(),
		};

		TRefCountPtr<ID3DBlob> Signature;
		TRefCountPtr<ID3DBlob> Error;
		DxCheck(D3D12SerializeVersionedRootSignature(&RootSignatureDesc, Signature.GetInitReference(), Error.GetInitReference()));
		DxCheck(GDevice->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(Resource.GetInitReference())));
	}

}
