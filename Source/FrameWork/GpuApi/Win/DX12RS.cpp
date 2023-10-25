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
		case FRAMEWORK::BindingShaderStage::All:	return D3D12_SHADER_VISIBILITY_ALL;
		default:
			check(false);
			return D3D12_SHADER_VISIBILITY_ALL;
		}
	}


	Dx12BindGroupLayout::Dx12BindGroupLayout(GpuBindGroupLayoutDesc LayoutDesc)
		: Desc(MoveTemp(LayoutDesc))
	{
		for (const auto& BindingLayoutInfo : Desc.Layouts)
		{
			BindingSlots.Add(BindingLayoutInfo.Slot);
			//For the moment, all uniformbuffers in the layout are bound via root descriptor.
			if (BindingLayoutInfo.Type == BindingType::UniformBuffer)
			{
				CD3DX12_ROOT_PARAMETER1 DynamicBufferRootParameter;
				DynamicBufferRootParameter.InitAsConstantBufferView(BindingLayoutInfo.Slot, Desc.GroupNumber,
					D3D12_ROOT_DESCRIPTOR_FLAG_NONE, MapShaderVisibility(BindingLayoutInfo.Stage));
				DynamicBufferRootParameters.Add(BindingLayoutInfo.Slot, MoveTemp(DynamicBufferRootParameter));
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
		: GpuBindGroup(InDesc.Layout)
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
				for (auto Slot : Slots)
				{
					RootParameters.Add(Layout->GetDynamicBufferRootParameter(Slot));
					DynamicBufferToRootParameterIndex.Add(Slot, RootParameters.Num() - 1);
				}

				for (int32 i = 0; i < (int32)BindingShaderStage::Num; i++)
				{

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
