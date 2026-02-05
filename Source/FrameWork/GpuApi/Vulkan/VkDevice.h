#pragma once
#include "VkCommon.h"

namespace FW
{
	inline VkInstance GInstance;
	inline VkDevice GDevice;
	inline VkPhysicalDevice GPhysicalDevice = VK_NULL_HANDLE;
	inline VkQueue GGraphicsQueue;
	inline uint32_t GraphicsQueueIndex;
	inline VmaAllocator GAllocator;
	extern void InitVulkanCore();
}