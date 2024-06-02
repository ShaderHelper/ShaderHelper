#pragma once
#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"
#include "Dx12Descriptor.h"
#include "Dx12Buffer.h"

namespace FRAMEWORK
{
	class Dx12BindGroupLayout : public GpuBindGroupLayout
	{
	public:
		Dx12BindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc);

		const CD3DX12_ROOT_PARAMETER& GetDynamicBufferRootParameter(BindingSlot InSlot) const {
			return DynamicBufferRootParameters[InSlot];
		}

		TOptional<CD3DX12_ROOT_PARAMETER> GetDescriptorTableRootParameter_CbvSrvUav(D3D12_SHADER_VISIBILITY Visibility) const {
			return DescriptorTableRootParameters_CbvSrvUav.Contains(Visibility) ? DescriptorTableRootParameters_CbvSrvUav[Visibility] : TOptional<CD3DX12_ROOT_PARAMETER>();
		}

		TOptional<CD3DX12_ROOT_PARAMETER> GetDescriptorTableRootParameter_Sampler(D3D12_SHADER_VISIBILITY Visibility) const {
			return DescriptorTableRootParameters_Sampler.Contains(Visibility) ? DescriptorTableRootParameters_Sampler[Visibility] : TOptional<CD3DX12_ROOT_PARAMETER>();
		}

	private:
		TMap<D3D12_SHADER_VISIBILITY, TArray<CD3DX12_DESCRIPTOR_RANGE>> DescriptorTableRanges_CbvSrvUav;
		TMap<D3D12_SHADER_VISIBILITY, TArray<CD3DX12_DESCRIPTOR_RANGE>> DescriptorTableRanges_Sampler;
        TMap<D3D12_SHADER_VISIBILITY, CD3DX12_ROOT_PARAMETER> DescriptorTableRootParameters_CbvSrvUav;
		TMap<D3D12_SHADER_VISIBILITY, CD3DX12_ROOT_PARAMETER> DescriptorTableRootParameters_Sampler;
		TMap<BindingSlot, CD3DX12_ROOT_PARAMETER> DynamicBufferRootParameters;
	};

	class Dx12BindGroup : public GpuBindGroup, public Dx12DeferredDeleteObject<Dx12BindGroup>
	{
	public:
		Dx12BindGroup(const GpuBindGroupDesc& InDesc);

		D3D12_GPU_VIRTUAL_ADDRESS GetDynamicBufferGpuAddr(BindingSlot InSlot) const {
			return DynamicBufferStorage[InSlot]->GetAllocation().GetGpuAddr();
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptorTableStart_CbvSrvUav(D3D12_SHADER_VISIBILITY Visibility) const {
			return DescriptorTableStorage_CbvSrvUav[Visibility]->GetGpuHandle();
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptorTableStart_Sampler(D3D12_SHADER_VISIBILITY Visibility) const {
			return DescriptorTableStorage_Sampler[Visibility]->GetGpuHandle();
		}
        
        void ApplyDrawBinding(ID3D12GraphicsCommandList* CommandList, class Dx12RootSignature* RootSig);
		//void ApplyComputeBinding();

	private:
		TMap<D3D12_SHADER_VISIBILITY, TUniquePtr<GpuDescriptorRange>> DescriptorTableStorage_CbvSrvUav;
		TMap<D3D12_SHADER_VISIBILITY, TUniquePtr<GpuDescriptorRange>> DescriptorTableStorage_Sampler;
		TMap<BindingSlot, Dx12Buffer*> DynamicBufferStorage;
	};

	struct RootSignatureDesc
	{
		RootSignatureDesc(Dx12BindGroupLayout* InLayout0, Dx12BindGroupLayout* InLayout1,
			Dx12BindGroupLayout* InLayout2, Dx12BindGroupLayout* InLayout3)
			: Layout0(InLayout0), Layout1(InLayout1), Layout2(InLayout2), Layout3(InLayout3)
		{
			if (Layout0) { LayoutDesc0 = Layout0->GetDesc(); }
			if (Layout1) { LayoutDesc1 = Layout1->GetDesc(); }
			if (Layout2) { LayoutDesc2 = Layout2->GetDesc(); }
			if (Layout3) { LayoutDesc3 = Layout3->GetDesc(); }
		}

		Dx12BindGroupLayout* Layout0;
		Dx12BindGroupLayout* Layout1;
		Dx12BindGroupLayout* Layout2;
		Dx12BindGroupLayout* Layout3;
		TOptional<GpuBindGroupLayoutDesc> LayoutDesc0;
		TOptional<GpuBindGroupLayoutDesc> LayoutDesc1;
		TOptional<GpuBindGroupLayoutDesc> LayoutDesc2;
		TOptional<GpuBindGroupLayoutDesc> LayoutDesc3;

		bool operator==(const RootSignatureDesc& Other) const
		{
			return LayoutDesc0 == Other.LayoutDesc0 && LayoutDesc1 == Other.LayoutDesc1 &&
				LayoutDesc2 == Other.LayoutDesc2 && LayoutDesc3 == Other.LayoutDesc3;
		}

		friend uint32 GetTypeHash(const RootSignatureDesc& Key) {
			uint32 Hash = 0;
			if (Key.LayoutDesc0) { Hash = HashCombine(Hash, GetTypeHash(*Key.LayoutDesc0)); }
			if (Key.LayoutDesc1) { Hash = HashCombine(Hash, GetTypeHash(*Key.LayoutDesc1)); }
			if (Key.LayoutDesc2) { Hash = HashCombine(Hash, GetTypeHash(*Key.LayoutDesc2)); }
			if (Key.LayoutDesc3) { Hash = HashCombine(Hash, GetTypeHash(*Key.LayoutDesc3)); }
			return Hash;
		}
	};


	class Dx12RootSignature
	{
	public:
		Dx12RootSignature(const RootSignatureDesc& InDesc);
		ID3D12RootSignature* GetResource() const { return Resource; }

		uint32 GetDynamicBufferRootParameterIndex(BindingSlot InSlot, BindingGroupSlot InGroupSlot) const {
			return DynamicBufferToRootParameterIndex[InGroupSlot][InSlot];
		}

		TOptional<uint32> GetCbvSrvUavTableToRootParameterIndex(D3D12_SHADER_VISIBILITY Visibility, BindingGroupSlot InGroupSlot) const {
			return CbvSrvUavTableToRootParameterIndex[InGroupSlot].Contains(Visibility) ?
				CbvSrvUavTableToRootParameterIndex[InGroupSlot][Visibility] : TOptional<uint32>();
		}

		TOptional<uint32> GetSamplerTableToRootParameterIndex(D3D12_SHADER_VISIBILITY Visibility, BindingGroupSlot InGroupSlot) const {
			return SamplerTableToRootParameterIndex[InGroupSlot].Contains(Visibility) ?
				SamplerTableToRootParameterIndex[InGroupSlot][Visibility] : TOptional<uint32>();
		}

	private:
		TMap<BindingSlot, uint32> DynamicBufferToRootParameterIndex[GpuResourceLimit::MaxBindableBingGroupNum];
		TMap<D3D12_SHADER_VISIBILITY, uint32> CbvSrvUavTableToRootParameterIndex[GpuResourceLimit::MaxBindableBingGroupNum];
		TMap<D3D12_SHADER_VISIBILITY, uint32> SamplerTableToRootParameterIndex[GpuResourceLimit::MaxBindableBingGroupNum];
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
