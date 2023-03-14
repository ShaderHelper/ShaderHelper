#pragma once
#include "CommonHeader.h"
namespace FRAMEWORK
{
	class GpuResource : public FThreadSafeRefCountedObject {};
	
	class GpuTextureResource : public GpuResource {};
}