/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#include "vkr_descriptor_set.h"

#include "vkr_descriptor_set_gen.h"

static void
vkr_dispatch_vkGetDescriptorSetLayoutSupport(
   UNUSED struct vn_dispatch_context *dispatch,
   struct vn_command_vkGetDescriptorSetLayoutSupport *args)
{
   vn_replace_vkGetDescriptorSetLayoutSupport_args_handle(args);
   vkGetDescriptorSetLayoutSupport(args->device, args->pCreateInfo, args->pSupport);
}

static void
vkr_dispatch_vkCreateDescriptorSetLayout(
   struct vn_dispatch_context *dispatch,
   struct vn_command_vkCreateDescriptorSetLayout *args)
{
   struct vkr_context *ctx = dispatch->data;

   CREATE_OBJECT(layout, descriptor_set_layout, DESCRIPTOR_SET_LAYOUT,
                 vkCreateDescriptorSetLayout, pSetLayout);

   vkr_device_add_object(ctx, &layout->base);
}

static void
vkr_dispatch_vkDestroyDescriptorSetLayout(
   struct vn_dispatch_context *dispatch,
   struct vn_command_vkDestroyDescriptorSetLayout *args)
{
   struct vkr_context *ctx = dispatch->data;

   DESTROY_OBJECT(layout, descriptor_set_layout, DESCRIPTOR_SET_LAYOUT,
                  vkDestroyDescriptorSetLayout, descriptorSetLayout);

   vkr_device_remove_object(ctx, &layout->base);
}

static void
vkr_dispatch_vkCreateDescriptorPool(struct vn_dispatch_context *dispatch,
                                    struct vn_command_vkCreateDescriptorPool *args)
{
   struct vkr_context *ctx = dispatch->data;

   CREATE_OBJECT(pool, descriptor_pool, DESCRIPTOR_POOL, vkCreateDescriptorPool,
                 pDescriptorPool);

   list_inithead(&pool->descriptor_sets);

   vkr_device_add_object(ctx, &pool->base);
}

static void
vkr_dispatch_vkDestroyDescriptorPool(struct vn_dispatch_context *dispatch,
                                     struct vn_command_vkDestroyDescriptorPool *args)
{
   struct vkr_context *ctx = dispatch->data;

   DESTROY_OBJECT(pool, descriptor_pool, DESCRIPTOR_POOL, vkDestroyDescriptorPool,
                  descriptorPool);

   vkr_context_remove_objects(ctx, &pool->descriptor_sets);
   vkr_device_remove_object(ctx, &pool->base);
}

static void
vkr_dispatch_vkResetDescriptorPool(struct vn_dispatch_context *dispatch,
                                   struct vn_command_vkResetDescriptorPool *args)
{
   struct vkr_context *ctx = dispatch->data;

   struct vkr_descriptor_pool *pool =
      vkr_descriptor_pool_from_handle(args->descriptorPool);
   if (!pool) {
      vkr_cs_decoder_set_fatal(&ctx->decoder);
      return;
   }

   vn_replace_vkResetDescriptorPool_args_handle(args);
   args->ret = vkResetDescriptorPool(args->device, args->descriptorPool, args->flags);

   vkr_context_remove_objects(ctx, &pool->descriptor_sets);
   list_inithead(&pool->descriptor_sets);
}

static void
vkr_dispatch_vkAllocateDescriptorSets(struct vn_dispatch_context *dispatch,
                                      struct vn_command_vkAllocateDescriptorSets *args)
{
   struct vkr_context *ctx = dispatch->data;

   ALLOCATE_POOL_OBJECTS(descriptor_set, DESCRIPTOR_SET, DescriptorSet,
                         vkAllocateDescriptorSets, descriptorSetCount, descriptorPool,
                         descriptor_pool, DESCRIPTOR_POOL);
}

static void
vkr_dispatch_vkFreeDescriptorSets(struct vn_dispatch_context *dispatch,
                                  struct vn_command_vkFreeDescriptorSets *args)
{
   struct vkr_context *ctx = dispatch->data;

