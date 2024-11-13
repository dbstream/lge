/**
 * Drawing textual debug information.
 * Copyright (C) 2024  dbstream
 */
#define LGE_MODULE "LGEDebugUI"

#include <LGE/Application.h>
#include <LGE/DebugUI.h>
#include <LGE/Descriptor.h>
#include <LGE/GPUMemory.h>
#include <LGE/Log.h>
#include <LGE/Pipeline.h>
#include <LGE/VulkanFunctions.h>
#include <LGE/Window.h>

#include <stdarg.h>
#include <stdexcept>
#include <vector>

#include "DebugUIFont.cc"


namespace LGE {

#include "debugui.frag.txt"
#include "debugui.vert.txt"

static GPUImage font_image;
static VkImageView font_image_view;

static VkSampler sampler;
static DescriptorSetLayout set_layout;
static VkDescriptorSet descriptor_set;

class DebugUIPipeline : public Pipeline {
public:
	VkPipelineLayout m_layout;

	DebugUIPipeline (void)
		: Pipeline ()
	{
		VkDescriptorSetLayout layouts[1] = {
			GetVkDescriptorSetLayout (set_layout)
		};

		VkPipelineLayoutCreateInfo layout_ci {};
		layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_ci.setLayoutCount = 1;
		layout_ci.pSetLayouts = layouts;

		VkResult result = vkCreatePipelineLayout (gVkDevice,
			&layout_ci, nullptr, &m_layout);

		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkCreatePipelineLayout returned ") + VulkanTypeToString (result));
	}

	~DebugUIPipeline (void)
	{
		vkDestroyPipelineLayout (gVkDevice, m_layout, nullptr);
	}

