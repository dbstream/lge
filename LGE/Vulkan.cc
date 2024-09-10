/** 
 * Vulkan globals and helpers.
 * Copyright (C) 2024  dbstream
 */
#define LGE_MODULE "LGEVulkan"

#include <LGE/Application.h>
#include <LGE/Init.h>
#include <LGE/Log.h>
#include <LGE/Vulkan.h>

#include <VKFW/vkfw.h>

#include <vulkan/vk_enum_string_helper.h> // used by VulkanTypeToString

#include <exception>
#include <vector>

#include <string.h>

namespace LGE {

/**
 * Create one "everything queue". Most if not all modern hardware provide one
 * queue that has graphics, compute, and present support.
 */

uint32_t gVulkanVersion;
uint32_t gVulkanDeviceVersion;
VkInstance gVkInstance;
VkPhysicalDevice gVkPhysicalDevice;
VkDevice gVkDevice;
VkQueue gVkQueue;
VkPhysicalDeviceFeatures gVkFeatures10;
VkPhysicalDeviceVulkan11Features gVkFeatures11;
VkPhysicalDeviceVulkan12Features gVkFeatures12;
VkPhysicalDeviceVulkan13Features gVkFeatures13;
uint32_t gVkQueueFamily;

bool
InitializeVulkan (void)
{
	bool flag = false;
	auto instance_ext = [&](const char *name, bool required){
		if (::vkfwRequestInstanceExtension (name, required) != VK_SUCCESS)
			flag = true;
	};
	auto device_ext = [&](const char *name, bool required){
		if (::vkfwRequestDeviceExtension (name, required) != VK_SUCCESS)
			flag = true;
	};

	gVulkanVersion = ::vkfwGetVkInstanceVersion ();
	if (VK_API_VERSION_VARIANT (gVulkanVersion) != 0 || gVulkanVersion < VK_API_VERSION_1_3) {
		Log ("The current Vulkan ICD does not support Vulkan core 1.3 (gVulkanVersion=%d.%d.%d, variant=%d)",
			static_cast<int> (VK_API_VERSION_MAJOR (gVulkanVersion)),
			static_cast<int> (VK_API_VERSION_MINOR (gVulkanVersion)),
			static_cast<int> (VK_API_VERSION_PATCH (gVulkanVersion)),
			static_cast<int> (VK_API_VERSION_VARIANT (gVulkanVersion)));
		return false;
	}

	instance_ext (VK_KHR_SURFACE_EXTENSION_NAME, true);
	device_ext (VK_KHR_SWAPCHAIN_EXTENSION_NAME, true);
	if (flag)
		return false;

	/**
	 * VKFW fills in most of VkInstanceCreateInfo for us. Note that the same
	 * does not apply to VkApplicationInfo.
	 */
	VkInstanceCreateInfo instance_ci {};
	VkApplicationInfo app_info {};

	instance_ci.pApplicationInfo = &app_info;
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = gApplication->GetUserFriendlyName ();
	app_info.pEngineName = "dbstream/LGE";
	app_info.apiVersion = VK_API_VERSION_1_3;

	uint32_t vkfwFlags = 0;
	if (!bIsProduction && bLoggingEnabled)
		vkfwFlags |= VKFW_CREATE_INSTANCE_DEBUG_MESSENGER;

	if (::vkfwCreateInstance (&gVkInstance, &instance_ci, vkfwFlags) != VK_SUCCESS)
		return false;

	/**
	 * Our current device selection can be substantially improved. Currently
	 * we look for a discrete GPU, then an integrated GPU, then any GPU,
	 * without checking for extension support or anything. Additionally, we
	 * enable the full GPU feature set, regardless of what we use.
	 */

	auto choose_gpu = [&](void) -> VkPhysicalDevice {
		uint32_t count;
		VkResult result = ::vkEnumeratePhysicalDevices (gVkInstance, &count, nullptr);

		if (!count || result != VK_SUCCESS)
			return VK_NULL_HANDLE;

		std::vector<VkPhysicalDevice> devices (count);
		result = ::vkEnumeratePhysicalDevices (gVkInstance, &count, devices.data ());
		if (result != VK_SUCCESS)
			return VK_NULL_HANDLE;

		int best = 0;
		VkPhysicalDevice bestDev = VK_NULL_HANDLE;
		for (VkPhysicalDevice device : devices) {
			VkPhysicalDeviceProperties props;
			::vkGetPhysicalDeviceProperties (device, &props);

			int score;
			switch (props.deviceType) {
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				score = 3;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				score = 2;
				break;
			default:
				score = 1;
				break;
			}

			if (score > best) {
				bestDev = device;
				best = score;
			}
		}

		return bestDev;
	};

	try {
		gVkPhysicalDevice = choose_gpu ();
		if (gVkPhysicalDevice == VK_NULL_HANDLE)
			return false;
	} catch (const std::exception &e) {
		Log ("choose_gpu threw an exception: %s", e.what ());
		return false;
	}

	VkPhysicalDeviceProperties props;
	::vkGetPhysicalDeviceProperties (gVkPhysicalDevice, &props);
	Log ("Using GPU: %s  (Vulkan version: %d.%d.%d)", props.deviceName,
		(int) VK_API_VERSION_MAJOR (props.apiVersion),
		(int) VK_API_VERSION_MINOR (props.apiVersion),
		(int) VK_API_VERSION_PATCH (props.apiVersion));

	// sanity check
	if (VK_API_VERSION_VARIANT (props.apiVersion) != 0)
		return false;

	gVulkanDeviceVersion = props.apiVersion;

	::memset (&gVkFeatures10, 0, sizeof (gVkFeatures10));
	::memset (&gVkFeatures11, 0, sizeof (gVkFeatures11));
	::memset (&gVkFeatures12, 0, sizeof (gVkFeatures12));
	::memset (&gVkFeatures13, 0, sizeof (gVkFeatures13));
	gVkFeatures11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	gVkFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	gVkFeatures13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

	// VKFW fills in most of DeviceCreateInfo for us.
	VkDeviceCreateInfo device_ci {};

	VkPhysicalDeviceFeatures2 feat2  {};
	if (gVulkanDeviceVersion >= VK_API_VERSION_1_1) {
		feat2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		feat2.pNext = &gVkFeatures11;
		if (gVulkanDeviceVersion >= VK_API_VERSION_1_2) {
			gVkFeatures11.pNext = &gVkFeatures12;
			if (gVulkanDeviceVersion >= VK_API_VERSION_1_3) {
				gVkFeatures12.pNext = &gVkFeatures13;
			}
		}

		::vkGetPhysicalDeviceFeatures2 (gVkPhysicalDevice, &feat2);
		gVkFeatures10 = feat2.features;

		device_ci.pNext = &feat2;
	} else {
		::vkGetPhysicalDeviceFeatures (gVkPhysicalDevice, &gVkFeatures10);
		device_ci.pEnabledFeatures = &gVkFeatures10;
	}

	auto choose_queue = [&](void) -> uint32_t {
		uint32_t count;
		::vkGetPhysicalDeviceQueueFamilyProperties (gVkPhysicalDevice, &count, nullptr);

		std::vector<VkQueueFamilyProperties> queues (count);
		::vkGetPhysicalDeviceQueueFamilyProperties (gVkPhysicalDevice, &count, queues.data ());

		constexpr uint32_t required_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
		for (uint32_t i = 0; i < count; i++) {
			VkQueueFamilyProperties &p = queues[i];
			if ((p.queueFlags & required_flags) != required_flags)
				continue;
			VkBool32 b;
			if (::vkfwGetPhysicalDevicePresentSupport (gVkPhysicalDevice, i, &b) != VK_SUCCESS)
				return UINT32_MAX;
			if (!b)
				continue;

			return i;
		}

		return UINT32_MAX;
	};

	try {
		gVkQueueFamily = choose_queue ();
		if (gVkQueueFamily == UINT32_MAX)
			return false;
	} catch (const std::exception &e) {
		Log ("choose_queue threw an exception: %s", e.what ());
		return false;
	}

	float qp = 1.0f;
	VkDeviceQueueCreateInfo queue_ci {};
	queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_ci.queueFamilyIndex = gVkQueueFamily;
	queue_ci.queueCount = 1;
	queue_ci.pQueuePriorities = &qp;

	device_ci.queueCreateInfoCount = 1;
	device_ci.pQueueCreateInfos = &queue_ci;

	if (::vkfwCreateDevice (&gVkDevice, gVkPhysicalDevice, &device_ci) != VK_SUCCESS) {
		Log ("Failed to create Vulkan device. Try using MESA_VK_DEVICE_SELECT= or similar.");
		return false;
	}

	::vkGetDeviceQueue (gVkDevice, gVkQueueFamily, 0, &gVkQueue);

	return true;
}

void
TerminateVulkan (void)
{
	// vkfwTerminate() will destroy the VkDevice and the VkInstance for us.
}

/** VulkanTypeToString instantiations */

template<> const char *
VulkanTypeToString<VkResult> (VkResult value)
{
	return string_VkResult (value);
}

template<> const char *
VulkanTypeToString<VkFormat> (VkFormat value)
{
	return string_VkFormat (value);
}

template<> const char *
VulkanTypeToString<VkColorSpaceKHR> (VkColorSpaceKHR value)
{
	return string_VkColorSpaceKHR (value);
}

template<> const char *
VulkanTypeToString<VkPresentModeKHR> (VkPresentModeKHR value)
{
	return string_VkPresentModeKHR (value);
}

}