   FREE_POOL_OBJECTS(descriptor_set, DESCRIPTOR_SET, vkFreeDescriptorSets,
                     pDescriptorSets, descriptorSetCount, descriptorPool);

   args->ret = VK_SUCCESS;
}

static void
vkr_dispatch_vkUpdateDescriptorSets(UNUSED struct vn_dispatch_context *dispatch,
                                    struct vn_command_vkUpdateDescriptorSets *args)
{
   vn_replace_vkUpdateDescriptorSets_args_handle(args);
   vkUpdateDescriptorSets(args->device, args->descriptorWriteCount,
                          args->pDescriptorWrites, args->descriptorCopyCount,
                          args->pDescriptorCopies);
}

static void
vkr_dispatch_vkCreateDescriptorUpdateTemplate(
   struct vn_dispatch_context *dispatch,
   struct vn_command_vkCreateDescriptorUpdateTemplate *args)
{
   struct vkr_context *ctx = dispatch->data;

   CREATE_OBJECT(templ, descriptor_update_template, DESCRIPTOR_UPDATE_TEMPLATE,
                 vkCreateDescriptorUpdateTemplate, pDescriptorUpdateTemplate);

   vkr_device_add_object(ctx, &templ->base);
}

static void
vkr_dispatch_vkDestroyDescriptorUpdateTemplate(
   struct vn_dispatch_context *dispatch,
   struct vn_command_vkDestroyDescriptorUpdateTemplate *args)
{
   struct vkr_context *ctx = dispatch->data;

   DESTROY_OBJECT(templ, descriptor_update_template, DESCRIPTOR_UPDATE_TEMPLATE,
                  vkDestroyDescriptorUpdateTemplate, descriptorUpdateTemplate);

   vkr_device_remove_object(ctx, &templ->base);
}

void
vkr_context_init_descriptor_set_layout_dispatch(struct vkr_context *ctx)
{
   struct vn_dispatch_context *dispatch = &ctx->dispatch;

   dispatch->dispatch_vkGetDescriptorSetLayoutSupport =
      vkr_dispatch_vkGetDescriptorSetLayoutSupport;
   dispatch->dispatch_vkCreateDescriptorSetLayout =
      vkr_dispatch_vkCreateDescriptorSetLayout;
   dispatch->dispatch_vkDestroyDescriptorSetLayout =
      vkr_dispatch_vkDestroyDescriptorSetLayout;
}

void
vkr_context_init_descriptor_pool_dispatch(struct vkr_context *ctx)
{
   struct vn_dispatch_context *dispatch = &ctx->dispatch;

   dispatch->dispatch_vkCreateDescriptorPool = vkr_dispatch_vkCreateDescriptorPool;
   dispatch->dispatch_vkDestroyDescriptorPool = vkr_dispatch_vkDestroyDescriptorPool;
   dispatch->dispatch_vkResetDescriptorPool = vkr_dispatch_vkResetDescriptorPool;
}

void
vkr_context_init_descriptor_set_dispatch(struct vkr_context *ctx)
{
   struct vn_dispatch_context *dispatch = &ctx->dispatch;

   dispatch->dispatch_vkAllocateDescriptorSets = vkr_dispatch_vkAllocateDescriptorSets;
   dispatch->dispatch_vkFreeDescriptorSets = vkr_dispatch_vkFreeDescriptorSets;
   dispatch->dispatch_vkUpdateDescriptorSets = vkr_dispatch_vkUpdateDescriptorSets;
}

void
vkr_context_init_descriptor_update_template_dispatch(struct vkr_context *ctx)
{
   struct vn_dispatch_context *dispatch = &ctx->dispatch;

   dispatch->dispatch_vkCreateDescriptorUpdateTemplate =
      vkr_dispatch_vkCreateDescriptorUpdateTemplate;
   dispatch->dispatch_vkDestroyDescriptorUpdateTemplate =
      vkr_dispatch_vkDestroyDescriptorUpdateTemplate;
   dispatch->dispatch_vkUpdateDescriptorSetWithTemplate = NULL;
}
