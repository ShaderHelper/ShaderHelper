#include "CommonHeader.h"
#define VOLK_IMPLEMENTATION
#include "VkDevice.h"

namespace FW
{
	void InitVulkanCore()
	{
		if (volkInitialize() != VK_SUCCESS)
		{
			FPlatformMisc::MessageBoxExt(
				EAppMsgType::Ok,
				TEXT("Current device does not support Vulkan. Please try updating your graphics driver."),
				TEXT("Error:"));
			std::_Exit(0);
		}

		uint32_t ExtCount = 0;
		VkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &ExtCount, nullptr));

		TArray<VkExtensionProperties> AvailableExts;
		AvailableExts.SetNum(ExtCount);
		VkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &ExtCount, AvailableExts.GetData()));
	}
}

