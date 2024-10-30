/**
 * GPU descriptor set allocation and management.
 * Copyright (C) 2024  dbstream
 */
#define LGE_MODULE "LGEDescriptor"

#include <LGE/Application.h>
#include <LGE/Descriptor.h>
#include <LGE/Log.h>
#include <LGE/VulkanFunctions.h>

#include <map>
#include <stdexcept>
#include <vector>

namespace LGE {

void
DescriptorInit (void)
{
	// nothing to do here
}

struct SamplerCreateInfoComparator {
	constexpr bool
	operator() (const VkSamplerCreateInfo &a, const VkSamplerCreateInfo &b) const
	{
		if (a.flags < b.flags)						return true;
		if (a.magFilter < b.magFilter)					return true;
		if (a.minFilter < b.minFilter)					return true;
		if (a.mipmapMode < b.mipmapMode)				return true;
		if (a.addressModeU < b.addressModeU)				return true;
		if (a.addressModeV < b.addressModeV)				return true;
		if (a.addressModeW < b.addressModeW)				return true;
		if (a.mipLodBias < b.mipLodBias)				return true;
		if (a.anisotropyEnable < b.anisotropyEnable)			return true;
		if (a.maxAnisotropy < b.maxAnisotropy)				return true;
		if (a.compareEnable < b.compareEnable)				return true;
		if (a.compareOp < b.compareOp)					return true;
		if (a.minLod < b.minLod)					return true;
		if (a.maxLod < b.maxLod)					return true;
		if (a.borderColor < b.borderColor)				return true;
		if (a.unnormalizedCoordinates < b.unnormalizedCoordinates)	return true;
		return false;
	}
};

typedef std::map<VkSamplerCreateInfo, VkSampler, SamplerCreateInfoComparator> SamplerMap;
static SamplerMap samplers;

VkSampler
GetSampler (const VkSamplerCreateInfo *ci)
{
	if (ci->sType != VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO || ci->pNext != nullptr)
		throw std::runtime_error ("Unsupported VkSamplerCreateInfo");

	SamplerMap::iterator it = samplers.find (*ci);
	if (it != samplers.end ())
		return it->second;

	VkSampler sampler;
	VkResult result = ::vkCreateSampler (gVkDevice, ci, nullptr, &sampler);
	if (result != VK_SUCCESS)
		throw std::runtime_error (std::string ("vkCreateSampler returned ") + VulkanTypeToString (result));

	try {
		samplers[*ci] = sampler;
	} catch (...) {
		::vkDestroySampler (gVkDevice, sampler, nullptr);
		throw;
	}

	return sampler;
}

static constexpr int DESCRIPTOR_SETS_PER_POOL = 32;

static constexpr int NUM_SUPPORTED_DESCRIPTOR_TYPES = 11;
static constexpr VkDescriptorType pool_descriptor_types[NUM_SUPPORTED_DESCRIPTOR_TYPES] = {
	VK_DESCRIPTOR_TYPE_SAMPLER,
	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
	VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
	VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
	VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
};

struct DescriptorSetLayout_T {
	VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_set_freelist;
	std::vector<VkDescriptorSet> m_stash[CPU_RENDER_AHEAD];
	std::vector<VkDescriptorPool> m_old_pools;
	uint32_t m_pool_sizes[NUM_SUPPORTED_DESCRIPTOR_TYPES] = { 0 };
	int m_stash_index = 0;
};

VkDescriptorSetLayout
GetVkDescriptorSetLayout (DescriptorSetLayout l)
{
	return l->m_layout;
}

static std::vector<DescriptorSetLayout> layouts;

DescriptorSetLayout
GetDescriptorSetLayout (const VkDescriptorSetLayoutCreateInfo *ci)
{
	if (ci->pNext)
		throw std::runtime_error ("unsupported VkDescriptorSetLayoutCreateInfo");

	DescriptorSetLayout l = new DescriptorSetLayout_T;
	for (uint32_t i = 0; i < ci->bindingCount; i++) {
		bool found = false;
		for (int j = 0; j < NUM_SUPPORTED_DESCRIPTOR_TYPES; j++) {
			if (ci->pBindings[i].descriptorType == pool_descriptor_types[j]) {
				found = true;
				l->m_pool_sizes[j] += ci->pBindings[i].descriptorCount;
				break;
			}
		}

		if (!found) {
			delete l;
			throw std::runtime_error ("unsupported VkDescriptorSetLayoutCreateInfo");
		}
	}

	try {
		layouts.push_back (l);
	} catch (...) {
		delete l;
		throw;
	}

	VkResult result = ::vkCreateDescriptorSetLayout (gVkDevice, ci, nullptr, &l->m_layout);
	if (result != VK_SUCCESS) {
		layouts.pop_back ();
		delete l;
		throw std::runtime_error (std::string ("vkCreateDescriptorSetLayout returned ") + VulkanTypeToString (result));
	}

	return l;
}

static void
destroy_layout (DescriptorSetLayout l)
{
	for (VkDescriptorPool p : l->m_old_pools)
		::vkDestroyDescriptorPool (gVkDevice, p, nullptr);

	::vkDestroyDescriptorSetLayout (gVkDevice, l->m_layout, nullptr);
	delete l;
}

void
DescriptorTerminate (void)
{
	for (DescriptorSetLayout l : layouts)
		destroy_layout (l);

	layouts.clear ();

	for (std::pair<VkSamplerCreateInfo, VkSampler> it : samplers)
		::vkDestroySampler (gVkDevice, it.second, nullptr);

	samplers.clear ();
}

VkDescriptorSet
CreateDescriptorSet (DescriptorSetLayout l)
{
	if (l->m_set_freelist.empty ()) {
		VkDescriptorPoolCreateInfo ci {};
		ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		ci.maxSets = DESCRIPTOR_SETS_PER_POOL;

		VkDescriptorPoolSize pool_sizes[NUM_SUPPORTED_DESCRIPTOR_TYPES];
		ci.pPoolSizes = pool_sizes;

		for (int i = 0; i < NUM_SUPPORTED_DESCRIPTOR_TYPES; i++) {
			if (!l->m_pool_sizes[i])
				continue;

			pool_sizes[ci.poolSizeCount++] = {
				pool_descriptor_types[i],
				DESCRIPTOR_SETS_PER_POOL * l->m_pool_sizes[i]
			};
		}

		VkDescriptorPool pool;
		VkResult result = ::vkCreateDescriptorPool (gVkDevice, &ci, nullptr, &pool);
		if (result != VK_SUCCESS)
			throw std::runtime_error (std::string ("vkCreateDescriptorPool returned ") + VulkanTypeToString (result));

		try {
			l->m_old_pools.push_back (pool);
		} catch (...) {
			::vkDestroyDescriptorPool (gVkDevice, pool, nullptr);
			throw;
		}

		try {
			VkDescriptorSet sets[DESCRIPTOR_SETS_PER_POOL];
			VkDescriptorSetLayout set_layouts[DESCRIPTOR_SETS_PER_POOL];
			for (int i = 0; i < DESCRIPTOR_SETS_PER_POOL; i++)
				set_layouts[i] = l->m_layout;

			VkDescriptorSetAllocateInfo ai {};
			ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			ai.descriptorPool = pool;
			ai.descriptorSetCount = DESCRIPTOR_SETS_PER_POOL;
			ai.pSetLayouts = set_layouts;
			result = ::vkAllocateDescriptorSets (gVkDevice, &ai, sets);
			if (result != VK_SUCCESS)
				throw std::runtime_error (std::string ("vkAllocateDescriptorSets returned ") + VulkanTypeToString (result));

			l->m_set_freelist.insert (l->m_set_freelist.end(),
				&sets[0], &sets[DESCRIPTOR_SETS_PER_POOL]);
		} catch (...) {
			::vkDestroyDescriptorPool (gVkDevice, pool, nullptr);
			l->m_old_pools.pop_back ();
			throw;
		}
	}

	VkDescriptorSet set = l->m_set_freelist.back ();
	l->m_set_freelist.pop_back ();
	return set;
}

void
FreeDescriptorSet (DescriptorSetLayout l, VkDescriptorSet set)
{
	l->m_stash[l->m_stash_index].push_back (set);
}

VkDescriptorSet
CreateTemporaryDescriptorSet (DescriptorSetLayout l)
{
	VkDescriptorSet set = CreateDescriptorSet (l);
	FreeDescriptorSet (l, set);
	return set;
}

void
DescriptorNextFrame (void)
{
	for (DescriptorSetLayout l : layouts) {
		l->m_stash_index++;
		if (l->m_stash_index >= CPU_RENDER_AHEAD)
			l->m_stash_index = 0;

		l->m_set_freelist.insert (l->m_set_freelist.end (),
			l->m_stash[l->m_stash_index].begin (),
			l->m_stash[l->m_stash_index].end ());
		l->m_stash[l->m_stash_index].clear ();
	}
}

}
