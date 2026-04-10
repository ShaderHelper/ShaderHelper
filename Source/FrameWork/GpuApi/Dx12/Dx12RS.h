#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"
#include "Dx12Descriptor.h"
#include "Dx12Buffer.h"

namespace FW
{
	class Dx12BindGroupLayout : public GpuBindGroupLayout
	{
	public:
		Dx12BindGroupLayout(const GpuBindGroupLayoutDesc& LayoutDesc);

		const CD3DX12_ROOT_PARAMETER1& GetDynamicBufferRootParameter(BindingSlot InSlot) const {
			return DynamicBufferRootParameters[InSlot];
		}

		TOptional<CD3DX12_ROOT_PARAMETER1> GetDescriptorTableRootParameter_CbvSrvUav(D3D12_SHADER_VISIBILITY Visibility) const {
			return DescriptorTableRootParameters_CbvSrvUav.Contains(Visibility) ? DescriptorTableRootParameters_CbvSrvUav[Visibility] : TOptional<CD3DX12_ROOT_PARAMETER1>();
		}

		TOptional<CD3DX12_ROOT_PARAMETER1> GetDescriptorTableRootParameter_Sampler(D3D12_SHADER_VISIBILITY Visibility) const {
			return DescriptorTableRootParameters_Sampler.Contains(Visibility) ? DescriptorTableRootParameters_Sampler[Visibility] : TOptional<CD3DX12_ROOT_PARAMETER1>();
		}

	private:
		TMap<D3D12_SHADER_VISIBILITY, TArray<CD3DX12_DESCRIPTOR_RANGE1>> DescriptorTableRanges_CbvSrvUav;
		TMap<D3D12_SHADER_VISIBILITY, TArray<CD3DX12_DESCRIPTOR_RANGE1>> DescriptorTableRanges_Sampler;
		TMap<D3D12_SHADER_VISIBILITY, CD3DX12_ROOT_PARAMETER1> DescriptorTableRootParameters_CbvSrvUav;
		TMap<D3D12_SHADER_VISIBILITY, CD3DX12_ROOT_PARAMETER1> DescriptorTableRootParameters_Sampler;
		TMap<BindingSlot, CD3DX12_ROOT_PARAMETER1> DynamicBufferRootParameters;
	};

	class Dx12BindGroup : public GpuBindGroup, public Dx12DeferredDeleteObject<Dx12BindGroup>
	{
	public:
		Dx12BindGroup(const GpuBindGroupDesc& InDesc);

		D3D12_GPU_VIRTUAL_ADDRESS GetDynamicBufferGpuAddr(BindingSlot InSlot) const {
			return DynamicBufferStorage[InSlot]->GetAllocation().GetGpuAddr();
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptorTableStart_CbvSrvUav(D3D12_SHADER_VISIBILITY Visibility) const {
			return DescriptorTableStorage_CbvSrvUav[Visibility]->GetGpuHandle(0);
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptorTableStart_Sampler(D3D12_SHADER_VISIBILITY Visibility) const {
			return DescriptorTableStorage_Sampler[Visibility]->GetGpuHandle(0);
		}
        
        void ApplyDrawBinding(ID3D12GraphicsCommandList* CommandList, class Dx12RootSignature* RootSig);
		void ApplyComputeBinding(ID3D12GraphicsCommandList* CommandList, Dx12RootSignature* RootSig);

	private:
		TMap<D3D12_SHADER_VISIBILITY, TUniquePtr<GpuDescriptorRange>> DescriptorTableStorage_CbvSrvUav;
		TMap<D3D12_SHADER_VISIBILITY, TUniquePtr<GpuDescriptorRange>> DescriptorTableStorage_Sampler;
		TMap<BindingSlot, Dx12Buffer*> DynamicBufferStorage;
	};

	struct RootSignatureDesc
	{
		RootSignatureDesc() = default;
		RootSignatureDesc(const TArray<GpuBindGroupLayout*>& InBindGroupLayouts)
		{
			for (GpuBindGroupLayout* InLayout : InBindGroupLayouts)
			{
				if (!InLayout) continue;
				Layouts.Add(static_cast<Dx12BindGroupLayout*>(InLayout));
				LayoutDescs.Add({InLayout->GetGroupNumber(), InLayout->GetDesc()});
			}
		}

		TArray<Dx12BindGroupLayout*> Layouts;
		TArray<TPair<BindingGroupSlot, GpuBindGroupLayoutDesc>> LayoutDescs;

		bool operator==(const RootSignatureDesc& Other) const
		{
			return LayoutDescs == Other.LayoutDescs;
		}

		friend uint32 GetTypeHash(const RootSignatureDesc& Key) {
			uint32 Hash = 0;
			for (const auto& [GroupSlot, Desc] : Key.LayoutDescs)
			{
				Hash = HashCombine(Hash, ::GetTypeHash(GroupSlot));
				Hash = HashCombine(Hash, GetTypeHash(Desc));
			}
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

		TOptional<uint32> GetCbvSrvUavTableRootParameterIndex(D3D12_SHADER_VISIBILITY Visibility, BindingGroupSlot InGroupSlot) const {
			return CbvSrvUavTableToRootParameterIndex[InGroupSlot].Contains(Visibility) ?
				CbvSrvUavTableToRootParameterIndex[InGroupSlot][Visibility] : TOptional<uint32>();
		}

		TOptional<uint32> GetSamplerTableRootParameterIndex(D3D12_SHADER_VISIBILITY Visibility, BindingGroupSlot InGroupSlot) const {
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
