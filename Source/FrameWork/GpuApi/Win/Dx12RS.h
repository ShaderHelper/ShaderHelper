#pragma once
#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"
#include "Dx12Descriptor.h"

namespace FRAMEWORK
{
	class Dx12BindGroupLayout : public GpuBindGroupLayout
	{
	public:
		Dx12BindGroupLayout(GpuBindGroupLayoutDesc LayoutDesc);

		const CD3DX12_ROOT_PARAMETER1& GetDynamicBufferRootParameter(BindingSlot InSlot) const {
			return DynamicBufferRootParameters[InSlot];
		}

		const CD3DX12_ROOT_PARAMETER1& GetDescriptorTableRootParameter(BindingShaderStage InStage) const {
			return DescriptorTableRootParameters_CbvSrvUav[InStage];
		}

		const GpuBindGroupLayoutDesc& GetDesc() const {
			return Desc;
		}

		const TArray<BindingSlot>& GetBindingSlots() const {
			return BindingSlots;
		}

	private:
		GpuBindGroupLayoutDesc Desc;
		TMap<BindingShaderStage, CD3DX12_ROOT_PARAMETER1> DescriptorTableRootParameters_CbvSrvUav;
		TMap<BindingSlot, CD3DX12_ROOT_PARAMETER1> DynamicBufferRootParameters;
		TArray<BindingSlot> BindingSlots;
	};

	class Dx12BindGroup : public GpuBindGroup
	{
	public:
		Dx12BindGroup(const GpuBindGroupDesc& InDesc);
		~Dx12BindGroup();

		D3D12_GPU_VIRTUAL_ADDRESS GetDynamicBufferGpuAddr(BindingSlot InSlot) const {
			return DynamicBufferStorage[InSlot];
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetBaseDescriptor(BindingShaderStage InStage) const {
			return DescriptorTableStorage[InStage];
		}

	private:
		TMap<BindingShaderStage, D3D12_GPU_DESCRIPTOR_HANDLE> DescriptorTableStorage;
		TMap<BindingSlot, D3D12_GPU_VIRTUAL_ADDRESS> DynamicBufferStorage;
	};

	struct RootSignatureDesc
	{
		Dx12BindGroupLayout* Layout0;
		Dx12BindGroupLayout* Layout1;
		Dx12BindGroupLayout* Layout2;
		Dx12BindGroupLayout* Layout3;

		bool operator==(const RootSignatureDesc& Other) const {
			return Layout0->GetDesc() == Other.Layout0->GetDesc() && Layout1->GetDesc() == Other.Layout1->GetDesc() &&
				Layout2->GetDesc() == Other.Layout2->GetDesc() && Layout3->GetDesc() == Other.Layout3->GetDesc();
		}

		friend uint32 GetTypeHash(const RootSignatureDesc& Key) {
			uint32 Hash = GetTypeHash(Key.Layout0->GetDesc());
			Hash = HashCombine(Hash, GetTypeHash(Key.Layout1->GetDesc()));
			Hash = HashCombine(Hash, GetTypeHash(Key.Layout2->GetDesc()));
			Hash = HashCombine(Hash, GetTypeHash(Key.Layout3->GetDesc()));
			return Hash;
		}
	};


	class Dx12RootSignature
	{
	public:
		Dx12RootSignature(const RootSignatureDesc& InDesc);
		ID3D12RootSignature* GetResource() const { return Resource; }

		uint32 GetDynamicBufferRootParameterIndex(BindingSlot InSlot) const {
			return DynamicBufferToRootParameterIndex[InSlot];
		}

		uint32 GetDescriptorTableRootParameterIndex(BindingShaderStage InStage) const {
			return DescriptorTableToRootParameterIndex[InStage];
		}

	private:
		TMap<BindingSlot, uint32> DynamicBufferToRootParameterIndex;
		TMap<BindingShaderStage, uint32> DescriptorTableToRootParameterIndex;
		TRefCountPtr<ID3D12RootSignature> Resource;
	};

	class Dx12RootSignatureManager
	{
		Dx12RootSignatureManager() = default;
	public:
		static Dx12RootSignature* GetRootSignature(const RootSignatureDesc& InDesc);

	private:
		TMap<RootSignatureDesc, Dx12RootSignature*> RootSignatureCache;
	};

	TRefCountPtr<Dx12BindGroupLayout> CreateDx12BindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc);
	TRefCountPtr<Dx12BindGroup> CreateDx12BindGroup(const GpuBindGroupDesc& Desc);
}
