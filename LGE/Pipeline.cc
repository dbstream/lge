/**
 * Pipeline helpers.
 * Copyright (C) 2024  dbstream
 */
#define LGE_MODULE "LGEPipeline"

#include <LGE/Application.h>
#include <LGE/Pipeline.h>
#include <LGE/Vulkan.h>
#include <LGE/Window.h>

#include <VKFW/vkfw.h>

#include <stdexcept>
#include <string>

namespace LGE {

VkPipelineCache gPipelineCache = VK_NULL_HANDLE;

/**
 * Currently we build on the assumption that pipeline destruction only happens
 * on swapchain recreate and at program exit, hence the lack of vkWaitDevice.
 * If this assumption is broken, this code will be faulty and it is our
 * responsibility to fix it.
 */

Pipeline::Pipeline (void) {}

Pipeline::~Pipeline (void)
{
	if (m_pipeline != VK_NULL_HANDLE) {
		::vkDestroyPipeline (gVkDevice, m_pipeline, nullptr);
		m_pipeline = VK_NULL_HANDLE;
	}
}

void
Pipeline::Create (void)
{
	throw std::runtime_error ("Pipeline::Create is not implemented");
}

void
Pipeline::Bind (VkCommandBuffer cmd, VkPipelineBindPoint bind_point)
{
	VkRenderPass rp = gApplication->GetRenderPass ();
	if (m_pipeline == VK_NULL_HANDLE || rp != m_targetRenderPass) {
		if (m_pipeline != VK_NULL_HANDLE) {
			::vkDestroyPipeline (gVkDevice, m_pipeline, nullptr);
			m_pipeline = VK_NULL_HANDLE;
		}

		m_targetRenderPass = rp;
		this->Create ();
	}

	::vkCmdBindPipeline (cmd, bind_point, m_pipeline);
}

// TODO: make use of maintenance5 in LinkShaderModules and FreeShaderModules

void
LinkShaderModules (VkGraphicsPipelineCreateInfo *pipeline_ci,
	uint32_t count, const ShaderModuleInfo *shaders)
{
	pipeline_ci->stageCount = 0;
	pipeline_ci->pStages = nullptr;

	if (!count)
		return;

	VkPipelineShaderStageCreateInfo *stages = new VkPipelineShaderStageCreateInfo[count];

	for (uint32_t i = 0; i < count; i++) {
		VkShaderModuleCreateInfo shader_module_ci {};
		shader_module_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_module_ci.codeSize = shaders[i].size;
		shader_module_ci.pCode = shaders[i].code;

		VkResult result = ::vkCreateShaderModule (gVkDevice,
			&shader_module_ci, nullptr, &stages[i].module);
		if (result != VK_SUCCESS) {
			while (i)
				::vkDestroyShaderModule (gVkDevice, stages[--i].module, nullptr);
			delete[] stages;
			throw std::runtime_error (std::string ("vkCreateShaderModule returned ") + VulkanTypeToString (result));
		}

		stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[i].pNext = nullptr;
		stages[i].flags = 0;
		stages[i].stage = shaders[i].stage;
		stages[i].pName = "main";
		stages[i].pSpecializationInfo = nullptr;
	}

	pipeline_ci->stageCount = count;
	pipeline_ci->pStages = stages;
}

void
FreeShaderModules (VkGraphicsPipelineCreateInfo *pipeline_ci)
{
	if (!pipeline_ci->stageCount)
		return;

	for (uint32_t i = 0; i < pipeline_ci->stageCount; i++)
		::vkDestroyShaderModule (LGE::gVkDevice,
			pipeline_ci->pStages[i].module, nullptr);

	delete[] pipeline_ci->pStages;
	pipeline_ci->stageCount = 0;
	pipeline_ci->pStages = nullptr;
}

}
