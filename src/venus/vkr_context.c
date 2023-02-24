/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#include "vkr_context.h"

#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "util/anon_file.h"
#include "venus-protocol/vn_protocol_renderer_dispatches.h"

#define XXH_INLINE_ALL
#include "util/xxhash.h"

#include "vkr_buffer.h"
#include "vkr_command_buffer.h"
#include "vkr_context.h"
#include "vkr_cs.h"
#include "vkr_descriptor_set.h"
#include "vkr_device.h"
#include "vkr_device_memory.h"
#include "vkr_image.h"
#include "vkr_instance.h"
#include "vkr_physical_device.h"
#include "vkr_pipeline.h"
#include "vkr_query_pool.h"
#include "vkr_queue.h"
#include "vkr_render_pass.h"
#include "vkr_ring.h"
#include "vkr_transport.h"

void
vkr_context_add_instance(struct vkr_context *ctx,
                         struct vkr_instance *instance,
                         const char *name)
{
   vkr_context_add_object(ctx, &instance->base);

   assert(!ctx->instance);
   ctx->instance = instance;

   if (name && name[0] != '\0') {
      assert(!ctx->instance_name);
      ctx->instance_name = strdup(name);
   }
}

void
vkr_context_remove_instance(struct vkr_context *ctx, struct vkr_instance *instance)
{
   assert(ctx->instance && ctx->instance == instance);
   ctx->instance = NULL;

   if (ctx->instance_name) {
      free(ctx->instance_name);
      ctx->instance_name = NULL;
   }

   vkr_context_remove_object(ctx, &instance->base);
}

static void
vkr_dispatch_debug_log(UNUSED struct vn_dispatch_context *dispatch, const char *msg)
{
   vkr_log(msg);
}

static void
vkr_context_init_dispatch(struct vkr_context *ctx)
{
   struct vn_dispatch_context *dispatch = &ctx->dispatch;

   dispatch->data = ctx;
   dispatch->debug_log = vkr_dispatch_debug_log;

   dispatch->encoder = (struct vn_cs_encoder *)&ctx->encoder;
   dispatch->decoder = (struct vn_cs_decoder *)&ctx->decoder;

   vkr_context_init_transport_dispatch(ctx);

   vkr_context_init_instance_dispatch(ctx);
   vkr_context_init_physical_device_dispatch(ctx);
   vkr_context_init_device_dispatch(ctx);

   vkr_context_init_queue_dispatch(ctx);
   vkr_context_init_fence_dispatch(ctx);
   vkr_context_init_semaphore_dispatch(ctx);
   vkr_context_init_event_dispatch(ctx);

   vkr_context_init_device_memory_dispatch(ctx);

   vkr_context_init_buffer_dispatch(ctx);
   vkr_context_init_buffer_view_dispatch(ctx);

   vkr_context_init_image_dispatch(ctx);
   vkr_context_init_image_view_dispatch(ctx);
   vkr_context_init_sampler_dispatch(ctx);
   vkr_context_init_sampler_ycbcr_conversion_dispatch(ctx);

   vkr_context_init_descriptor_set_layout_dispatch(ctx);
   vkr_context_init_descriptor_pool_dispatch(ctx);
   vkr_context_init_descriptor_set_dispatch(ctx);
   vkr_context_init_descriptor_update_template_dispatch(ctx);

   vkr_context_init_render_pass_dispatch(ctx);
   vkr_context_init_framebuffer_dispatch(ctx);

   vkr_context_init_query_pool_dispatch(ctx);

   vkr_context_init_shader_module_dispatch(ctx);
   vkr_context_init_pipeline_layout_dispatch(ctx);
   vkr_context_init_pipeline_cache_dispatch(ctx);
   vkr_context_init_pipeline_dispatch(ctx);

   vkr_context_init_command_pool_dispatch(ctx);
   vkr_context_init_command_buffer_dispatch(ctx);
}

bool
vkr_context_submit_fence(struct vkr_context *ctx,
                         uint32_t flags,
                         uint32_t ring_idx,
                         uint64_t fence_id)
{
   /* retire fence on cpu timeline directly */
   if (ring_idx == 0) {
      ctx->retire_fence(ctx->ctx_id, ring_idx, fence_id);
      return true;
   }

   if (ring_idx >= ARRAY_SIZE(ctx->sync_queues) || !ctx->sync_queues[ring_idx]) {
      vkr_log("submit_fence: invalid ring_idx %u", ring_idx);
      return false;
   }

   /* always merge fences */
   assert(!(flags & ~VIRGL_RENDERER_FENCE_FLAG_MERGEABLE));
   flags = VIRGL_RENDERER_FENCE_FLAG_MERGEABLE;
   bool ok = vkr_queue_sync_submit(ctx->sync_queues[ring_idx], flags, ring_idx, fence_id);

   return ok;
}

