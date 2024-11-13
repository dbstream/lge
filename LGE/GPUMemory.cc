/**
 * GPU memory management.
 * Copyright (C) 2024  dbstream
 */
#define LGE_MODULE "LGEGpuMemory"

#include <LGE/Application.h>
#include <LGE/Descriptor.h>
#include <LGE/GPUMemory.h>
#include <LGE/Log.h>
#include <LGE/VulkanFunctions.h>

#include <stdexcept>
#include <string.h>

#include <vector>

#include <vulkan/vulkan_format_traits.hpp>

#include "../vendor/vk_mem_alloc.h"

namespace LGE {

static VmaAllocator gAllocator;

class TemporaryCommandBuffer {
public:
	VkCommandPool m_pool = VK_NULL_HANDLE;
	VkCommandBuffer m_cmd = VK_NULL_HANDLE;

	~TemporaryCommandBuffer (void)
	{
		if (m_pool != VK_NULL_HANDLE)
			::vkDestroyCommandPool (gVkDevice, m_pool, nullptr);
	}

	VkCommandBuffer
	create (void)
	{
		VkCommandPoolCreateInfo pool_ci {};
		pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_ci.queueFamilyIndex = gVkQueueFamily;

		VkResult result = ::vkCreateCommandPool (gVkDevice,
			&pool_ci, nullptr, &m_pool);

		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkCreateCommandPool returned ") + VulkanTypeToString (result));

		VkCommandBufferAllocateInfo cmd_ai {};
		cmd_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmd_ai.commandPool = m_pool;
		cmd_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmd_ai.commandBufferCount = 1;

		result = ::vkAllocateCommandBuffers (gVkDevice, &cmd_ai, &m_cmd);
		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkAllocateCommandBuffers returned ") + VulkanTypeToString (result));

		VkCommandBufferBeginInfo begin_info {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		result = ::vkBeginCommandBuffer (m_cmd, &begin_info);
		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkBeginCommandBuffer returned ") + VulkanTypeToString (result));

		return m_cmd;
	}

	void
	submit (void)
	{
		VkResult result = ::vkEndCommandBuffer (m_cmd);
		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkEndCommandBuffer returned ") + VulkanTypeToString (result));

		VkSubmitInfo submit_info {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &m_cmd;
		result = ::vkQueueSubmit (gVkQueue, 1, &submit_info, VK_NULL_HANDLE);
		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkQueueSubmit returned ") + VulkanTypeToString (result));

		result = ::vkQueueWaitIdle (gVkQueue);
		if (result != VK_SUCCESS)
			Log ("warning: vkQueueWaitIdle returned %s", VulkanTypeToString (result));
	}
};

struct StagingBuffer {
public:
	VkBuffer m_buffer = VK_NULL_HANDLE;
	VmaAllocation m_allocation = VK_NULL_HANDLE;

	~StagingBuffer (void)
	{
		if (m_allocation != VK_NULL_HANDLE)
			::vmaDestroyBuffer (gAllocator, m_buffer, m_allocation);
	}

	VkBuffer
	create (const void *data, size_t size)
	{
		VkBufferCreateInfo buffer_ci {};
		buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_ci.size = size;
		buffer_ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		buffer_ci.queueFamilyIndexCount = 1;
		buffer_ci.pQueueFamilyIndices = &gVkQueueFamily;

		VmaAllocationCreateInfo alloc_info {};
		alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
			| VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
		alloc_info.priority = 0.5f;

		VmaAllocationInfo info;
		VkResult result = ::vmaCreateBuffer (gAllocator, &buffer_ci, &alloc_info,
			&m_buffer, &m_allocation, &info);

		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vmaCreateBuffer returned ") + VulkanTypeToString (result));

		::memcpy (info.pMappedData, data, size);
		return m_buffer;
	}
};

void
MMInit (void)
{
	VmaAllocatorCreateInfo allocator_ci {};
	allocator_ci.flags = 0;
	allocator_ci.physicalDevice = gVkPhysicalDevice;
	allocator_ci.device = gVkDevice;
	allocator_ci.instance = gVkInstance;
	allocator_ci.vulkanApiVersion = gVulkanDeviceVersion;

	if (gVkFeatures12.bufferDeviceAddress)
		allocator_ci.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	VkResult result = ::vmaCreateAllocator (&allocator_ci, &gAllocator);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vmaCreateAllocator returned ") + VulkanTypeToString (result));

	DescriptorInit ();
}

void
MMTerminate (void)
{
	DescriptorTerminate ();

	// quick and dirty way to flush temporary buffers
	for (size_t i = 0; i < CPU_RENDER_AHEAD; i++)
		MMNextFrame ();

	::vmaDestroyAllocator (gAllocator);
}

void
MMDestroyGPUBuffer (GPUBuffer &buffer)
{
	::vmaDestroyBuffer (gAllocator, buffer.m_buffer, buffer.m_allocation);
	buffer.m_buffer = VK_NULL_HANDLE;
	buffer.m_allocation = VK_NULL_HANDLE;
}

GPUBuffer
MMCreateMeshGPUBuffer (const void *data, size_t size, VkBufferUsageFlags usage)
{
	if (size > UINT64_MAX)
		throw std::runtime_error ("MMCreateMeshGPUBuffer: too large!");

	VkBufferCreateInfo buffer_ci {};
	buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_ci.size = size;
	buffer_ci.usage = usage;
	buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_ci.queueFamilyIndexCount = 1;
	buffer_ci.pQueueFamilyIndices = &gVkQueueFamily;

	if (data)
		// we are going to perform an initial transfer to the buffer
		buffer_ci.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VmaAllocationCreateInfo alloc_info {};
	alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	alloc_info.priority = 0.5f;

	GPUBuffer mesh;
	VkResult result = ::vmaCreateBuffer (gAllocator, &buffer_ci, &alloc_info,
		&mesh.m_buffer, &mesh.m_allocation, nullptr);

	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vmaCreateBuffer returned ") + VulkanTypeToString (result));

	if (data) {
		try {
			Log ("Copy via MMCopyToGPUBuffer");
			MMCopyToGPUBuffer (mesh, data, size, 0);
			Log ("Copy done");
		} catch (...) {
			::vmaDestroyBuffer (gAllocator, mesh.m_buffer, mesh.m_allocation);
			throw;
		}
	}

	return mesh;
}

void
MMCopyToGPUBuffer (GPUBuffer &target, const void *data, size_t size, size_t offset)
{
	StagingBuffer stagingmgr;
	VkBuffer staging = stagingmgr.create (data, size);

	TemporaryCommandBuffer cmdmgr;
	VkCommandBuffer cmd = cmdmgr.create ();

	VkBufferCopy copy_info {};
	copy_info.srcOffset = 0;
	copy_info.dstOffset = offset;
	copy_info.size = size;
	::vkCmdCopyBuffer (cmd, staging, target.m_buffer, 1, &copy_info);

	cmdmgr.submit ();
}

static size_t stash_index = 0;
static std::vector<GPUBuffer> stash[CPU_RENDER_AHEAD];

VkBuffer
MMCreateTemporaryGPUBuffer (const void *data, size_t size, VkBufferUsageFlags usage)
{
	if (size > UINT64_MAX)
		throw std::runtime_error ("MMCreateTemporaryGPUBuffer: too large!");

	VkBufferCreateInfo buffer_ci {};
	buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_ci.size = size;
	buffer_ci.usage = usage;
	buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_ci.queueFamilyIndexCount = 1;
	buffer_ci.pQueueFamilyIndices = &gVkQueueFamily;

	VmaAllocationCreateInfo alloc_info {};
	alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT
		| VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	alloc_info.priority = 0.5f;

	GPUBuffer buffer;
	VmaAllocationInfo info;
	VkResult result = ::vmaCreateBuffer (gAllocator, &buffer_ci, &alloc_info,
		&buffer.m_buffer, &buffer.m_allocation, &info);

	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vmaCreateBuffer returned ") + VulkanTypeToString (result));

	try {
		stash[stash_index].push_back (buffer);
	} catch (...) {
		::vmaDestroyBuffer (gAllocator, buffer.m_buffer, buffer.m_allocation);
		throw;
	}

	::memcpy (info.pMappedData, data, size);
	return buffer.m_buffer;
}

void
MMDestroyGPUImage (GPUImage &image)
{
	::vmaDestroyImage (gAllocator, image.m_image, image.m_allocation);
	image.m_image = VK_NULL_HANDLE;
	image.m_allocation = VK_NULL_HANDLE;
}

GPUImage
MMCreateGPUImage (VkImageType type, VkExtent3D extent, VkFormat format,
	VkImageUsageFlags usage)
{
	VkImageCreateInfo image_ci {};
	image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_ci.imageType = type;
	image_ci.format = format;
	image_ci.extent = extent;
	image_ci.mipLevels = 1;
	image_ci.arrayLayers = 1;
	image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
	image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_ci.usage = usage;
	image_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_ci.queueFamilyIndexCount = 1;
	image_ci.pQueueFamilyIndices = &gVkQueueFamily;
	image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo alloc_ci {};
	alloc_ci.usage = VMA_MEMORY_USAGE_AUTO;
	alloc_ci.priority = 0.5f;

	GPUImage image;
	VkResult result = ::vmaCreateImage (gAllocator, &image_ci, &alloc_ci,
		&image.m_image, &image.m_allocation, nullptr);

	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vmaCreateImage returned ") + VulkanTypeToString (result));

	return image;
}

GPUImage
MMUploadTexture2D (VkFormat format, VkExtent2D extent, const void *data)
{
	VkExtent3D extent3d {};
	extent3d.width = extent.width;
	extent3d.height = extent.height;
	extent3d.depth = 1;
	GPUImage image = MMCreateGPUImage (VK_IMAGE_TYPE_2D, extent3d, format,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	try {
		VkDeviceSize size = (VkDeviceSize) extent.width * extent.height
			* vk::blockSize (static_cast <vk::Format> (format));

		StagingBuffer stagingmgr;
		VkBuffer staging = stagingmgr.create (data, size);

		TemporaryCommandBuffer cmdmgr;
		VkCommandBuffer cmd = cmdmgr.create ();

		VkImageMemoryBarrier barrier {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image.m_image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier (cmd,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkBufferImageCopy copy {};
		copy.bufferRowLength = extent.width;
		copy.bufferImageHeight = extent.height;
		copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.imageSubresource.layerCount = 1;
		copy.imageExtent = extent3d;
		vkCmdCopyBufferToImage (cmd, staging, image.m_image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkCmdPipelineBarrier (cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		cmdmgr.submit ();
	} catch (...) {
		MMDestroyGPUImage (image);
		throw;
	}

	return image;
}

void
MMNextFrame (void)
{
	DescriptorNextFrame ();

	stash_index++;
	if (stash_index >= CPU_RENDER_AHEAD)
		stash_index = 0;

	for (GPUBuffer &b : stash[stash_index])
		MMDestroyGPUBuffer (b);
	stash[stash_index].clear ();
}

}
