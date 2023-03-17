#pragma once
namespace FRAMEWORK
{
namespace GpuApi
{
	FRAMEWORK_API void InitApiEnv(); 

	//Wait for all gpu work to finish
	FRAMEWORK_API void FlushGpu();
}
}
