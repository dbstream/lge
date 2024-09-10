/**
 * Vulkan globals and helpers.
 * Copyright (C) 2024  dbstream
 */
#pragma once

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>

namespace LGE {

extern uint32_t gVulkanVersion;
extern uint32_t gVulkanDeviceVersion;
extern VkInstance gVkInstance;
extern VkPhysicalDevice gVkPhysicalDevice;
extern VkDevice gVkDevice;
extern VkQueue gVkQueue;
extern VkPhysicalDeviceFeatures gVkFeatures10;
extern VkPhysicalDeviceVulkan11Features gVkFeatures11;
extern VkPhysicalDeviceVulkan12Features gVkFeatures12;
extern VkPhysicalDeviceVulkan13Features gVkFeatures13;
extern uint32_t gVkQueueFamily;

/**
 * Get a human-readable string from a Vulkan value.
 *
 * Currently supported types: VkResult, VkFormat, VkColorSpaceKHR,
 * VkPresentModeKHR.
 *
 * @note this is implemented using explicit template instantiation. Code that
 * uses this on an unsupported type will compile, but cause linker errors.
 *
 * @return pointer to a string that will remain valid for the entire duration
 * of the program.
 */
template <class T>
const char *
VulkanTypeToString (T value);

/** 
 * Initialize Vulkan.
 *
 * @return true on success, false otherwise.
 */
bool
InitializeVulkan (void);

/**
 * Terminate Vulkan. This should not be called if InitializeVulkan has not
 * returned success.
 */
void
TerminateVulkan (void);

}
