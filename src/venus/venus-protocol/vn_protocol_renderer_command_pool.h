/* This file is generated by venus-protocol.  See vn_protocol_renderer.h. */

/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#ifndef VN_PROTOCOL_RENDERER_COMMAND_POOL_H
#define VN_PROTOCOL_RENDERER_COMMAND_POOL_H

#include "vn_protocol_renderer_structs.h"

#pragma GCC diagnostic push
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 12
#pragma GCC diagnostic ignored "-Wdangling-pointer"
#endif
#pragma GCC diagnostic ignored "-Wpointer-arith"
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* struct VkCommandPoolCreateInfo chain */

static inline void *
vn_decode_VkCommandPoolCreateInfo_pnext_temp(struct vn_cs_decoder *dec)
{
    /* no known/supported struct */
    if (vn_decode_simple_pointer(dec))
        vn_cs_decoder_set_fatal(dec);
    return NULL;
}

static inline void
vn_decode_VkCommandPoolCreateInfo_self_temp(struct vn_cs_decoder *dec, VkCommandPoolCreateInfo *val)
{
    /* skip val->{sType,pNext} */
    vn_decode_VkFlags(dec, &val->flags);
    vn_decode_uint32_t(dec, &val->queueFamilyIndex);
}

static inline void
vn_decode_VkCommandPoolCreateInfo_temp(struct vn_cs_decoder *dec, VkCommandPoolCreateInfo *val)
{
    VkStructureType stype;
    vn_decode_VkStructureType(dec, &stype);
    if (stype != VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO)
        vn_cs_decoder_set_fatal(dec);

    val->sType = stype;
    val->pNext = vn_decode_VkCommandPoolCreateInfo_pnext_temp(dec);
    vn_decode_VkCommandPoolCreateInfo_self_temp(dec, val);
}

static inline void
vn_replace_VkCommandPoolCreateInfo_handle_self(VkCommandPoolCreateInfo *val)
{
    /* skip val->sType */
    /* skip val->pNext */
    /* skip val->flags */
    /* skip val->queueFamilyIndex */
}

static inline void
vn_replace_VkCommandPoolCreateInfo_handle(VkCommandPoolCreateInfo *val)
{
    struct VkBaseOutStructure *pnext = (struct VkBaseOutStructure *)val;

    do {
        switch ((int32_t)pnext->sType) {
        case VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO:
            vn_replace_VkCommandPoolCreateInfo_handle_self((VkCommandPoolCreateInfo *)pnext);
            break;
        default:
            /* ignore unknown/unsupported struct */
            break;
        }
        pnext = pnext->pNext;
    } while (pnext);
}

static inline void vn_decode_vkCreateCommandPool_args_temp(struct vn_cs_decoder *dec, struct vn_command_vkCreateCommandPool *args)
{
    vn_decode_VkDevice_lookup(dec, &args->device);
    if (vn_decode_simple_pointer(dec)) {
        args->pCreateInfo = vn_cs_decoder_alloc_temp(dec, sizeof(*args->pCreateInfo));
        if (!args->pCreateInfo) return;
        vn_decode_VkCommandPoolCreateInfo_temp(dec, (VkCommandPoolCreateInfo *)args->pCreateInfo);
    } else {
        args->pCreateInfo = NULL;
        vn_cs_decoder_set_fatal(dec);
    }
    if (vn_decode_simple_pointer(dec)) {
        vn_cs_decoder_set_fatal(dec);
    } else {
        args->pAllocator = NULL;
    }
    if (vn_decode_simple_pointer(dec)) {
        args->pCommandPool = vn_cs_decoder_alloc_temp(dec, sizeof(*args->pCommandPool));
        if (!args->pCommandPool) return;
        vn_decode_VkCommandPool(dec, args->pCommandPool);
    } else {
        args->pCommandPool = NULL;
        vn_cs_decoder_set_fatal(dec);
    }
}

static inline void vn_replace_vkCreateCommandPool_args_handle(struct vn_command_vkCreateCommandPool *args)
{
    vn_replace_VkDevice_handle(&args->device);
    if (args->pCreateInfo)
        vn_replace_VkCommandPoolCreateInfo_handle((VkCommandPoolCreateInfo *)args->pCreateInfo);
    /* skip args->pAllocator */
    /* skip args->pCommandPool */
}