bool
vkr_context_submit_cmd(struct vkr_context *ctx, const void *buffer, size_t size)
{
   mtx_lock(&ctx->mutex);

   /* CS error is considered fatal (destroy the context?) */
   if (vkr_context_get_fatal(ctx)) {
      mtx_unlock(&ctx->mutex);
      vkr_log("submit_cmd: early bail due to fatal decoder state");
      return false;
   }

   vkr_cs_decoder_set_stream(&ctx->decoder, buffer, size);

   bool ok = true;
   while (vkr_cs_decoder_has_command(&ctx->decoder)) {
      vn_dispatch_command(&ctx->dispatch);
      if (vkr_context_get_fatal(ctx)) {
         vkr_log("submit_cmd: vn_dispatch_command failed");
         ok = false;
         break;
      }
   }

   vkr_cs_decoder_reset(&ctx->decoder);

   mtx_unlock(&ctx->mutex);

   return ok;
}

static inline void
vkr_context_free_resource(struct hash_entry *entry)
{
   struct vkr_resource *res = entry->data;
   if (res->fd_type == VIRGL_RESOURCE_FD_SHM)
      munmap(res->u.data, res->size);
   else if (res->u.fd >= 0)
      close(res->u.fd);
   free(res);
}

static inline bool
vkr_context_add_resource(struct vkr_context *ctx, struct vkr_resource *res)
{
   mtx_lock(&ctx->resource_mutex);
   assert(!_mesa_hash_table_search(ctx->resource_table, &res->res_id));
   struct hash_entry *entry =
      _mesa_hash_table_insert(ctx->resource_table, &res->res_id, res);
   mtx_unlock(&ctx->resource_mutex);

   return entry;
}

static inline void
vkr_context_remove_resource(struct vkr_context *ctx, uint32_t res_id)
{
   mtx_lock(&ctx->resource_mutex);
   struct hash_entry *entry = _mesa_hash_table_search(ctx->resource_table, &res_id);
   if (likely(entry)) {
      vkr_context_free_resource(entry);
      _mesa_hash_table_remove(ctx->resource_table, entry);
   }
   mtx_unlock(&ctx->resource_mutex);
}

static bool
vkr_context_import_resource_internal(struct vkr_context *ctx,
                                     uint32_t res_id,
                                     uint64_t blob_size,
                                     enum virgl_resource_fd_type fd_type,
                                     int fd,
                                     void *mmap_ptr)
{
   assert(!vkr_context_get_resource(ctx, res_id));

   struct vkr_resource *res = malloc(sizeof(*res));
   if (!res)
      return false;

   res->res_id = res_id;
   res->fd_type = fd_type;
   res->size = blob_size;

   /* fd and mmap_ptr cannot be valid at the same time, but allowed to be -1 and NULL */
   assert(fd < 0 || !mmap_ptr);
   if (mmap_ptr)
      res->u.data = mmap_ptr;
   else
      res->u.fd = fd;

   if (!vkr_context_add_resource(ctx, res)) {
      free(res);
      return false;
   }

   return true;
}

static bool
vkr_context_create_resource_from_shm(struct vkr_context *ctx,
                                     uint32_t res_id,
                                     uint64_t blob_size,
                                     struct virgl_context_blob *out_blob)
{
   assert(!vkr_context_get_resource(ctx, res_id));

   int fd = os_create_anonymous_file(blob_size, "vkr-shmem");
   if (fd < 0)
      return false;

   void *mmap_ptr = mmap(NULL, blob_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
   if (mmap_ptr == MAP_FAILED) {
      close(fd);
      return false;
   }

   if (!vkr_context_import_resource_internal(ctx, res_id, blob_size,
                                             VIRGL_RESOURCE_FD_SHM, -1, mmap_ptr)) {
      munmap(mmap_ptr, blob_size);
      close(fd);
      return false;
   }

   *out_blob = (struct virgl_context_blob){
      .type = VIRGL_RESOURCE_FD_SHM,
      .u.fd = fd,
      .map_info = VIRGL_RENDERER_MAP_CACHE_CACHED,
   };

   return true;
}

static bool
vkr_context_create_resource_from_device_memory(struct vkr_context *ctx,
                                               uint32_t res_id,
                                               uint64_t blob_id,
                                               uint64_t blob_size,
                                               uint32_t blob_flags,
                                               struct virgl_context_blob *out_blob)
{
   assert(!vkr_context_get_resource(ctx, res_id));

