#pragma once
#include "CommonHeader.h"
#include "GpuResourceCommon.h"

namespace FRAMEWORK
{
	class GpuBuffer : public GpuResource
	{
	public:
		virtual void* GetMappedData() = 0;
	};
}
