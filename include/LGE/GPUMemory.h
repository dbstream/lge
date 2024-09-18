/**
 * GPU memory management.
 * Copyright (C) 2024  dbstream
 */
#pragma once

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>

VK_DEFINE_HANDLE (VmaAllocation)

namespace LGE {

/**
 * Initialize the GPU memory manager.
 */
void
MMInit (void);

/**
 * Shutdown the GPU memory manager.
 */
void
MMTerminate (void);

struct GPUBuffer {
	VkBuffer m_buffer = VK_NULL_HANDLE;
	VmaAllocation m_allocation = VK_NULL_HANDLE;

	constexpr
	operator bool (void) const
	{
		return m_buffer != VK_NULL_HANDLE;
	}
};

/**
 * Destroy the GPU buffer and free the associated memory.
 *
 * @param buffer buffer to destroy.
 */
void
MMDestroyGPUBuffer (GPUBuffer &buffer);

/**
 * Create a GPU buffer suitable for mesh data, and fill it with the given data.
 *
 * @param data pointer to data.
 * @param size buffer size.
 * @param usage Vulkan buffer usage bits.
 *
 * @return newly created GPU buffer.
 */
GPUBuffer
MMCreateMeshGPUBuffer (const void *data, size_t size, VkBufferUsageFlags usage);

/**
 * Copy data to the GPU buffer, via a staging buffer.
 *
 * @param target target GPU buffer.
 * @param data pointer to data.
 * @param size size of data.
 * @param offset destination offset.
 */
void
MMCopyToGPUBuffer (GPUBuffer &target, const void *data, size_t size, size_t offset);

}
