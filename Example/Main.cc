/**
 * "Hello Triangle" application for LGE (Lightweight Game Engine).
 * Copyright (C) 2024  dbstream
 */
#define LGE_MODULE "Example"

#include <LGE/Application.h>
#include <LGE/Descriptor.h>
#include <LGE/GPUMemory.h>
#include <LGE/Init.h>
#include <LGE/Pipeline.h>
#include <LGE/VulkanFunctions.h>

#include <cmath>
#include <stdexcept>

#include "position_color.frag.txt"
#include "position_color.vert.txt"

static LGE::DescriptorSetLayout set_layout = nullptr;

class HelloTrianglePipeline : public LGE::Pipeline {
public:
	VkPipelineLayout m_layout;

	HelloTrianglePipeline (void)
		: LGE::Pipeline ()
	{
		VkDescriptorSetLayout set_layouts[1] = {
			LGE::GetVkDescriptorSetLayout (set_layout)
		};

		VkPipelineLayoutCreateInfo layout_ci {};
		layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_ci.setLayoutCount = 1;
		layout_ci.pSetLayouts = set_layouts;

		VkResult result = vkCreatePipelineLayout (LGE::gVkDevice,
			&layout_ci, nullptr, &m_layout);

		if (result != VK_SUCCESS)
			throw std::runtime_error ("vkCreatePipelineLayout failed");
	}

	~HelloTrianglePipeline (void)
	{
		vkDestroyPipelineLayout (LGE::gVkDevice, m_layout, nullptr);
	}

	virtual void
	Create (void) override
	{
		VkVertexInputBindingDescription vertex_input_bindings[2] {};
		vertex_input_bindings[0].binding = 0;
		vertex_input_bindings[0].stride = 8 * sizeof (float);
		vertex_input_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		vertex_input_bindings[1].binding = 1;
		vertex_input_bindings[1].stride = 2 * sizeof (float);
		vertex_input_bindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		VkVertexInputAttributeDescription vertex_input_attributes[3] {};
		vertex_input_attributes[0].location = 0;
		vertex_input_attributes[0].binding = 0;
		vertex_input_attributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertex_input_attributes[0].offset = 0;
		vertex_input_attributes[1].location = 1;
		vertex_input_attributes[1].binding = 0;
		vertex_input_attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertex_input_attributes[1].offset = 4 * sizeof (float);
		vertex_input_attributes[2].location = 2;
		vertex_input_attributes[2].binding = 1;
		vertex_input_attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
		vertex_input_attributes[2].offset = 0;

		VkPipelineVertexInputStateCreateInfo vertex_input_state {};
		vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_state.vertexBindingDescriptionCount = 2;
		vertex_input_state.pVertexBindingDescriptions = vertex_input_bindings;
		vertex_input_state.vertexAttributeDescriptionCount = 3;
		vertex_input_state.pVertexAttributeDescriptions = vertex_input_attributes;

		VkPipelineInputAssemblyStateCreateInfo input_assembly_state {};
		input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo viewport_state {};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterization_state {};
		rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterization_state.depthBiasConstantFactor = 0.0f;
		rasterization_state.depthBiasClamp = 0.0f;
		rasterization_state.depthBiasSlopeFactor = 1.0f;
		rasterization_state.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisample_state {};
		multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_state.minSampleShading = 1.0f;

		VkPipelineColorBlendAttachmentState color_blend_attachment_state {};
		color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment_state.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT
			| VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT
			| VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo color_blend_state {};
		color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_state.logicOp = VK_LOGIC_OP_COPY;
		color_blend_state.attachmentCount = 1;
		color_blend_state.pAttachments = &color_blend_attachment_state;

		VkDynamicState dynamic_state_list[2] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_state {};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.dynamicStateCount = 2;
		dynamic_state.pDynamicStates = dynamic_state_list;

		VkGraphicsPipelineCreateInfo pipeline_ci {};
		pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_ci.pVertexInputState = &vertex_input_state;
		pipeline_ci.pInputAssemblyState = &input_assembly_state;
		pipeline_ci.pViewportState = &viewport_state;
		pipeline_ci.pRasterizationState = &rasterization_state;
		pipeline_ci.pMultisampleState = &multisample_state;
		pipeline_ci.pColorBlendState = &color_blend_state;
		pipeline_ci.pDynamicState = &dynamic_state;
		pipeline_ci.layout = m_layout;
		pipeline_ci.renderPass = m_targetRenderPass;
		pipeline_ci.basePipelineIndex = -1;

		LGE::ShaderModuleInfo shader_module_info[2] {};
		shader_module_info[0].code = vertex_shader;
		shader_module_info[0].size = sizeof (vertex_shader);
		shader_module_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_module_info[1].code = fragment_shader;
		shader_module_info[1].size = sizeof (fragment_shader);
		shader_module_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		LGE::LinkShaderModules (&pipeline_ci, 2, shader_module_info);
		VkResult result = vkCreateGraphicsPipelines (LGE::gVkDevice,
			LGE::gPipelineCache, 1, &pipeline_ci, nullptr, &m_pipeline);
		LGE::FreeShaderModules (&pipeline_ci);

		if (result != VK_SUCCESS)
			throw std::runtime_error ("vkCreateGraphicsPipelines failed");
	}
};

