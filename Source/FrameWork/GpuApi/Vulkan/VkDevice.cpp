#include "CommonHeader.h"
#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION
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
			FString Message = FString::Printf(TEXT("Required Vulkan instance extension %hs is not supported."), ExtName);
			throw std::runtime_error(TCHAR_TO_UTF8(*Message));
		};

		auto CheckLayerSupported = [&AvailableLayers](const char* LayerName) {
			for (const auto& Layer : AvailableLayers)
			{
				if (FCStringAnsi::Strcmp(Layer.layerName, LayerName) == 0)
				{
					return;
				}
			}
			FString Message = FString::Printf(TEXT("Required Vulkan instance layer %hs is not supported."), LayerName);
			throw std::runtime_error(TCHAR_TO_UTF8(*Message));
		};

		TArray<const char*> Exts, Layers;
#if GPU_API_DEBUG
		bool bEnableValidationLayer = FParse::Param(FCommandLine::Get(), TEXT("VulkanValidation"));
		if (bEnableValidationLayer)
		{
			Layers.Add("VK_LAYER_KHRONOS_validation");
		}
		Exts.Add(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
			.apiVersion = VK_API_VERSION_1_1
		};

		VkInstanceCreateInfo InstanceInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &AppInfo,
			.enabledLayerCount = (uint32_t)Layers.Num(),
			.ppEnabledLayerNames = Layers.GetData(),
			.enabledExtensionCount = (uint32_t)Exts.Num(),
			.ppEnabledExtensionNames = Exts.GetData(),
		};
		VkResult Result = vkCreateInstance(&InstanceInfo, nullptr, &GInstance);
		if (Result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create instance, could not find a compatible Vulkan driver.");
		}
	}

	void CreateDevice()
	{
		uint32_t DeviceCount = 0;
		VkCheck(vkEnumeratePhysicalDevices(GInstance, &DeviceCount, nullptr));

		TArray<VkPhysicalDevice> PhysicalDevices;
		PhysicalDevices.SetNum(DeviceCount);
		VkCheck(vkEnumeratePhysicalDevices(GInstance, &DeviceCount, PhysicalDevices.GetData()));

		TArray<const char*> DeviceExts;
#if PLATFORM_WINDOWS
		DeviceExts.Add(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
#endif
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
				bool bFound = false;
				for (const auto& AvailableExt : AvailableDeviceExts)
				{
					if (FCStringAnsi::Strcmp(AvailableExt.extensionName, ExtName) == 0)
					{
						bFound = true;
						break;
					}
				}
				if (!bFound)
				{
					return false;
				}
			}

			int32 QueueIndex = GetGraphicsQueueIndex(InDevice);
			if (QueueIndex == -1)
			{
				return false;
			}

			return true;
		};

		if (CheckPhysicalDevice(PhysicalDevices[0]))
		{
			GPhysicalDevice = PhysicalDevices[0];
			GraphicsQueueIndex = GetGraphicsQueueIndex(GPhysicalDevice);

			VkPhysicalDeviceProperties2 Props{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
			vkGetPhysicalDeviceProperties2(GPhysicalDevice, &Props);
			SH_LOG(LogVulkan, Display, TEXT("Physical Device: %hs (Type: %hs, API: %u.%u.%u)"),
				Props.properties.deviceName,
				magic_enum::enum_name(Props.properties.deviceType).data(),
				VK_VERSION_MAJOR(Props.properties.apiVersion),
				VK_VERSION_MINOR(Props.properties.apiVersion),
				VK_VERSION_PATCH(Props.properties.apiVersion));
		}
		if (GPhysicalDevice == VK_NULL_HANDLE)
		{
			uint32_t DeviceExtCount = 0;
			vkEnumerateDeviceExtensionProperties(PhysicalDevices[0], nullptr, &DeviceExtCount, nullptr);
			TArray<VkExtensionProperties> AvailableDeviceExts;
			AvailableDeviceExts.SetNum(DeviceExtCount);
			vkEnumerateDeviceExtensionProperties(PhysicalDevices[0], nullptr, &DeviceExtCount, AvailableDeviceExts.GetData());

			FString MissingExts;
			for (const char* ExtName : DeviceExts)
			{
				bool bSupported = false;
				for (const auto& AvailableExt : AvailableDeviceExts)
				{
					if (FCStringAnsi::Strcmp(AvailableExt.extensionName, ExtName) == 0)
					{
						bSupported = true;
						break;
					}
				}
				if (!bSupported)
				{
					if (!MissingExts.IsEmpty()) MissingExts += TEXT(", ");
					MissingExts += UTF8_TO_TCHAR(ExtName);
				}
			}
			if (!MissingExts.IsEmpty())
			{
				FString Message = FString::Printf(TEXT("The preferred physical device does not support required device extension(s) [%s]."), *MissingExts);
				throw std::runtime_error(TCHAR_TO_UTF8(*Message));
			}
			throw std::runtime_error("Could not find the suitable physical device!");
		}

		TArray<float> QueuePriorities{ 1.0f };
		VkDeviceQueueCreateInfo QueueInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = GraphicsQueueIndex,
			.queueCount = 1,
			.pQueuePriorities = QueuePriorities.GetData()
		};

		VkPhysicalDeviceFeatures2 Features{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
		};	
		vkGetPhysicalDeviceFeatures2(GPhysicalDevice, &Features);

		VkDeviceCreateInfo DeviceInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &Features,
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &QueueInfo,
			.enabledExtensionCount = (uint32_t)DeviceExts.Num(),
			.ppEnabledExtensionNames = DeviceExts.GetData(),
		};

		VkCheck(vkCreateDevice(GPhysicalDevice, &DeviceInfo, nullptr, &GDevice));
		vkGetDeviceQueue(GDevice, GraphicsQueueIndex, 0, &GGraphicsQueue);
	}

	void InitVulkanCore()
	{
		if (volkInitialize() != VK_SUCCESS)
		{
			throw std::runtime_error("Current device does not support Vulkan. Please try updating your graphics driver.");
		}

		CreateInstance();
		volkLoadInstance(GInstance);

		CreateDevice();
		volkLoadDevice(GDevice);

		VmaAllocatorCreateInfo AllocatorCreateInfo = {};
		AllocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT |
			VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT |
			VMA_ALLOCATOR_CREATE_KHR_EXTERNAL_MEMORY_WIN32_BIT;
		AllocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_1;
		AllocatorCreateInfo.physicalDevice = GPhysicalDevice;
		AllocatorCreateInfo.device = GDevice;
		AllocatorCreateInfo.instance = GInstance;

		VmaVulkanFunctions VulkanFunctions;
		vmaImportVulkanFunctionsFromVolk(&AllocatorCreateInfo, &VulkanFunctions);
		AllocatorCreateInfo.pVulkanFunctions = &VulkanFunctions;

		vmaCreateAllocator(&AllocatorCreateInfo, &GAllocator);
	}
}