	virtual void
	Create (void) override
	{
		VkVertexInputBindingDescription vertex_input_bindings[1] {};
		vertex_input_bindings[0].binding = 0;
		vertex_input_bindings[0].stride = 8 * sizeof (float);
		vertex_input_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription vertex_input_attributes[3] {};
		vertex_input_attributes[0].location = 0;
		vertex_input_attributes[0].binding = 0;
		vertex_input_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		vertex_input_attributes[0].offset = 0;
		vertex_input_attributes[1].location = 1;
		vertex_input_attributes[1].binding = 0;
		vertex_input_attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
		vertex_input_attributes[1].offset = 2 * sizeof (float);
		vertex_input_attributes[2].location = 2;
		vertex_input_attributes[2].binding = 0;
		vertex_input_attributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertex_input_attributes[2].offset = 4 * sizeof (float);

		VkPipelineVertexInputStateCreateInfo vertex_input_state {};
		vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_state.vertexBindingDescriptionCount = 1;
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
		color_blend_attachment_state.blendEnable = VK_TRUE;
		color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment_state.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT
			| VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT;

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
		pipeline_ci.subpass = gApplication->GetDebugUISubpass ();
		pipeline_ci.basePipelineIndex = -1;

		ShaderModuleInfo shader_module_info[2] {};
		shader_module_info[0].code = debugui_vert;
		shader_module_info[0].size = sizeof (debugui_vert);
		shader_module_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_module_info[1].code = debugui_frag;
		shader_module_info[1].size = sizeof (debugui_frag);
		shader_module_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		LinkShaderModules (&pipeline_ci, 2, shader_module_info);
		VkResult result = vkCreateGraphicsPipelines (gVkDevice,
			gPipelineCache, 1, &pipeline_ci, nullptr, &m_pipeline);
		FreeShaderModules (&pipeline_ci);

		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkCreateGraphicsPipelines returned ") + VulkanTypeToString (result));
	}
};

static DebugUIPipeline *pipeline = nullptr;

void
DebugUIInit (void)
{
	VkSamplerCreateInfo sampler_ci {};
	sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_ci.magFilter = VK_FILTER_NEAREST;
	sampler_ci.minFilter = VK_FILTER_NEAREST;
	sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_ci.mipLodBias = 0.0f;
	sampler_ci.anisotropyEnable = VK_FALSE;
	sampler_ci.compareEnable = VK_FALSE;
	sampler_ci.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_ci.minLod = 0.0f;
	sampler_ci.maxLod = 0.0f;
	sampler_ci.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_ci.unnormalizedCoordinates = VK_FALSE;
	sampler = GetSampler (&sampler_ci);

	VkDescriptorSetLayoutBinding bindings[1] {};
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo set_layout_ci {};
	set_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set_layout_ci.bindingCount = 1;
	set_layout_ci.pBindings = bindings;
	set_layout = GetDescriptorSetLayout (&set_layout_ci);

	VkExtent2D extent = {
		DebugUIFont::font_bitmap_size,
		DebugUIFont::font_bitmap_size
	};

	font_image = MMUploadTexture2D (VK_FORMAT_R8_UNORM, extent,
		DebugUIFont::font_bitmap);

	VkImageViewCreateInfo view_ci {};
	view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_ci.image = font_image.m_image;
	view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_ci.format = VK_FORMAT_R8_UNORM;
	view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_ci.subresourceRange.levelCount = 1;
	view_ci.subresourceRange.layerCount = 1;
	VkResult result = ::vkCreateImageView (gVkDevice, &view_ci, nullptr, &font_image_view);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkCreateImageView returned ") + VulkanTypeToString (result));

	try {
		descriptor_set = CreateDescriptorSet (set_layout);
	} catch (...) {
		::vkDestroyImageView (gVkDevice, font_image_view, nullptr);
		MMDestroyGPUImage (font_image);
		throw;
	}

	VkDescriptorImageInfo im {};
	im.sampler = sampler;
	im.imageView = font_image_view;
	im.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet wr {};
	wr.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	wr.dstSet = descriptor_set;
	wr.descriptorCount = 1;
	wr.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	wr.pImageInfo = &im;

	vkUpdateDescriptorSets (gVkDevice, 1, &wr, 0, nullptr);
}

struct DebugUIVertex {
	float x, y;
	float u, v;
	float r, g, b, a;
};

static std::vector<DebugUIVertex> vertices;

void
DebugUITerminate (void)
{
	if (pipeline) {
		delete pipeline;
		pipeline = nullptr;
	}

	::vkDestroyImageView (gVkDevice, font_image_view, nullptr);
	MMDestroyGPUImage (font_image);

	vertices.clear ();
}

static void
measure_text (const char *text, int &width, int &height)
{
	width = 0;
	height = 12;
	for (; *text; text++) {
		DebugUIFont::glyph glyph = DebugUIFont::glyphs[*text];
		if (!glyph.exists)
			continue;

		width += glyph.advance;
	}
}

/**
 * This is a very limited text drawing function. It currently only handles the
 * intersection of what the font supports and ASCII.
 */
void
DebugUIDrawText (const char *text, int x, int y, DebugUICorner corner,
	float r, float g, float b, float a)
{
	if (!*text)
		return;

	if (corner != DebugUICorner::TOP_LEFT) {
		int width, height;
		measure_text (text, width, height);

		switch (corner) {
		case DebugUICorner::TOP_RIGHT:
			x = gWindow->GetSwapchainExtent ().width - x - width;
			break;
		case DebugUICorner::BOTTOM_LEFT:
			y = gWindow->GetSwapchainExtent ().height - y - height;
			break;
		case DebugUICorner::BOTTOM_RIGHT:
			x = gWindow->GetSwapchainExtent ().width - x - width;
			y = gWindow->GetSwapchainExtent ().height - y - height;
			break;
		default:;
		}
	}

	float xscale = 1.0f / (float) gWindow->GetSwapchainExtent ().width;
	float yscale = 1.0f / (float) gWindow->GetSwapchainExtent ().height;
	float uvscale = 1.0f / (float) DebugUIFont::font_bitmap_size;

	float x_ = xscale * x, y_ = yscale * y;
	do {
		DebugUIFont::glyph glyph = DebugUIFont::glyphs[*text];
		if (glyph.width && glyph.height) {
			DebugUIVertex topleft, topright, bottomleft, bottomright;

			float left = x_ + xscale * glyph.xoffset;
			float top = y_ + yscale * (12 - glyph.yoffset);
			float right = left + xscale * glyph.width;
			float bottom = top + yscale * glyph.height;
			float uvleft = glyph.atlas_xoffset * uvscale;
			float uvtop = glyph.atlas_yoffset * uvscale;
			float uvright = uvleft + uvscale * glyph.width;
			float uvbottom = uvtop + uvscale * glyph.height;

			topleft.x = left;
			topleft.y = top;
			topleft.u = uvleft;
			topleft.v = uvtop;
			topleft.r = r;
			topleft.g = g;
			topleft.b = b;
			topleft.a = a;
			topright.x = right;
			topright.y = top;
			topright.u = uvright;
			topright.v = uvtop;
			topright.r = r;
			topright.g = g;
			topright.b = b;
			topright.a = a;
			bottomleft.x = left;
			bottomleft.y = bottom;
			bottomleft.u = uvleft;
			bottomleft.v = uvbottom;
			bottomleft.r = r;
			bottomleft.g = g;
			bottomleft.b = b;
			bottomleft.a = a;
			bottomright.x = right;
			bottomright.y = bottom;
			bottomright.u = uvright;
			bottomright.v = uvbottom;
			bottomright.r = r;
			bottomright.g = g;
			bottomright.b = b;
			bottomright.a = a;

			DebugUIVertex new_verts[6] = {
				topleft, topright, bottomleft,
				bottomleft, topright, bottomright
			};

			vertices.insert (vertices.end (), &new_verts[0], &new_verts[6]);
		}
		x_ += xscale * glyph.advance;
	} while (*(++text));
}

void
DebugUIPrintf (int x, int y, DebugUICorner corner,
	float r, float g, float b, float a,
	const char *fmt, ...)
{
	char buf[128];
	va_list args;
	va_start (args, fmt);
	vsnprintf (buf, sizeof (buf), fmt, args);
	va_end (args);

	buf[sizeof (buf) - 1] = 0;
	DebugUIDrawText (buf, x, y, corner, r, g, b, a);
}

void
DebugUIDraw (VkCommandBuffer cmd)
{
	if (vertices.empty ())
		return;

	size_t vertex_count = vertices.size ();
	VkBuffer vbuf = MMCreateTemporaryGPUBuffer (
		vertices.data (), vertex_count * sizeof (DebugUIVertex),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	vertices.clear ();

	if (!pipeline)
		pipeline = new DebugUIPipeline;
	pipeline->Bind (cmd, VK_PIPELINE_BIND_POINT_GRAPHICS);

	vkCmdBindDescriptorSets (cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipeline->m_layout, 0, 1, &descriptor_set, 0, nullptr);

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers (cmd, 0, 1, &vbuf, offsets);
	vkCmdDraw (cmd, vertex_count, 1, 0, 0);
}

}