   struct vkr_device_memory *mem = vkr_context_get_object(ctx, blob_id);
   if (!mem || mem->base.type != VK_OBJECT_TYPE_DEVICE_MEMORY)
      return false;

   struct virgl_context_blob blob;
   if (!vkr_device_memory_export_blob(mem, blob_size, blob_flags, &blob))
      return false;

   /* For CROSS_DEVICE, store a dup'ed fd in vkr_resource for:
    * - vkAllocateMemory for dma_buf import
    * - vkGetMemoryFdPropertiesKHR for dma_buf fd properties query
    */
   int res_fd = -1;
   if (blob_flags & VIRGL_RENDERER_BLOB_FLAG_USE_CROSS_DEVICE) {
      res_fd = os_dupfd_cloexec(blob.u.fd);
      if (res_fd < 0) {
         close(blob.u.fd);
         return false;
      }
   }

   if (!vkr_context_import_resource_internal(ctx, res_id, blob_size, blob.type, res_fd,
                                             NULL)) {
      if (res_fd >= 0)
         close(res_fd);
      close(blob.u.fd);
      return false;
   }

   *out_blob = blob;

   return true;
}

bool
vkr_context_create_resource(struct vkr_context *ctx,
                            uint32_t res_id,
                            uint64_t blob_id,
                            uint64_t blob_size,
                            uint32_t blob_flags,
                            struct virgl_context_blob *out_blob)
{
   /* blob_id == 0 does not refer to an existing VkDeviceMemory, but implies a shm
    * allocation. It is logically contiguous and it can be exported.
    */
   if (!blob_id && blob_flags == VIRGL_RENDERER_BLOB_FLAG_USE_MAPPABLE)
      return vkr_context_create_resource_from_shm(ctx, res_id, blob_size, out_blob);

