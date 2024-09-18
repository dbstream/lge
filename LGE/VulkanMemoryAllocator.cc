/**
 * Provide the VulkanMemoryAllocator implementation.
 * Copyright (C) 2024  dbstream
 */

#include <VKFW/vkfw.h>

#define VMA_IMPLEMENTATION 1
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "../vendor/vk_mem_alloc.h"