static HelloTrianglePipeline *hello_triangle = nullptr;
static LGE::GPUBuffer hello_buffer;

static const float buffer_data[] = {
	0.0f,	-0.5f,	0.0f,	1.0f,	1.0f,	0.0f,	0.0f,	1.0f,
	0.5f,	0.5f,	0.0f,	1.0f,	0.0f,	1.0f,	0.0f,	1.0f,
	-0.5f,	0.5f,	0.0f,	1.0f,	0.0f,	0.0f,	1.0f,	1.0f
};

class ExampleApplication : public LGE::Application {
public:
	virtual const char *
	GetUserFriendlyName (void) override
	{
		return "Example";
	}

	virtual void
	Draw (VkCommandBuffer cmd) override
	{
		if (!set_layout) {
			VkDescriptorSetLayoutBinding bindings[1] {};
			bindings[0].binding = 0;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayoutCreateInfo ci {};
			ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			ci.bindingCount = 1;
			ci.pBindings = bindings;

			set_layout = LGE::GetDescriptorSetLayout (&ci);
		}

		if (!hello_triangle)
			hello_triangle = new HelloTrianglePipeline;

		if (!hello_buffer)
			hello_buffer = LGE::MMCreateMeshGPUBuffer (buffer_data,
				sizeof (buffer_data), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

		VkViewport viewport {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) m_extent.width;
		viewport.height = (float) m_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor {};
		scissor.extent = m_extent;

		VkDescriptorSet set = LGE::CreateTemporaryDescriptorSet (set_layout);

		float uniform_vars[1] = { 1.5f + 0.5f * std::sinf (vkfwGetTime () / 250000.0f) };
		VkDescriptorBufferInfo b {};
		b.buffer = LGE::MMCreateTemporaryGPUBuffer (uniform_vars, sizeof (uniform_vars),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		b.range = VK_WHOLE_SIZE;
		VkWriteDescriptorSet w {};
		w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		w.dstSet = set;
		w.dstBinding = 0;
		w.dstArrayElement = 0;
		w.descriptorCount = 1;
		w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		w.pBufferInfo = &b;
		vkUpdateDescriptorSets (LGE::gVkDevice, 1, &w, 0, nullptr);

		hello_triangle->Bind (cmd, VK_PIPELINE_BIND_POINT_GRAPHICS);
		vkCmdBindDescriptorSets (cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			hello_triangle->m_layout, 0, 1, &set, 0, nullptr);

		vkCmdSetViewport (cmd, 0, 1, &viewport);
		vkCmdSetScissor (cmd, 0, 1, &scissor);

		float instance_info[] = {
			-0.5f, 0.5f,
			0.5f, 0.5f,
			0.0f, -0.5f
		};

		VkDeviceSize offsets[2] = { 0, 0 };
		VkBuffer buffers[2] = {
			hello_buffer.m_buffer,
			LGE::MMCreateTemporaryGPUBuffer (instance_info,
				sizeof (instance_info), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
		};
		vkCmdBindVertexBuffers (cmd, 0, 2, buffers, offsets);
		vkCmdDraw (cmd, 3, sizeof (instance_info) / (2 * sizeof (float)), 0, 0);
	}

	virtual void
	Cleanup (void) override
	{
		if (hello_triangle) {
			delete hello_triangle;
			hello_triangle = nullptr;
		}

		if (hello_buffer)
			LGE::MMDestroyGPUBuffer (hello_buffer);

		LGE::Application::Cleanup ();
	}
};

int
main (int argc, const char *argv[])
{
	(void) argc;

	ExampleApplication app;
	return LGE::LGEMain (app, argv);
}
