#pragma once
#include "GpuResourceCommon.h"

namespace FW
{
	struct GpuSamplerDesc
	{
		SamplerFilter Filter = SamplerFilter::Point;
		SamplerAddressMode AddressU = SamplerAddressMode::Clamp;
		SamplerAddressMode AddressV = SamplerAddressMode::Clamp;
		SamplerAddressMode AddressW = SamplerAddressMode::Clamp;
		CompareMode Compare = CompareMode::Never;

		bool operator==(const GpuSamplerDesc& Other) const
		{
			return Filter == Other.Filter && AddressU == Other.AddressU && AddressV == Other.AddressV && AddressW == Other.AddressW
				&& Compare == Other.Compare;
		}
	};

	class GpuSampler : public GpuResource
	{
	public:
		GpuSampler(const GpuSamplerDesc& InDesc) : GpuResource(GpuResourceType::Sampler)
		, Desc(InDesc)
		{}

		const GpuSamplerDesc& GetDesc() const { return Desc; }

	private:
		GpuSamplerDesc Desc;
	};
}
