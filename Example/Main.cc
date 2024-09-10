/**
 * "Hello Triangle" application for LGE (Lightweight Game Engine).
 * Copyright (C) 2024  dbstream
 */
#define LGE_MODULE "Example"

#include <LGE/Application.h>
#include <LGE/Init.h>
#include <LGE/Pipeline.h>
#include <LGE/VulkanFunctions.h>

#include <stdexcept>

/**
 * Fragment shader source:
 *	layout (location = 0) in vec4 color_;
 *	layout (location = 0) out vec4 color;
 *
 *	void
 *	main (void)
 *	{
 *		color = color_;
 *	}
 *
 * Compilation: glslang -V100 --glsl-version 460 -Os -g0
 */
static const uint32_t fragment_shader[] = {
	0x07230203,0x00010000,0x0008000b,0x0000000d,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000b,0x00030010,
	0x00000004,0x00000007,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000b,
	0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,
	0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,
	0x00000003,0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040020,0x0000000a,
	0x00000001,0x00000007,0x0004003b,0x0000000a,0x0000000b,0x00000001,0x00050036,0x00000002,
	0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003d,0x00000007,0x0000000c,
	0x0000000b,0x0003003e,0x00000009,0x0000000c,0x000100fd,0x00010038
};

/**
 * Vertex shader source:
 *	layout (location = 0) out vec4 color;
 *
 *	vec4 positions[3] = {
 *		vec4 (0.0, -0.5, 0.0, 1.0),
 *		vec4 (0.5, 0.5, 0.0, 1.0),
 *		vec4 (-0.5, 0.5, 0.0, 1.0)
 *	};
 *
 *	vec4 colors[3] = {
 *		vec4 (1.0, 0.0, 0.0, 1.0),
 *		vec4 (0.0, 1.0, 0.0, 1.0),
 *		vec4 (0.0, 0.0, 1.0, 1.0)
 *	};
 *
 *	void
 *	main (void)
 *	{
 *		gl_Position = positions[gl_VertexIndex];
 *		color = colors[gl_VertexIndex];
 *	}
 *
 * Compilation: glslang -V100 --glsl-version 460 -Os -g0
 */
static const uint32_t vertex_shader[] = {
	0x07230203,0x00010000,0x0008000b,0x0000002d,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0008000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000001e,0x00000022,0x00000029,
	0x00050048,0x0000001c,0x00000000,0x0000000b,0x00000000,0x00050048,0x0000001c,0x00000001,
	0x0000000b,0x00000001,0x00050048,0x0000001c,0x00000002,0x0000000b,0x00000003,0x00050048,
	0x0000001c,0x00000003,0x0000000b,0x00000004,0x00030047,0x0000001c,0x00000002,0x00040047,
	0x00000022,0x0000000b,0x0000002a,0x00040047,0x00000029,0x0000001e,0x00000000,0x00020013,
	0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,
	0x00000007,0x00000006,0x00000004,0x00040015,0x00000008,0x00000020,0x00000000,0x0004002b,
	0x00000008,0x00000009,0x00000003,0x0004001c,0x0000000a,0x00000007,0x00000009,0x00040020,
	0x0000000b,0x00000006,0x0000000a,0x0004003b,0x0000000b,0x0000000c,0x00000006,0x0004002b,
	0x00000006,0x0000000d,0x00000000,0x0004002b,0x00000006,0x0000000e,0xbf000000,0x0004002b,
	0x00000006,0x0000000f,0x3f800000,0x0007002c,0x00000007,0x00000010,0x0000000d,0x0000000e,
	0x0000000d,0x0000000f,0x0004002b,0x00000006,0x00000011,0x3f000000,0x0007002c,0x00000007,
	0x00000012,0x00000011,0x00000011,0x0000000d,0x0000000f,0x0007002c,0x00000007,0x00000013,
	0x0000000e,0x00000011,0x0000000d,0x0000000f,0x0006002c,0x0000000a,0x00000014,0x00000010,
	0x00000012,0x00000013,0x0004003b,0x0000000b,0x00000015,0x00000006,0x0007002c,0x00000007,
	0x00000016,0x0000000f,0x0000000d,0x0000000d,0x0000000f,0x0007002c,0x00000007,0x00000017,
	0x0000000d,0x0000000f,0x0000000d,0x0000000f,0x0007002c,0x00000007,0x00000018,0x0000000d,
	0x0000000d,0x0000000f,0x0000000f,0x0006002c,0x0000000a,0x00000019,0x00000016,0x00000017,
	0x00000018,0x0004002b,0x00000008,0x0000001a,0x00000001,0x0004001c,0x0000001b,0x00000006,
	0x0000001a,0x0006001e,0x0000001c,0x00000007,0x00000006,0x0000001b,0x0000001b,0x00040020,
	0x0000001d,0x00000003,0x0000001c,0x0004003b,0x0000001d,0x0000001e,0x00000003,0x00040015,
	0x0000001f,0x00000020,0x00000001,0x0004002b,0x0000001f,0x00000020,0x00000000,0x00040020,
	0x00000021,0x00000001,0x0000001f,0x0004003b,0x00000021,0x00000022,0x00000001,0x00040020,
	0x00000024,0x00000006,0x00000007,0x00040020,0x00000027,0x00000003,0x00000007,0x0004003b,
	0x00000027,0x00000029,0x00000003,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,
	0x000200f8,0x00000005,0x0003003e,0x0000000c,0x00000014,0x0003003e,0x00000015,0x00000019,
	0x0004003d,0x0000001f,0x00000023,0x00000022,0x00050041,0x00000024,0x00000025,0x0000000c,
	0x00000023,0x0004003d,0x00000007,0x00000026,0x00000025,0x00050041,0x00000027,0x00000028,
	0x0000001e,0x00000020,0x0003003e,0x00000028,0x00000026,0x00050041,0x00000024,0x0000002b,
	0x00000015,0x00000023,0x0004003d,0x00000007,0x0000002c,0x0000002b,0x0003003e,0x00000029,
	0x0000002c,0x000100fd,0x00010038
};

class HelloTrianglePipeline : public LGE::Pipeline {
	VkPipelineLayout m_layout;
public:
	HelloTrianglePipeline (void)
		: LGE::Pipeline ()
	{
		VkPipelineLayoutCreateInfo layout_ci {};
		layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

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
		VkPipelineVertexInputStateCreateInfo vertex_input_state {};
		vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

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
		if (!hello_triangle)
			hello_triangle = new HelloTrianglePipeline;

		VkViewport viewport {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) m_extent.width;
		viewport.height = (float) m_extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor {};
		scissor.extent = m_extent;

		hello_triangle->Bind (cmd, VK_PIPELINE_BIND_POINT_GRAPHICS);
		vkCmdSetViewport (cmd, 0, 1, &viewport);
		vkCmdSetScissor (cmd, 0, 1, &scissor);
		vkCmdDraw (cmd, 3, 1, 0, 0);
	}

	virtual void
	Cleanup (void) override
	{
		if (hello_triangle) {
			delete hello_triangle;
			hello_triangle = nullptr;
		}

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
