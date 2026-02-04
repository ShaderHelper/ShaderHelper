#include "CommonHeader.h"
#define VOLK_IMPLEMENTATION
#include "VkDevice.h"

namespace FW
{
	void CreateInstance()
	{
		uint32_t ExtCount = 0;
		VkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &ExtCount, nullptr));

		TArray<VkExtensionProperties> AvailableExts;
		AvailableExts.SetNum(ExtCount);
		VkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &ExtCount, AvailableExts.GetData()));

		uint32_t LayerCount = 0;
		VkCheck(vkEnumerateInstanceLayerProperties(&LayerCount, nullptr));

		TArray<VkLayerProperties> AvailableLayers;
		AvailableLayers.SetNum(LayerCount);
		VkCheck(vkEnumerateInstanceLayerProperties(&LayerCount, AvailableLayers.GetData()));

		auto CheckExtSupported = [&AvailableExts](const char* ExtName) {
			for (const auto& Ext : AvailableExts)
			{
				if (FCStringAnsi::Strcmp(Ext.extensionName, ExtName) == 0)
				{
					return;
				}
			}
			FString Message = FString::Printf(TEXT("Required Vulkan extension %hs is not supported."), ExtName);
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *Message, TEXT("Error:"));
			std::_Exit(0);
		};

		auto CheckLayerSupported = [&AvailableLayers](const char* LayerName) {
			for (const auto& Layer : AvailableLayers)
			{
				if (FCStringAnsi::Strcmp(Layer.layerName, LayerName) == 0)
				{
					return;
				}
			}
			FString Message = FString::Printf(TEXT("Required Vulkan layer %hs is not supported."), LayerName);
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, *Message, TEXT("Error:"));
			std::_Exit(0);
		};

		TArray<const char*> Exts, Layers;
#if GPU_API_DEBUG
		bool bEnableValidationLayer = FParse::Param(FCommandLine::Get(), TEXT("VulkanValidation"));
		if (bEnableValidationLayer)
		{
			Exts.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			Layers.Add("VK_LAYER_KHRONOS_validation");
		}
		for (const char* ExtName : Exts)
		{
			CheckExtSupported(ExtName);
		}
		for (const char* LayerName : Layers)
		{
			CheckLayerSupported(LayerName);
		}
#endif

		VkApplicationInfo AppInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "ShaderHelper",
			.apiVersion = VK_API_VERSION_1_3
		};

		VkInstanceCreateInfo InstanceInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &AppInfo,
			.enabledLayerCount = (uint32_t)Layers.Num(),
			.ppEnabledLayerNames = Layers.GetData(),
			.enabledExtensionCount = (uint32_t)Exts.Num(),
			.ppEnabledExtensionNames = Exts.GetData(),
		};

		VkCheck(vkCreateInstance(&InstanceInfo, nullptr, &GInstance));
	}

	void CreateDevice()
	{
		uint32_t DeviceCount = 0;
		VkCheck(vkEnumeratePhysicalDevices(GInstance, &DeviceCount, nullptr));

		TArray<VkPhysicalDevice> PhysicalDevices;
		PhysicalDevices.SetNum(DeviceCount);
		VkCheck(vkEnumeratePhysicalDevices(GInstance, &DeviceCount, PhysicalDevices.GetData()));

		TArray<const char*> DeviceExts;
		auto GetGraphicsQueueIndex = [](VkPhysicalDevice InDevice) -> int32
		{
			uint32_t QueueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(InDevice, &QueueFamilyCount, nullptr);
			TArray<VkQueueFamilyProperties> QueueFamilies;
			QueueFamilies.SetNum(QueueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(InDevice, &QueueFamilyCount, QueueFamilies.GetData());
			for (int32 Index = 0; Index < QueueFamilies.Num(); Index++)
			{
				if (QueueFamilies[Index].queueCount > 0 && QueueFamilies[Index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					return Index;
				}
			}
			return -1;
		};

		auto CheckPhysicalDevice = [&](VkPhysicalDevice InDevice) -> bool
		{
			uint32_t DeviceExtCount = 0;
			VkCheck(vkEnumerateDeviceExtensionProperties(InDevice, nullptr, &DeviceExtCount, nullptr));
			TArray<VkExtensionProperties> AvailableDeviceExts;
			AvailableDeviceExts.SetNum(DeviceExtCount);
			VkCheck(vkEnumerateDeviceExtensionProperties(InDevice, nullptr, &DeviceExtCount, AvailableDeviceExts.GetData()));

			for (const char* ExtName : DeviceExts)
			{
				for (const auto& AvailableExt : AvailableDeviceExts)
				{
					if (FCStringAnsi::Strcmp(AvailableExt.extensionName, ExtName) != 0)
					{
						return false;
					}
				}
			}

			int32 QueueIndex = GetGraphicsQueueIndex(InDevice);
			if (QueueIndex == -1)
			{
				return false;
			}

			return true;
		};

		auto GetDeviceTypePriority = [](VkPhysicalDeviceType Type) -> int32 {
			switch (Type)
			{
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return 0;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return 1;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return 2;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:            return 3;
			default:                                     return 4;
			}
		};

		TArray<VkPhysicalDevice> SuitableDevices;
		for (auto Device : PhysicalDevices)
		{
			if (CheckPhysicalDevice(Device))
			{
				SuitableDevices.Add(Device);
			}
		}

		Algo::Sort(SuitableDevices, [&](VkPhysicalDevice A, VkPhysicalDevice B) {
			VkPhysicalDeviceProperties2 PropsA{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
			VkPhysicalDeviceProperties2 PropsB{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
			vkGetPhysicalDeviceProperties2(A, &PropsA);
			vkGetPhysicalDeviceProperties2(B, &PropsB);
			return GetDeviceTypePriority(PropsA.properties.deviceType) < GetDeviceTypePriority(PropsB.properties.deviceType);
		});

		VkPhysicalDevice TargetDevice = VK_NULL_HANDLE;
		int32 TargetGraphicsQueueIndex = -1;
		if (SuitableDevices.Num() > 0)
		{
			TargetDevice = SuitableDevices[0];
			TargetGraphicsQueueIndex = GetGraphicsQueueIndex(TargetDevice);
		}
		if (TargetDevice == VK_NULL_HANDLE)
		{
			FPlatformMisc::MessageBoxExt(EAppMsgType::Ok, TEXT("Could not find the suitable physical device!"), TEXT("Error:"));
			std::_Exit(0);
		}

		TArray<float> QueuePriorities{ 1.0f };
		VkDeviceQueueCreateInfo QueueInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = (uint32_t)TargetGraphicsQueueIndex,
			.queueCount = 1,
			.pQueuePriorities = QueuePriorities.GetData()
		};

		VkPhysicalDeviceFeatures2 Features{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
		};	
		vkGetPhysicalDeviceFeatures2(TargetDevice, &Features);

		VkDeviceCreateInfo DeviceInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &Features,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &QueueInfo,
			.enabledExtensionCount = (uint32_t)DeviceExts.Num(),
			.ppEnabledExtensionNames = DeviceExts.GetData(),
		};

		VkCheck(vkCreateDevice(TargetDevice, &DeviceInfo, nullptr, &GDevice));
		vkGetDeviceQueue(GDevice, TargetGraphicsQueueIndex, 0, &GGraphicsQueue);
	}

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

		CreateInstance();
		volkLoadInstance(GInstance);

		CreateDevice();
		volkLoadDevice(GDevice);
	}
}

