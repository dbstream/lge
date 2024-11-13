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
 * @note Use this when uploading mesh data that will live for a long time.
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
 * Copy data to the GPU buffer, via a staging buffer. The target buffer should
 * be created with VK_BUFFER_USAGE_TRANSFER_DST_BIT.
 *
 * @param target target GPU buffer.
 * @param data pointer to data.
 * @param size size of data.
 * @param offset destination offset.
 */
void
MMCopyToGPUBuffer (GPUBuffer &target, const void *data, size_t size, size_t offset);

/**
 * Create a temporary GPU buffer and fill it with the given data. The buffer
 * will be freed automatically by LGE after the current frame has completed
 * rendering.
 *
 * @note Use this when uploading data that will be referenced only during the
 * current frame, such as a uniform buffer with camera information.
 *
 * @param data pointer to data.
 * @param size size of data.
 * @param usage Vulkan buffer usage bits.
 *
 * @return Vulkan buffer.
 */
VkBuffer
MMCreateTemporaryGPUBuffer (const void *data, size_t size, VkBufferUsageFlags usage);

struct GPUImage {
	VkImage m_image = VK_NULL_HANDLE;
	VmaAllocation m_allocation = VK_NULL_HANDLE;

	constexpr
	operator bool (void) const
	{
		return m_image != VK_NULL_HANDLE;
	}
};

/**
 * Destroy the GPU image and free the associated memory.
 *
 * @param image image to destroy.
 */
void
MMDestroyGPUImage (GPUImage &image);

/**
 * Create a GPU image.
 *
 * @param type image type.
 * @param extent image extent. If 1D, y and z must be 1. If 2D, z must be 1.
 * @param format image format.
 * @param usage image usage.
 */
GPUImage
MMCreateGPUImage (VkImageType type, VkExtent3D extent, VkFormat format,
	VkImageUsageFlags usage);

/**
 * Upload a 2D texture to the GPU.
 *
 * @param format image format.
 * @param extent image extent.
 * @param data pixel data, must match format.
 */
GPUImage
MMUploadTexture2D (VkFormat format, VkExtent2D extent, const void *data);

/**
 * Tell the memory manager that the VkFence for rendering operations on a frame
 * has completed.
 */
void
MMNextFrame (void);

}
