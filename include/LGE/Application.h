/**
 * Application base.
 * Copyright (C) 2024  dbstream
 */
#pragma once

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>

#include <stddef.h>

typedef struct VKFWevent_T VKFWevent;

namespace LGE {

/**
 * CPU render ahead affects resource management outside the scope of Application
 * too. Keep it a constexpr global.
 */
static constexpr size_t CPU_RENDER_AHEAD = 3;

class Application {
private:
	bool m_keep_running = true;
	size_t m_frameIndex = 0;

	VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
	VkCommandPool m_commandPool = VK_NULL_HANDLE;
	VkCommandBuffer m_commandBuffers[CPU_RENDER_AHEAD];
	VkFence m_fences[CPU_RENDER_AHEAD];
	VkSemaphore m_semaphores[2 * CPU_RENDER_AHEAD];

protected:
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
	VkFormat m_format;
	VkExtent2D m_extent;

	Application (void);
public:
	virtual
	~Application (void);

	/**
	 * Get the current application render pass.
	 *
	 * @note the render pass may not be created on application startup, and
	 * it may be recreated at any time in between frames. This function is
	 * only safe to call inside Application::Draw.
	 *
	 * @return current application render pass.
	 */
	VkRenderPass
	GetRenderPass (void)
	{
		return m_renderPass;
	}

	/**
	 * Get the name of the application.
	 *
	 * @return pointer to a string that will stay valid for the duration
	 * of the program.
	 */
	virtual const char *
	GetUserFriendlyName (void);

	/**
	 * Check if required Vulkan features and extensions are supported.
	 * Throws an exception if something is missing.
	 */
	virtual void
	CheckRequirements (void);

	/**
	 * Test if the application wants to continue running.
	 *
	 * @return boolean value indicating whether the application should
	 * keep running.
	 */
	virtual bool
	KeepRunning (void);

	/**
	 * Handle an event from the event loop.
	 *
	 * @param e event from VKFW.
	 */
	virtual void
	HandleEvent (const VKFWevent &e);

	/**
	 * Render the application.
	 */
	virtual void
	Render (void);

	/**
	 * Recreate the default render pass.
	 *
	 * @note m_format will be updated to the swapchain format before this
	 * function is called.
	 *
	 * @return a render pass that matches the format of the swapchain.
	 */
	virtual VkRenderPass
	CreateRenderPass (void);

	/**
	 * Recreate the default framebuffer.
	 *
	 * @note m_extent will be updated to the swapchain extent before this
	 * function is called.
	 *
	 * @param rp the default render pass.
	 *
	 * @return an imageless framebuffer.
	 */
	virtual VkFramebuffer
	CreateFramebuffer (VkRenderPass rp);

	/**
	 * Begin rendering with the default render pass.
	 *
	 * @param cmd command buffer used by this frame.
	 * @param rp render pass.
	 * @param fb framebuffer.
	 * @param target swapchain image to render to.
	 */
	virtual void
	BeginRendering (VkCommandBuffer cmd, VkRenderPass rp, VkFramebuffer fb, VkImageView target);

	/**
	 * Send draw commands to the command buffer.
	 *
	 * This function is called by the default Render function with the
	 * default render pass already begun.
	 */
	virtual void
	Draw (VkCommandBuffer cmd);

	/**
	 * Clean up Vulkan objects.
	 *
	 * @note vkDeviceWaitIdle is called between this and any rendering
	 * operations.
	 */
	virtual void
	Cleanup (void);
};

extern Application *gApplication;

}
