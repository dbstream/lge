/**
 * Application defaults.
 * Copyright (C) 2024  dbstream
 */
#define LGE_MODULE "LGEApplication"

#include <LGE/Application.h>
#include <LGE/Log.h>
#include <LGE/Vulkan.h>
#include <LGE/Window.h>

#include <VKFW/vkfw.h>

#include <stdexcept>
#include <string>

namespace LGE {

Application *gApplication;

Application::~Application (void) {}
Application::Application (void) {}

const char *Application::GetUserFriendlyName (void)
{
	return "<UserFriendlyName>";
}

void
Application::CheckRequirements (void)
{
	/**
	 * We don't need to check for Vulkan 1.2 before testing gVkFeatures12.
	 * Core feature structs not supported by the device are cleared by us.
	 */

	if (!gVkFeatures12.imagelessFramebuffer)
		throw std::runtime_error ("Vulkan12Features::imagelessFramebuffer is not supported by the device");
}

bool
Application::KeepRunning (void)
{
	return m_keep_running;
}

void
Application::HandleEvent (const VKFWevent &e)
{
	if (e.type == VKFW_EVENT_WINDOW_CLOSE_REQUEST) {
		Log ("Exiting due to WINDOW_CLOSE_REQUEST");
		m_keep_running = false;
	}
}

void
Application::Render (void)
{
	VkResult result;

	if (m_commandPool == VK_NULL_HANDLE) {
		VkCommandPoolCreateInfo pool_ci {};
		pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_ci.queueFamilyIndex = gVkQueueFamily;

		result = ::vkCreateCommandPool (gVkDevice, &pool_ci, nullptr, &m_commandPool);
		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkCreateCommandPool returned ") + VulkanTypeToString (result));

		VkCommandBufferAllocateInfo cmd_ai {};
		cmd_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmd_ai.commandPool = m_commandPool;
		cmd_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmd_ai.commandBufferCount = CPU_RENDER_AHEAD;
		result = ::vkAllocateCommandBuffers (gVkDevice, &cmd_ai, m_commandBuffers);
		if (result != VK_SUCCESS) {
			::vkDestroyCommandPool (gVkDevice, m_commandPool, nullptr);
			m_commandPool = VK_NULL_HANDLE;
			throw std::runtime_error (std::string ("vkAllocateCommandBuffers returned ") + VulkanTypeToString (result));
		}

		VkFenceCreateInfo fence_ci {};
		fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (size_t i = 0; i < CPU_RENDER_AHEAD; i++) {
			result = ::vkCreateFence (gVkDevice, &fence_ci, nullptr, &m_fences[i]);
			if (result != VK_SUCCESS) {
				while (i)
					::vkDestroyFence (gVkDevice, m_fences[--i], nullptr);
				::vkDestroyCommandPool (gVkDevice, m_commandPool, nullptr);
				m_commandPool = VK_NULL_HANDLE;
				throw std::runtime_error (std::string ("vkCreateFence returned ") + VulkanTypeToString (result));
			}
		}

		VkSemaphoreCreateInfo sema_ci {};
		sema_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for (size_t i = 0; i < 2 * CPU_RENDER_AHEAD; i++) {
			result = ::vkCreateSemaphore (gVkDevice, &sema_ci, nullptr, &m_semaphores[i]);
			if (result != VK_SUCCESS) {
				while (i)
					::vkDestroySemaphore (gVkDevice, m_semaphores[--i], nullptr);
				for (i = CPU_RENDER_AHEAD; i; )
					::vkDestroyFence (gVkDevice, m_fences[--i], nullptr);
				::vkDestroyCommandPool (gVkDevice, m_commandPool, nullptr);
				m_commandPool = VK_NULL_HANDLE;
			}
		}
	}

	VkCommandBuffer cmd = m_commandBuffers[m_frameIndex];
	VkFence fence = m_fences[m_frameIndex];
	VkSemaphore sema0 = m_semaphores[2 * m_frameIndex + 0];
	VkSemaphore sema1 = m_semaphores[2 * m_frameIndex + 1];

	result = ::vkWaitForFences (gVkDevice, 1, &fence, VK_TRUE, UINT64_MAX);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkWaitForFences returned ") + VulkanTypeToString (result));

	uint32_t swapchain_index;
	if (!gWindow->AcquireSwapchainImage (&swapchain_index, sema0))
		return;

	if (m_renderPass == VK_NULL_HANDLE || m_format != gWindow->GetSwapchainFormat ()) {
		if (m_renderPass != VK_NULL_HANDLE) {
			::vkDestroyRenderPass (gVkDevice, m_renderPass, nullptr);
			m_renderPass = VK_NULL_HANDLE;
		}

		if (m_framebuffer != VK_NULL_HANDLE) {
			::vkDestroyFramebuffer (gVkDevice, m_framebuffer, nullptr);
			m_framebuffer = VK_NULL_HANDLE;
		}

		m_format = gWindow->GetSwapchainFormat ();
		m_renderPass = this->CreateRenderPass ();
	}

	VkExtent2D extent = gWindow->GetSwapchainExtent ();
	if (m_framebuffer == VK_NULL_HANDLE || m_extent.width != extent.width || m_extent.height != extent.height) {
		if (m_framebuffer != VK_NULL_HANDLE) {
			::vkDestroyFramebuffer (gVkDevice, m_framebuffer, nullptr);
			m_framebuffer = VK_NULL_HANDLE;
		}

		m_extent = extent;
		m_framebuffer = this->CreateFramebuffer (m_renderPass);
	}

	result = ::vkResetCommandBuffer (cmd, 0);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkResetCommandBuffer returned ") + VulkanTypeToString (result));

	VkCommandBufferBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	result = ::vkBeginCommandBuffer (cmd, &begin_info);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkBeginCommandBuffer returned ") + VulkanTypeToString (result));

	this->BeginRendering (cmd, m_renderPass, m_framebuffer, gWindow->GetImageView (swapchain_index));
	this->Draw (cmd);
	::vkCmdEndRenderPass (cmd);

	result = ::vkEndCommandBuffer (cmd);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkEndCommandBuffer returned ") + VulkanTypeToString (result));

	result = ::vkResetFences (gVkDevice, 1, &fence);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkResetFences returned ") + VulkanTypeToString (result));

	VkPipelineStageFlags wait_psf = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit_info {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &sema0;
	submit_info.pWaitDstStageMask = &wait_psf;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &sema1;
	result = ::vkQueueSubmit (gVkQueue, 1, &submit_info, fence);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkQueueSubmit returned ") + VulkanTypeToString (result));

	gWindow->PresentSwapchainImage (swapchain_index, sema1);
	m_frameIndex++;
	if (m_frameIndex >= CPU_RENDER_AHEAD)
		m_frameIndex = 0;
}

VkRenderPass
Application::CreateRenderPass (void)
{
	VkAttachmentDescription ad {};
	ad.format = m_format;
	ad.samples = VK_SAMPLE_COUNT_1_BIT;
	ad.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	ad.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ad.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ad.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	ad.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ad.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference ar {};
	ar.attachment = 0;
	ar.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription sd {};
	sd.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sd.colorAttachmentCount = 1;
	sd.pColorAttachments = &ar;

	VkRenderPassCreateInfo rp_ci {};
	rp_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rp_ci.attachmentCount = 1;
	rp_ci.pAttachments = &ad;
	rp_ci.subpassCount = 1;
	rp_ci.pSubpasses = &sd;

	VkRenderPass rp;
	VkResult result = ::vkCreateRenderPass (gVkDevice, &rp_ci, nullptr, &rp);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkCreateRenderPass returned ") + VulkanTypeToString (result));

	return rp;
}

VkFramebuffer
Application::CreateFramebuffer (VkRenderPass rp)
{
	VkFramebufferAttachmentImageInfo aii {};
	aii.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO;
	aii.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	aii.width = m_extent.width;
	aii.height = m_extent.height;
	aii.layerCount = 1;
	aii.viewFormatCount = 1;
	aii.pViewFormats = &m_format;

	VkFramebufferAttachmentsCreateInfo a_ci {};
	a_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
	a_ci.attachmentImageInfoCount = 1;
	a_ci.pAttachmentImageInfos = &aii;

	VkFramebufferCreateInfo fb_ci {};
	fb_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_ci.pNext = &a_ci;
	fb_ci.flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
	fb_ci.renderPass = rp;
	fb_ci.attachmentCount = 1;
	fb_ci.width = m_extent.width;
	fb_ci.height = m_extent.height;
	fb_ci.layers = 1;

	VkFramebuffer fb;
	VkResult result = ::vkCreateFramebuffer (gVkDevice, &fb_ci, nullptr, &fb);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkCreateFramebuffer returned ") + VulkanTypeToString (result));

	return fb;
}

void
Application::BeginRendering (VkCommandBuffer cmd, VkRenderPass rp, VkFramebuffer fb, VkImageView target)
{
	VkRenderPassAttachmentBeginInfo rp_ai {};
	rp_ai.sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO;
	rp_ai.attachmentCount = 1;
	rp_ai.pAttachments = &target;

	VkClearValue cv {};
	cv.color.float32[0] = 0.0f;
	cv.color.float32[1] = 0.0f;
	cv.color.float32[2] = 0.0f;
	cv.color.float32[3] = 1.0f;

	VkRenderPassBeginInfo begin_info {};
	begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	begin_info.pNext = &rp_ai;
	begin_info.renderPass = rp;
	begin_info.framebuffer = fb;
	begin_info.renderArea.extent = m_extent;
	begin_info.clearValueCount = 1;
	begin_info.pClearValues = &cv;
	::vkCmdBeginRenderPass (cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void
Application::Draw (VkCommandBuffer cmd)
{}

void
Application::Cleanup (void)
{
	if (m_framebuffer != VK_NULL_HANDLE) {
		::vkDestroyFramebuffer (gVkDevice, m_framebuffer, nullptr);
		m_framebuffer = VK_NULL_HANDLE;
	}

	if (m_renderPass != VK_NULL_HANDLE) {
		::vkDestroyRenderPass (gVkDevice, m_renderPass, nullptr);
		m_renderPass = VK_NULL_HANDLE;
	}

	if (m_commandPool != VK_NULL_HANDLE) {
		::vkDestroyCommandPool (gVkDevice, m_commandPool, nullptr);
		m_commandPool = VK_NULL_HANDLE;

		for (size_t i = 0; i < CPU_RENDER_AHEAD; i++) {
			::vkDestroyFence (gVkDevice, m_fences[i], nullptr);
			::vkDestroySemaphore (gVkDevice, m_semaphores[2 * i + 0], nullptr);
			::vkDestroySemaphore (gVkDevice, m_semaphores[2 * i + 1], nullptr);
		}
	}
}

}
