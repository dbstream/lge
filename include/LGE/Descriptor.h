/**
 * Descriptor set management.
 * Copyright (C) 2024  dbstream
 */
#pragma once

/**
 * We provide two interfaces:
 *   Persistent descriptor sets. The application explicitly allocates and frees
 *   descriptor sets to us. However, we delay reusing the descriptor sets so
 *   that applications can free them without having to worry about frames that
 *   are in flight.
 *
 *   Transient descriptor sets. These are allocated by the application, then
 *   used during a single frame only. We automatically free them.
 *
 * We also manage VkSamplers.
 */

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>

namespace LGE {

typedef struct DescriptorSetLayout_T *DescriptorSetLayout;

/**
 * Initialize the descriptor manager.
 */
void
DescriptorInit (void);

/**
 * Terminate the descriptor manager.
 */
void
DescriptorTerminate (void);

/**
 * Tell the descriptor manager that the VkFence for rendering operations on a
 * frame has completed.
 */
void
DescriptorNextFrame (void);

/**
 * Create or retrieve a VkSampler.
 *
 * NOTE: lookup can be an expensive operation, so prefer caching samplers
 * in variables over depending on this to cache it for you.
 */
VkSampler
GetSampler (const VkSamplerCreateInfo *ci);

/**
 * Get the VkDescriptorSetLayout of a DescriptorSetLayout.
 */
VkDescriptorSetLayout
GetVkDescriptorSetLayout (DescriptorSetLayout l);

/**
 * Create or retrieve a DescriptorSetLayout (LGE-internal handle to a
 * VkDescriptorSetLayout that implements allocation, set re-use, etc.).
 *
 * NOTE: lookup is not implemented yet; thus, every time this is called,
 * a new DescriptorSetLayout is created. Since DescriptorSetLayouts are
 * not freed until DescriptorTerminate (the end of the program), not
 * storing these in variables leaks memory.
 */
DescriptorSetLayout
GetDescriptorSetLayout (const VkDescriptorSetLayoutCreateInfo *ci);

VkDescriptorSet
CreateDescriptorSet (DescriptorSetLayout l);

void
FreeDescriptorSet (DescriptorSetLayout l, VkDescriptorSet set);

VkDescriptorSet
CreateTemporaryDescriptorSet (DescriptorSetLayout l);

}