static inline void vn_encode_vkCreateCommandPool_reply(struct vn_cs_encoder *enc, const struct vn_command_vkCreateCommandPool *args)
{
    vn_encode_VkCommandTypeEXT(enc, &(VkCommandTypeEXT){VK_COMMAND_TYPE_vkCreateCommandPool_EXT});

    vn_encode_VkResult(enc, &args->ret);
    /* skip args->device */
    /* skip args->pCreateInfo */
    /* skip args->pAllocator */
    if (vn_encode_simple_pointer(enc, args->pCommandPool))
        vn_encode_VkCommandPool(enc, args->pCommandPool);
}

static inline void vn_decode_vkDestroyCommandPool_args_temp(struct vn_cs_decoder *dec, struct vn_command_vkDestroyCommandPool *args)
{
    vn_decode_VkDevice_lookup(dec, &args->device);
    vn_decode_VkCommandPool_lookup(dec, &args->commandPool);
    if (vn_decode_simple_pointer(dec)) {
        vn_cs_decoder_set_fatal(dec);
    } else {
        args->pAllocator = NULL;
    }
}

static inline void vn_replace_vkDestroyCommandPool_args_handle(struct vn_command_vkDestroyCommandPool *args)
{
    vn_replace_VkDevice_handle(&args->device);
    vn_replace_VkCommandPool_handle(&args->commandPool);
    /* skip args->pAllocator */
}

static inline void vn_encode_vkDestroyCommandPool_reply(struct vn_cs_encoder *enc, const struct vn_command_vkDestroyCommandPool *args)
{
    vn_encode_VkCommandTypeEXT(enc, &(VkCommandTypeEXT){VK_COMMAND_TYPE_vkDestroyCommandPool_EXT});

    /* skip args->device */
    /* skip args->commandPool */
    /* skip args->pAllocator */
}

static inline void vn_decode_vkResetCommandPool_args_temp(struct vn_cs_decoder *dec, struct vn_command_vkResetCommandPool *args)
{
    vn_decode_VkDevice_lookup(dec, &args->device);
    vn_decode_VkCommandPool_lookup(dec, &args->commandPool);
    vn_decode_VkFlags(dec, &args->flags);
}

static inline void vn_replace_vkResetCommandPool_args_handle(struct vn_command_vkResetCommandPool *args)
{
    vn_replace_VkDevice_handle(&args->device);
    vn_replace_VkCommandPool_handle(&args->commandPool);
    /* skip args->flags */
}

static inline void vn_encode_vkResetCommandPool_reply(struct vn_cs_encoder *enc, const struct vn_command_vkResetCommandPool *args)
{
    vn_encode_VkCommandTypeEXT(enc, &(VkCommandTypeEXT){VK_COMMAND_TYPE_vkResetCommandPool_EXT});

    vn_encode_VkResult(enc, &args->ret);
    /* skip args->device */
    /* skip args->commandPool */
    /* skip args->flags */
}

static inline void vn_decode_vkTrimCommandPool_args_temp(struct vn_cs_decoder *dec, struct vn_command_vkTrimCommandPool *args)
{
    vn_decode_VkDevice_lookup(dec, &args->device);
    vn_decode_VkCommandPool_lookup(dec, &args->commandPool);
    vn_decode_VkFlags(dec, &args->flags);
}

static inline void vn_replace_vkTrimCommandPool_args_handle(struct vn_command_vkTrimCommandPool *args)
{
    vn_replace_VkDevice_handle(&args->device);
    vn_replace_VkCommandPool_handle(&args->commandPool);
    /* skip args->flags */
}

static inline void vn_encode_vkTrimCommandPool_reply(struct vn_cs_encoder *enc, const struct vn_command_vkTrimCommandPool *args)
{
    vn_encode_VkCommandTypeEXT(enc, &(VkCommandTypeEXT){VK_COMMAND_TYPE_vkTrimCommandPool_EXT});

    /* skip args->device */
    /* skip args->commandPool */
    /* skip args->flags */
}

