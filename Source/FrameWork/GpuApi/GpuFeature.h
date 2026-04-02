#pragma once

#include "GpuResourceCommon.h"

#define ValidateGpuFeature(FeatureFlag, Msg)                 \
	if(!(FeatureFlag)) {                                     \
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, Msg,   \
	TEXT("Error:"));                                         \
		std::_Exit(0);                                       \
	}                                                        \
	else

namespace FW
{
	class GpuFeature
	{
	public:
		virtual ~GpuFeature() = default;

	public:
		virtual bool Support16bitType() const = 0;
		virtual bool SupportTimestampQuery() const = 0;
		virtual uint32 GetMaxSampleCount(GpuFormat Format) const = 0;
	};
}
