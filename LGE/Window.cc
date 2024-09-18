/**
 * Window abstraction.
 * Copyright (C) 2024  dbstream
 */
#define LGE_MODULE "LGEWindow"

#include <LGE/Application.h>
#include <LGE/Log.h>
#include <LGE/Vulkan.h>
#include <LGE/Window.h>

#include <VKFW/vkfw.h>

#include <stdexcept>
#include <vector>

namespace LGE {

Window *gWindow;

Window::Window (void)
{
	VkResult result;
	if ((result = ::vkfwCreateWindow (&m_wnd, {1280, 720})) != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkfwCreateWindow returned ") + VulkanTypeToString (result));

	::vkfwSetWindowUserPointer (m_wnd, this);

	// This can fail, but we don't care.
	::vkfwSetWindowTitle (m_wnd, gApplication->GetUserFriendlyName ());

	if ((result = ::vkfwShowWindow (m_wnd)) != VK_SUCCESS) {
		::vkfwDestroyWindow (m_wnd);
		throw std::runtime_error (std::string ("vkfwShowWindow returned ") + VulkanTypeToString (result));
	}

	if ((result = ::vkfwCreateSurface (m_wnd, &m_surface)) != VK_SUCCESS) {
		::vkfwDestroyWindow (m_wnd);
		throw std::runtime_error (std::string ("vkfwCreateSurface returned ") + VulkanTypeToString (result));
	}
}

Window::~Window (void)
{
	if (m_swapchain != VK_NULL_HANDLE) {
		for (uint32_t i = 0; i < m_swapchainSize; i++)
			vkDestroyImageView (gVkDevice, m_imageViews[i], nullptr);
		delete[] m_imageViews;
		delete[] m_images;
		::vkDeviceWaitIdle (gVkDevice);
		::vkDestroySwapchainKHR (gVkDevice, m_swapchain, nullptr);
	}

	::vkDestroySurfaceKHR (gVkInstance, m_surface, nullptr);
	::vkfwDestroyWindow (m_wnd);
}

void
Window::CreateSwapchain (void)
{
	auto choose_format = [&](void) -> VkSurfaceFormatKHR {
		VkResult result;
		uint32_t count = 0;

		result = ::vkGetPhysicalDeviceSurfaceFormatsKHR (gVkPhysicalDevice, m_surface, &count, nullptr);
		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkGetPhysicalDeviceSurfaceFormatsKHR returned ") + VulkanTypeToString (result));

		std::vector<VkSurfaceFormatKHR> formats (count);
		result = ::vkGetPhysicalDeviceSurfaceFormatsKHR (gVkPhysicalDevice, m_surface, &count, formats.data ());
		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkGetPhysicalDeviceSurfaceFormatsKHR returned ") + VulkanTypeToString (result));

		const VkSurfaceFormatKHR preferred_formats[] = {
			{ VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
			{ VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
			{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
			{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
		};

		for (const VkSurfaceFormatKHR &format : preferred_formats)
			for (const VkSurfaceFormatKHR &available : formats)
				if (format.format == available.format && format.colorSpace == available.colorSpace)
					return format;

		throw std::runtime_error ("No supported surface format is available");
	};

	auto choose_present_mode = [&](void) -> VkPresentModeKHR {
		VkResult result;
		uint32_t count = 0;

		result = ::vkGetPhysicalDeviceSurfacePresentModesKHR (gVkPhysicalDevice, m_surface, &count, nullptr);
		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkGetPhysicalDeviceSurfacePresentModes returned ") + VulkanTypeToString (result));

		std::vector<VkPresentModeKHR> modes (count);
		result = ::vkGetPhysicalDeviceSurfacePresentModesKHR (gVkPhysicalDevice, m_surface, &count, modes.data ());
		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkGetPhysicalDeviceSurfacePresentModes returned ") + VulkanTypeToString (result));

		const VkPresentModeKHR preferred_modes[] = {
			VK_PRESENT_MODE_FIFO_RELAXED_KHR,
			VK_PRESENT_MODE_FIFO_KHR
		};

		for (VkPresentModeKHR mode : preferred_modes)
			for (VkPresentModeKHR available : modes)
				if (mode == available)
					return mode;

		throw std::runtime_error ("No supported present mode is available");
	};

	VkResult result;

	VkBool32 b;
	result = ::vkGetPhysicalDeviceSurfaceSupportKHR (gVkPhysicalDevice, gVkQueueFamily, m_surface, &b);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkGetPhysicalDeviceSurfaceSupportKHR returned ") + VulkanTypeToString (result));

	if (!b)
		throw std::runtime_error ("vkGetPhysicalDeviceSurfaceSupportKHR says not supported");

	VkSurfaceCapabilitiesKHR capabilities;
	result = ::vkGetPhysicalDeviceSurfaceCapabilitiesKHR (gVkPhysicalDevice, m_surface, &capabilities);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkGetPhysicalDeviceSurfaceCapabilitiesKHR returned ") + VulkanTypeToString (result));

	VkSurfaceFormatKHR format = choose_format ();
	VkPresentModeKHR present_mode = choose_present_mode ();

	VkExtent2D extent = vkfwGetFramebufferExtent (m_wnd);
	if (capabilities.minImageExtent.width > extent.width)
		extent.width = capabilities.minImageExtent.width;
	else if (capabilities.maxImageExtent.width < extent.width)
		extent.width = capabilities.maxImageExtent.width;
	if (capabilities.minImageExtent.height > extent.height)
		extent.height = capabilities.minImageExtent.height;
	else if (capabilities.maxImageExtent.height < extent.height)
		extent.height = capabilities.maxImageExtent.height;

	// Log ("Creating %ux%u swapchain using %s, %s, %s", (unsigned int) extent.width, (unsigned int) extent.height,
	//	VulkanTypeToString (present_mode), VulkanTypeToString (format.format), VulkanTypeToString (format.colorSpace));

	VkSwapchainCreateInfoKHR swapchain_ci {};
	swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_ci.surface = m_surface;
	swapchain_ci.minImageCount = 3;
	if (capabilities.minImageCount > swapchain_ci.minImageCount)
		swapchain_ci.minImageCount = capabilities.minImageCount;
	else if (capabilities.maxImageCount && capabilities.maxImageCount < swapchain_ci.minImageCount)
		swapchain_ci.minImageCount = capabilities.maxImageCount;
	swapchain_ci.imageFormat = format.format;
	swapchain_ci.imageColorSpace = format.colorSpace;
	swapchain_ci.imageExtent = extent;
	swapchain_ci.imageArrayLayers = 1;
	swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_ci.queueFamilyIndexCount = 1;
	swapchain_ci.pQueueFamilyIndices = &gVkQueueFamily;
	swapchain_ci.preTransform = capabilities.currentTransform;
	swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_ci.presentMode = present_mode;
	swapchain_ci.oldSwapchain = m_swapchain;

	VkSwapchainKHR new_swapchain;
	result = ::vkCreateSwapchainKHR (gVkDevice, &swapchain_ci, nullptr, &new_swapchain);

	m_swapchainDirty = true;
	m_acquiredIndex = UINT32_MAX;

	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkCreateSwapchainKHR returned ") + VulkanTypeToString (result));

	if (m_swapchain) {
		::vkDeviceWaitIdle (gVkDevice);
		for (uint32_t i = 0; i < m_swapchainSize; i++)
			::vkDestroyImageView (gVkDevice, m_imageViews[i], nullptr);
		delete[] m_imageViews;
		delete[] m_images;
		::vkDestroySwapchainKHR (gVkDevice, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
	}

	m_generation++;
	m_swapchainDirty = false;
	m_swapchainExtent = extent;
	m_presentMode = present_mode;
	m_format = format.format;

	m_swapchainSize = 0;
	result = ::vkGetSwapchainImagesKHR (gVkDevice, new_swapchain, &m_swapchainSize, nullptr);
	if (result != VK_SUCCESS) {
		::vkDestroySwapchainKHR (gVkDevice, m_swapchain, nullptr);
		throw std::runtime_error (std::string ("vkGetSwapchainImagesKHR returned ") + VulkanTypeToString (result));
	}

	try {
		m_images = new VkImage[m_swapchainSize];
	} catch (...) {
		::vkDestroySwapchainKHR (gVkDevice, new_swapchain, nullptr);
		throw;
	}

	try {
		m_imageViews = new VkImageView[m_swapchainSize];
	} catch (...) {
		delete[] m_images;
		::vkDestroySwapchainKHR (gVkDevice, new_swapchain, nullptr);
		throw;
	}

	result = ::vkGetSwapchainImagesKHR (gVkDevice, new_swapchain, &m_swapchainSize, m_images);
	if (result != VK_SUCCESS) {
		delete[] m_imageViews;
		delete[] m_images;
		::vkDestroySwapchainKHR (gVkDevice, new_swapchain, nullptr);
		throw std::runtime_error (std::string ("vkGetSwapchainImagesKHR returned ") + VulkanTypeToString (result));
	}

	for (uint32_t i = 0; i < m_swapchainSize; i++) {
		VkImageViewCreateInfo view_ci {};
		view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_ci.image = m_images[i];
		view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_ci.format = m_format;
		view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_ci.subresourceRange.levelCount = 1;
		view_ci.subresourceRange.layerCount = 1;
		result = ::vkCreateImageView (gVkDevice, &view_ci, nullptr, &m_imageViews[i]);
		if (result != VK_SUCCESS) {
			while (i)
				::vkDestroyImageView (gVkDevice, m_imageViews[--i], nullptr);
			delete[] m_imageViews;
			delete[] m_images;
			::vkDestroySwapchainKHR (gVkDevice, new_swapchain, nullptr);
			throw std::runtime_error (std::string ("vkCreateImageview returned ") + VulkanTypeToString (result));
		}
	}

	m_swapchain = new_swapchain;
}

bool
Window::AcquireSwapchainImage (uint32_t *index, VkSemaphore sema)
{
	VkResult result;

	if (m_acquiredIndex != UINT32_MAX) {
		*index = m_acquiredIndex;
		return true;
	}

	if (m_swapchainDirty || m_swapchain == VK_NULL_HANDLE)
		CreateSwapchain ();

	result = ::vkAcquireNextImageKHR (gVkDevice, m_swapchain, UINT64_MAX, sema, VK_NULL_HANDLE, index);

	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
		m_swapchainDirty = true;
	else if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkAcquireNextImageKHR returned ") + VulkanTypeToString (result));

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		return false;

	m_acquiredIndex = *index;
	return true;
}

void
Window::PresentSwapchainImage (uint32_t index, VkSemaphore sema)
{
	m_acquiredIndex = UINT32_MAX;

	VkPresentInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.waitSemaphoreCount = sema != VK_NULL_HANDLE;
	info.pWaitSemaphores = (sema != VK_NULL_HANDLE) ? &sema : nullptr;
	info.swapchainCount = 1;
	info.pSwapchains = &m_swapchain;
	info.pImageIndices = &index;

	VkResult result = ::vkQueuePresentKHR (gVkQueue, &info);
	if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR)
		m_swapchainDirty = true;
	else if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkQueuePresentKHR returned ") + VulkanTypeToString (result));
}

}