static inline void vn_dispatch_vkCreateCommandPool(struct vn_dispatch_context *ctx, VkCommandFlagsEXT flags)
{
    struct vn_command_vkCreateCommandPool args;

    if (!ctx->dispatch_vkCreateCommandPool) {
        vn_cs_decoder_set_fatal(ctx->decoder);
        return;
    }

    vn_decode_vkCreateCommandPool_args_temp(ctx->decoder, &args);
    if (!args.device) {
        vn_cs_decoder_set_fatal(ctx->decoder);
        return;
    }

    if (!vn_cs_decoder_get_fatal(ctx->decoder))
        ctx->dispatch_vkCreateCommandPool(ctx, &args);

#ifdef DEBUG
    if (!vn_cs_decoder_get_fatal(ctx->decoder) && vn_dispatch_should_log_result(args.ret))
        vn_dispatch_debug_log(ctx, "vkCreateCommandPool returned %d", args.ret);
#endif

    if ((flags & VK_COMMAND_GENERATE_REPLY_BIT_EXT) && !vn_cs_decoder_get_fatal(ctx->decoder))
        vn_encode_vkCreateCommandPool_reply(ctx->encoder, &args);

    vn_cs_decoder_reset_temp_pool(ctx->decoder);
}

static inline void vn_dispatch_vkDestroyCommandPool(struct vn_dispatch_context *ctx, VkCommandFlagsEXT flags)
{
    struct vn_command_vkDestroyCommandPool args;

    if (!ctx->dispatch_vkDestroyCommandPool) {
        vn_cs_decoder_set_fatal(ctx->decoder);
        return;
    }

    vn_decode_vkDestroyCommandPool_args_temp(ctx->decoder, &args);
    if (!args.device) {
        vn_cs_decoder_set_fatal(ctx->decoder);
        return;
    }

    if (!vn_cs_decoder_get_fatal(ctx->decoder))
        ctx->dispatch_vkDestroyCommandPool(ctx, &args);

    if ((flags & VK_COMMAND_GENERATE_REPLY_BIT_EXT) && !vn_cs_decoder_get_fatal(ctx->decoder))
        vn_encode_vkDestroyCommandPool_reply(ctx->encoder, &args);

    vn_cs_decoder_reset_temp_pool(ctx->decoder);
}

static inline void vn_dispatch_vkResetCommandPool(struct vn_dispatch_context *ctx, VkCommandFlagsEXT flags)
{
    struct vn_command_vkResetCommandPool args;

    if (!ctx->dispatch_vkResetCommandPool) {
        vn_cs_decoder_set_fatal(ctx->decoder);
        return;
    }

    vn_decode_vkResetCommandPool_args_temp(ctx->decoder, &args);
    if (!args.device) {
        vn_cs_decoder_set_fatal(ctx->decoder);
        return;
    }

    if (!vn_cs_decoder_get_fatal(ctx->decoder))
        ctx->dispatch_vkResetCommandPool(ctx, &args);

#ifdef DEBUG
    if (!vn_cs_decoder_get_fatal(ctx->decoder) && vn_dispatch_should_log_result(args.ret))
        vn_dispatch_debug_log(ctx, "vkResetCommandPool returned %d", args.ret);
#endif

    if ((flags & VK_COMMAND_GENERATE_REPLY_BIT_EXT) && !vn_cs_decoder_get_fatal(ctx->decoder))
        vn_encode_vkResetCommandPool_reply(ctx->encoder, &args);

    vn_cs_decoder_reset_temp_pool(ctx->decoder);
}

static inline void vn_dispatch_vkTrimCommandPool(struct vn_dispatch_context *ctx, VkCommandFlagsEXT flags)
{
    struct vn_command_vkTrimCommandPool args;

    if (!ctx->dispatch_vkTrimCommandPool) {
        vn_cs_decoder_set_fatal(ctx->decoder);
        return;
    }

    vn_decode_vkTrimCommandPool_args_temp(ctx->decoder, &args);
    if (!args.device) {
        vn_cs_decoder_set_fatal(ctx->decoder);
        return;
    }

    if (!vn_cs_decoder_get_fatal(ctx->decoder))
        ctx->dispatch_vkTrimCommandPool(ctx, &args);

    if ((flags & VK_COMMAND_GENERATE_REPLY_BIT_EXT) && !vn_cs_decoder_get_fatal(ctx->decoder))
        vn_encode_vkTrimCommandPool_reply(ctx->encoder, &args);

    vn_cs_decoder_reset_temp_pool(ctx->decoder);
}

#pragma GCC diagnostic pop

#endif /* VN_PROTOCOL_RENDERER_COMMAND_POOL_H */
