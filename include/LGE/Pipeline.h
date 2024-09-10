/**
 * Pipeline helpers.
 * Copyright (C) 2024  dbstream
 */
#pragma once

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>

namespace LGE {

extern VkPipelineCache gPipelineCache;

class Pipeline {
protected:
	VkPipeline m_pipeline = VK_NULL_HANDLE;
	VkRenderPass m_targetRenderPass;

	/**
	 * Pipeline constructor.
	 *
	 * The utility of this is to make direct Pipeline construction
	 * impossible, as Pipeline must be subclassed with an implementation
	 * of Create.
	 */
	Pipeline (void);

	/**
	 * Create the VkPipeline object.
	 *
	 * This function is called by the implementation of Bind to recreate
	 * the pipeline object when necessary, for example on swapchain format
	 * change.
	 */
	virtual void
	Create (void);
public:
	virtual ~Pipeline (void);

	/**
	 * Bind the pipeline.
	 *
	 * @param cmd command buffer.
	 * @param bind_point bind point.
	 */
	void
	Bind (VkCommandBuffer cmd, VkPipelineBindPoint bind_point);
};

/**
 * The purpose of LinkShaderModules and FreeShaderModules is
 * 1) To make life easier for developers by reducing the number of different
 * types of objects to keep track of by one.
 * 2) To be able to transparently take advantage of features such as
 * maintenance5 when available.
 */

struct ShaderModuleInfo {
	const uint32_t *code;
	size_t size;
	VkShaderStageFlagBits stage;
};

/**
 * Store the specified shaders in the pipeline create info.
 *
 * @note FreeShaderModules must be called with the same pipeline create info,
 * otherwise VkShaderModule objects will be leaked.
 *
 * @param pipeline_ci pipeline create info.
 * @param count number of shaders.
 * @param shaders pointer to a list of structures describing shaders to include.
 */
void
LinkShaderModules (VkGraphicsPipelineCreateInfo *pipeline_ci,
	uint32_t count, const ShaderModuleInfo *shaders);

/**
 * Free the shader modules associated with a graphics pipeline created by
 * LinkShaderModules.
 */
void
FreeShaderModules (VkGraphicsPipelineCreateInfo *pipeline_ci);

}
