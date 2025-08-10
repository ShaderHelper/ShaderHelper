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
	};

	class GpuSampler : public GpuResource
	{
	public:
		GpuSampler() : GpuResource(GpuResourceType::Sampler)
		{}
	};
}
