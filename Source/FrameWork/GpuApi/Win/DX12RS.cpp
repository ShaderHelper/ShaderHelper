#include "CommonHeader.h"
#include "Dx12RS.h"
#include "Dx12Device.h"
#include "Dx12Buffer.h"
#include "Dx12CommandList.h"

namespace FRAMEWORK
{
	D3D12_DESCRIPTOR_RANGE_TYPE BindingTypeToDescriptorRangeType(BindingType InType)
	{
		switch (InType)
		{
		case FRAMEWORK::BindingType::UniformBuffer:	return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		default:
			check(false);
			return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		}
	}

	D3D12_SHADER_VISIBILITY MapShaderVisibility(BindingShaderStage InStage)
	{
		switch (InStage)
		{
		case FRAMEWORK::BindingShaderStage::Vertex:	return D3D12_SHADER_VISIBILITY_VERTEX;
		case FRAMEWORK::BindingShaderStage::Pixel:	return D3D12_SHADER_VISIBILITY_PIXEL;
		default:
			return D3D12_SHADER_VISIBILITY_ALL;
		}
	}


	Dx12BindGroupLayout::Dx12BindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc)
		: GpuBindGroupLayout(LayoutDesc)
	{
		for (const auto& BindingLayoutEntry : Desc.Layouts)
		{
			BindingSlots.Add(BindingLayoutEntry.Slot);
			//For the moment, all uniformbuffers in the layout are bound via root descriptor.
			if (BindingLayoutEntry.Type == BindingType::UniformBuffer)
			{
				CD3DX12_ROOT_PARAMETER1 DynamicBufferRootParameter;
				DynamicBufferRootParameter.InitAsConstantBufferView(BindingLayoutEntry.Slot, Desc.GroupNumber,
					D3D12_ROOT_DESCRIPTOR_FLAG_NONE, MapShaderVisibility(BindingLayoutEntry.Stage));
				DynamicBufferRootParameters.Add(BindingLayoutEntry.Slot, MoveTemp(DynamicBufferRootParameter));
			}
			else
			{

			}
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
		for (const auto& BindingEntry : InDesc.Resources)
		{
			BindingSlot Slot = BindingEntry.Slot;
			GpuResource* BindingResource = BindingEntry.Resource;

			if (BindingResource->GetType() == GpuResourceType::Buffer)
			{
				Dx12Buffer* Buffer = static_cast<Dx12Buffer*>(BindingResource);
				DynamicBufferStorage.Add(Slot, Buffer->GetAllocation().GetGpuAddr());
			}
			else
			{

			}
		}
	}

    void Dx12BindGroup::Apply(ID3D12GraphicsCommandList* CommandList, Dx12RootSignature* RootSig)
    {
        Dx12BindGroupLayout* Layout = static_cast<Dx12BindGroupLayout*>(GetLayout());
        for (auto Slot : Layout->GetBindingSlots())
        {
            D3D12_GPU_VIRTUAL_ADDRESS GpuAddr = GetDynamicBufferGpuAddr(Slot);
            uint32 RootParameterIndex = RootSig->GetDynamicBufferRootParameterIndex(Slot, Layout->GetGroupNumber());
            CommandList->SetGraphicsRootConstantBufferView(RootParameterIndex, GpuAddr);
        }
    }

	Dx12BindGroup::~Dx12BindGroup()
	{

	}

	Dx12RootSignature::Dx12RootSignature(const RootSignatureDesc& InDesc)
	{
		TArray<CD3DX12_ROOT_PARAMETER1> RootParameters;
		auto AddRootParameter = [&](const Dx12BindGroupLayout* Layout) {

			if (Layout != nullptr)
			{
				const TArray<BindingSlot>& Slots = Layout->GetBindingSlots();
				BindingGroupSlot CurGroupNumber = Layout->GetGroupNumber();
				for (auto Slot : Slots)
				{
					RootParameters.Add(Layout->GetDynamicBufferRootParameter(Slot));
					DynamicBufferToRootParameterIndex[CurGroupNumber].Add(Slot, RootParameters.Num() - 1);
				}

			}
	
		};

		AddRootParameter(InDesc.Layout0);
		AddRootParameter(InDesc.Layout1);
		AddRootParameter(InDesc.Layout2);
		AddRootParameter(InDesc.Layout3);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC RootSignatureDesc = { (uint32)RootParameters.Num(), RootParameters.GetData() };
		TRefCountPtr<ID3DBlob> Signature;
		TRefCountPtr<ID3DBlob> Error;
		DxCheck(D3D12SerializeVersionedRootSignature(&RootSignatureDesc, Signature.GetInitReference(), Error.GetInitReference()));
		DxCheck(GDevice->CreateRootSignature(0, Signature->GetBufferPointer(), Signature->GetBufferSize(), IID_PPV_ARGS(Resource.GetInitReference())));
	}

}
