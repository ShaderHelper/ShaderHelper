#pragma once

namespace FRAMEWORK::GpuFeature
{
#define ValidateGpuFeature(FeatureFlag, Msg)                \
	if(!FeatureFlag) {                                      \
		FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, Msg,  \
	TEXT("Error:"));                                        \
		std::_Exit(0);                                      \
	}                                                       \
	else

	FRAMEWORK_API extern bool Support16bitType;
}