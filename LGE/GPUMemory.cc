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

#include "../vendor/vk_mem_alloc.h"

namespace LGE {

static VmaAllocator gAllocator;

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

	VkBuffer staging_buffer;
	VmaAllocation staging_alloc;
	VmaAllocationInfo info;
	VkResult result = ::vmaCreateBuffer (gAllocator, &buffer_ci, &alloc_info,
		&staging_buffer, &staging_alloc, &info);

	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vmaCreateBuffer returned ") + VulkanTypeToString (result));

	::memcpy (info.pMappedData, data, size);

	VkCommandPoolCreateInfo pool_ci {};
	pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_ci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	pool_ci.queueFamilyIndex = gVkQueueFamily;

	VkCommandPool pool;
	result = ::vkCreateCommandPool (gVkDevice, &pool_ci, nullptr, &pool);
	if (result != VK_SUCCESS) {
		::vmaDestroyBuffer (gAllocator, staging_buffer, staging_alloc);
		throw std::runtime_error (std::string ("vkCreateCommandPool returned ") + VulkanTypeToString (result));
	}

	VkCommandBufferAllocateInfo cmd_ai {};
	cmd_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_ai.commandPool = pool;
	cmd_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_ai.commandBufferCount = 1;

	VkCommandBuffer cmd;
	result = ::vkAllocateCommandBuffers (gVkDevice, &cmd_ai, &cmd);
	if (result != VK_SUCCESS) {
		::vkDestroyCommandPool (gVkDevice, pool, nullptr);
		::vmaDestroyBuffer (gAllocator, staging_buffer, staging_alloc);
		throw std::runtime_error (std::string ("vkAllocateCommandBuffers returned ") + VulkanTypeToString (result));
	}

	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	result = ::vkBeginCommandBuffer (cmd, &begin_info);
	if (result != VK_SUCCESS) {
		::vkDestroyCommandPool (gVkDevice, pool, nullptr);
		::vmaDestroyBuffer (gAllocator, staging_buffer, staging_alloc);
		throw std::runtime_error (std::string ("vkBeginCommandBuffer returned ") + VulkanTypeToString (result));
	}

	VkBufferCopy copy_info {};
	copy_info.srcOffset = 0;
	copy_info.dstOffset = offset;
	copy_info.size = size;
	::vkCmdCopyBuffer (cmd, staging_buffer, target.m_buffer, 1, &copy_info);

	result = ::vkEndCommandBuffer (cmd);
	if (result != VK_SUCCESS) {
		::vkDestroyCommandPool (gVkDevice, pool, nullptr);
		::vmaDestroyBuffer (gAllocator, staging_buffer, staging_alloc);
		throw std::runtime_error (std::string ("vkEndCommandBuffer returned ") + VulkanTypeToString (result));
	}

	VkSubmitInfo si {};
	si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	si.commandBufferCount = 1;
	si.pCommandBuffers = &cmd;
	result = ::vkQueueSubmit (gVkQueue, 1, &si, VK_NULL_HANDLE);
	if (result != VK_SUCCESS) {
		::vkfw_vkDestroyCommandPool (gVkDevice, pool, nullptr);
		::vmaDestroyBuffer (gAllocator, staging_buffer, staging_alloc);
		throw std::runtime_error (std::string ("vkQueueSubmit returned ") + VulkanTypeToString (result));
	}

	result = ::vkQueueWaitIdle (gVkQueue);

	// we will destroy these no matter what
	::vkDestroyCommandPool (gVkDevice, pool, nullptr);
	::vmaDestroyBuffer (gAllocator, staging_buffer, staging_alloc);

	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkQueueWaitIdle returned ") + VulkanTypeToString (result));
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
