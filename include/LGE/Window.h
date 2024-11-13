/** 
 * Window management.
 * Copyright (C) 2024  dbstream
 */
#pragma once

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>

typedef struct VKFWwindow_T VKFWwindow;

namespace LGE {

class Window {
private:
	VKFWwindow *m_wnd;
	VkSurfaceKHR m_surface;

	VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
	bool m_swapchainDirty = false;

	VkExtent2D m_swapchainExtent;
	VkPresentModeKHR m_presentMode;
	VkFormat m_format;

	uint32_t m_swapchainSize;
	VkImage *m_images;
	VkImageView *m_imageViews;

	uint32_t m_acquiredIndex = UINT32_MAX;

	uint64_t m_generation = 0;
public:
	Window (void);
	~Window (void);

	/**
	 * Recreate the swapchain.
	 */
	void
	CreateSwapchain (void);

	/**
	 * Acquire an image from the swapchain.
	 *
	 * @param index pointer to a uint32_t that will be overwritten by the
	 * index of the acquired image.
	 * @param sema VkSemaphore argument to VkAcquireNextImageKHR.
	 *
	 * @return true if an image was successfully acquired.
	 */
	bool
	AcquireSwapchainImage (uint32_t *index, VkSemaphore sema);

	/**
	 * Present a previously acquired image to the swapchain.
	 *
	 * @param index index returned by AcquireSwapchainImage.
	 * @param sema VkSemaphore argument to VkQueuePresent.
	 */
	void
	PresentSwapchainImage (uint32_t index, VkSemaphore sema);

	/**
	 * Get an image view from the swapchain.
	 *
	 * @param index index returned by AcquireSwapchainImage.
	 *
	 * @return the corresponding image view.
	 */
	VkImageView
	GetImageView (uint32_t index)
	{
		return m_imageViews[index];
	}

	/**
	 * Get the current format of the swapchain images.
	 *
	 * @return the current format of the swapchain images.
	 */
	VkFormat
	GetSwapchainFormat (void)
	{
		return m_format;
	}

	/**
	 * Get the current extent of the swapchain images.
	 *
	 * @return the current extent of the swapchain images.
	 */
	VkExtent2D
	GetSwapchainExtent (void)
	{
		return m_swapchainExtent;
	}

	/**
	 * Check if we have a swapchain that can block on acquire.
	 *
	 * @return true if vkAcquireNextImageKHR may sleep, otherwise false.
	 */
	bool
	IsVsyncSwapchain (void)
	{
		if (m_swapchain == VK_NULL_HANDLE)
			return false;
		return m_presentMode == VK_PRESENT_MODE_FIFO_KHR
			|| m_presentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR;
	}

	/**
	 * Get the swapchain generation counter.
	 *
	 * This can be used by code that wishes to know if the swapchain has
	 * been recreated between two points in time, by testing the generation
	 * counter values for inequality.
	 *
	 * @return swapchain generation counter.
	 */
	uint64_t
	GetGenerationCounter (void)
	{
		return m_generation;
	}

	/**
	 * Set the swapchainDirty flag to true.
	 *
	 * This should be called when the WINDOW_RESIZE_NOTIFY event is
	 * received.
	 */
	void
	SetSwapchainDirty (void)
	{
		m_swapchainDirty = true;
	}
};

extern Window *gWindow;

}
