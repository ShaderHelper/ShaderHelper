#pragma once
#include "GpuResourceCommon.h"

namespace FRAMEWORK
{
	struct GpuSamplerDesc
	{
		SamplerFilter Filer = SamplerFilter::Point;
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