   return vkr_context_create_resource_from_device_memory(ctx, res_id, blob_id, blob_size,
                                                         blob_flags, out_blob);
}

bool
vkr_context_import_resource(struct vkr_context *ctx,
                            uint32_t res_id,
                            enum virgl_resource_fd_type fd_type,
                            int fd,
                            uint64_t size)
{
   return vkr_context_import_resource_internal(ctx, res_id, size, fd_type, fd, NULL);
}

void
vkr_context_destroy_resource(struct vkr_context *ctx, uint32_t res_id)
{
   struct vkr_resource *res = vkr_context_get_resource(ctx, res_id);
   if (!res)
      return;

   mtx_lock(&ctx->encoder.mutex);
   if (ctx->encoder.stream.resource && ctx->encoder.stream.resource == res) {
      /* TODO vkSetReplyCommandStreamMESA should support res_id 0 to unset. Until then,
       * and until we can ignore older guests, treat this as non-fatal. This can happen
       * when the driver side reply shmem has lost its last ref for being used as reply
       * shmem (it can still live in the driver side shmem cache but will be used for
       * other purposes the next time being allocated out).
       */
      vkr_cs_encoder_set_stream(&ctx->encoder, NULL, 0, 0);
   }
   mtx_unlock(&ctx->encoder.mutex);

   mtx_lock(&ctx->ring_mutex);
   struct vkr_ring *ring, *ring_tmp;
   LIST_FOR_EACH_ENTRY_SAFE (ring, ring_tmp, &ctx->rings, head) {
      if (ring->resource != res)
         continue;

      vkr_context_set_fatal(ctx);

      mtx_unlock(&ctx->ring_mutex);
      vkr_ring_stop(ring);
      mtx_lock(&ctx->ring_mutex);

      vkr_ring_destroy(ring);
   }
   mtx_unlock(&ctx->ring_mutex);

   vkr_context_remove_resource(ctx, res_id);
}

static inline const char *
vkr_context_get_name(const struct vkr_context *ctx)
{
   /* ctx->instance_name is the application name while ctx->debug_name is
    * usually the guest process name or the hypervisor name.  This never
    * returns NULL because ctx->debug_name is never NULL.
    */
   return ctx->instance_name ? ctx->instance_name : ctx->debug_name;
}

void
vkr_context_destroy(struct vkr_context *ctx)
{
   /* TODO Move the entire teardown process to a separate thread so that the main thread
    * cannot get blocked by the vkDeviceWaitIdle upon device destruction.
    */
   struct vkr_ring *ring, *ring_tmp;
   LIST_FOR_EACH_ENTRY_SAFE (ring, ring_tmp, &ctx->rings, head) {
      vkr_ring_stop(ring);
      vkr_ring_destroy(ring);
   }
   mtx_destroy(&ctx->ring_mutex);

   if (ctx->instance) {
      vkr_log("destroying context %d (%s) with a valid instance", ctx->ctx_id,
              vkr_context_get_name(ctx));

      vkr_instance_destroy(ctx, ctx->instance);
   }

   _mesa_hash_table_destroy(ctx->resource_table, vkr_context_free_resource);
   mtx_destroy(&ctx->resource_mutex);

   _mesa_hash_table_destroy(ctx->object_table, vkr_context_free_object);
   mtx_destroy(&ctx->object_mutex);

   vkr_cs_encoder_fini(&ctx->encoder);
   vkr_cs_decoder_fini(&ctx->decoder);

   mtx_destroy(&ctx->mutex);
   free(ctx->debug_name);
   free(ctx);
}

static uint32_t
vkr_hash_u64(const void *key)
{
   return XXH32(key, sizeof(uint64_t), 0);
}

static bool
vkr_key_u64_equal(const void *key1, const void *key2)
{
   return *(const uint64_t *)key1 == *(const uint64_t *)key2;
}

void
vkr_context_free_object(struct hash_entry *entry)
{
   struct vkr_object *obj = entry->data;
   free(obj);
}

struct vkr_context *
vkr_context_create(uint32_t ctx_id,
                   vkr_renderer_retire_fence_callback_type cb,
                   size_t debug_len,
                   const char *debug_name)
{
   struct vkr_context *ctx = calloc(1, sizeof(*ctx));
   if (!ctx)
      return NULL;

   ctx->ctx_id = ctx_id;
   ctx->retire_fence = cb;
   ctx->debug_name = malloc(debug_len + 1);
   if (!ctx->debug_name)
      goto err_debug_name;

   memcpy(ctx->debug_name, debug_name, debug_len);
   ctx->debug_name[debug_len] = '\0';

#ifdef ENABLE_VENUS_VALIDATE
   ctx->validate_level = VKR_CONTEXT_VALIDATE_ON;
   ctx->validate_fatal = false; /* TODO set this to true */
#else
   ctx->validate_level = VKR_CONTEXT_VALIDATE_NONE;
   ctx->validate_fatal = false;
#endif
   if (VKR_DEBUG(VALIDATE))
      ctx->validate_level = VKR_CONTEXT_VALIDATE_FULL;

   if (mtx_init(&ctx->mutex, mtx_plain) != thrd_success)
      goto err_mtx_init;

   if (mtx_init(&ctx->object_mutex, mtx_plain) != thrd_success)
      goto err_ctx_object_mutex;

   ctx->object_table = _mesa_hash_table_create(NULL, vkr_hash_u64, vkr_key_u64_equal);
   if (!ctx->object_table)
      goto err_ctx_object_table;

   if (mtx_init(&ctx->resource_mutex, mtx_plain) != thrd_success)
      goto err_ctx_resource_mutex;

   ctx->resource_table =
      _mesa_hash_table_create(NULL, _mesa_hash_u32, _mesa_key_u32_equal);
   if (!ctx->resource_table)
      goto err_ctx_resource_table;

   vkr_cs_decoder_init(&ctx->decoder, &ctx->cs_fatal_error, ctx->object_table);
   if (vkr_cs_encoder_init(&ctx->encoder, &ctx->cs_fatal_error))
      goto err_cs_encoder_init;

   vkr_context_init_dispatch(ctx);

   if (mtx_init(&ctx->ring_mutex, mtx_plain) != thrd_success)
      goto err_ctx_ring_mutex;

   list_inithead(&ctx->rings);

   return ctx;

err_ctx_ring_mutex:
   vkr_cs_encoder_fini(&ctx->encoder);
err_cs_encoder_init:
   _mesa_hash_table_destroy(ctx->resource_table, vkr_context_free_resource);
err_ctx_resource_table:
   mtx_destroy(&ctx->resource_mutex);
err_ctx_resource_mutex:
   _mesa_hash_table_destroy(ctx->object_table, vkr_context_free_object);
err_ctx_object_table:
   mtx_destroy(&ctx->object_mutex);
err_ctx_object_mutex:
   mtx_destroy(&ctx->mutex);
err_mtx_init:
   free(ctx->debug_name);
err_debug_name:
   free(ctx);
   return NULL;
}
