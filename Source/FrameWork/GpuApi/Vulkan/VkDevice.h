#pragma once
#include "VkCommon.h"

namespace FW
{
	inline VkInstance GInstance;
	inline VkDevice GDevice;
	inline VkQueue GGraphicsQueue;
	extern void InitVulkanCore();
}