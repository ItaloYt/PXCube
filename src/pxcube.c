#include "renderer/vulkan/command_buffer.h"
#include "renderer/vulkan/command_pool.h"
#include "renderer/vulkan/device.h"
#include "renderer/vulkan/fence.h"
#include "renderer/vulkan/framebuffers.h"
#include "renderer/vulkan/image_views.h"
#include "renderer/vulkan/images.h"
#include "renderer/vulkan/instance.h"
#include "renderer/vulkan/physical_devices.h"
#include "renderer/vulkan/pipeline.h"
#include "renderer/vulkan/pipeline_layout.h"
#include "renderer/vulkan/queue.h"
#include "renderer/vulkan/render_pass.h"
#include "renderer/vulkan/semaphore.h"
#include "renderer/vulkan/shader_module.h"
#include "renderer/vulkan/surface.h"
#include "renderer/vulkan/swapchain.h"
#include "util/resource.h"

#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

Resource *resources;

Window window;
Instance instance;
Surface surface;
PhysicalDevices physicals;
Device device;
Queue graphic_queue;
Queue present_queue;
Swapchain swapchain;
Images images;
ImageViews views;
RenderPass render_pass;
ShaderModule vertex_shader;
ShaderModule fragment_shader;
PipelineLayout layout;
Pipeline pipeline;
Framebuffers framebuffers;
CommandPool graphic_pool;
CommandBuffer gcommand; // Graphic Command Buffer
Semaphore render_signal;
Fence frame_signal;

void clear() {
  const VkDevice vk_device = device_get_handle(device);

  vkDeviceWaitIdle(vk_device);

  fence_destroy(frame_signal);
  semaphore_destroy(render_signal);
  command_buffer_destroy(gcommand);
  command_pool_destroy(graphic_pool);
  framebuffers_destroy(framebuffers);
  pipeline_destroy(pipeline);
  pipeline_layout_destroy(layout);
  shader_module_destroy(fragment_shader);
  shader_module_destroy(vertex_shader);
  render_pass_destroy(render_pass);
  image_views_destroy(views);
  images_destroy(images);
  swapchain_destroy(swapchain);
  queue_destroy(present_queue);
  queue_destroy(graphic_queue);
  device_destroy(device);
  physical_devices_destroy(physicals);
  surface_destroy(surface);
  instance_destroy(instance);
  window_destroy(window);

  for(unsigned index = 0; index < resource_count; ++index)
    resource_destroy(resources[index]);

  free(resources);
}

int main() {
  atexit(clear);

  resources = malloc(resource_count * sizeof(Resource));
  if(!resources) return 1;
  if(resource_load(resources) != SUCCESS) return 2;

  if(window_create(&window, "PXCube", 1000, 700) != SUCCESS) return 3;
  if(instance_create(&instance, window, "PXCube", VK_MAKE_API_VERSION(0, 0, 0, 0)) != SUCCESS) return 4;
  if(surface_create(&surface, instance) != SUCCESS) return 5;
  if(physical_devices_create(&physicals, instance) != SUCCESS) return 6;
  const VkPhysicalDevice *vk_physical_devices = physical_devices_get_handles(physicals);
  if(device_create(&device, surface, vk_physical_devices[0]) != SUCCESS) return 7;
  const unsigned graphic_family_index = device_get_graphic_family_index(device);
  const unsigned present_family_index = device_get_present_family_index(device);
  if(queue_create(&graphic_queue, device, graphic_family_index) != SUCCESS) return 8;
  if(queue_create(&present_queue, device, present_family_index) != SUCCESS) return 9;
  // TODO: Search for values in swapchain
  if(
    swapchain_create(
      &swapchain, device,
      &(VkSurfaceFormatKHR){
        .format = VK_FORMAT_B8G8R8A8_SRGB,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
      },
      VK_PRESENT_MODE_MAILBOX_KHR
    ) != SUCCESS
  ) return 10;
  if(images_create(&images, swapchain) != SUCCESS) return 11;
  if(image_views_create(&views, images) != SUCCESS) return 12;
  if(render_pass_create(&render_pass, swapchain) != SUCCESS) return 13;

  const void *shader_data = resource_get_handle(resources[0]);
  unsigned shader_size = resource_get_size(resources[0]);
  if(shader_module_create(&vertex_shader, device, shader_data, shader_size) != SUCCESS) return 14;

  shader_data = resource_get_handle(resources[1]);
  shader_size = resource_get_size(resources[1]);
  if(shader_module_create(&fragment_shader, device, shader_data, shader_size) != SUCCESS) return 15;

  if(pipeline_layout_create(&layout, device) != SUCCESS) return 16;
  if(pipeline_create(&pipeline, layout, render_pass, vertex_shader, fragment_shader) != SUCCESS) return 17;
  if(framebuffers_create(&framebuffers, render_pass, views) != SUCCESS) return 18;
  if(command_pool_create(&graphic_pool, device, graphic_family_index) != SUCCESS) return 19;
  if(command_buffer_create(&gcommand, graphic_pool) != SUCCESS) return 20;
  if(semaphore_create(&render_signal, device) != SUCCESS) return 21;
  if(fence_create(&frame_signal, device) != SUCCESS) return 22;

  const VkDevice vk_device = device_get_handle(device);
  const VkFramebuffer *vk_framebuffers = framebuffers_get_handles(framebuffers);
  const VkCommandBuffer vk_gcommand = command_buffer_get_handle(gcommand);
  const VkPipeline vk_pipeline = pipeline_get_handle(pipeline);
  const VkFence vk_frame_signal = fence_get_handle(frame_signal);
  const Semaphore image_signal = swapchain_get_image_signal(swapchain);

  while(!window_is_closed(window)) {
    window_update(window);

    if(vkWaitForFences(vk_device, 1, (VkFence[]){ vk_frame_signal }, VK_TRUE, -1) != VK_SUCCESS) return 23;
    if(vkResetFences(vk_device, 1, (VkFence[]){ vk_frame_signal }) != VK_SUCCESS) return 24;
    if(swapchain_acquire_image(swapchain, NULL) != SUCCESS) return 25;
    unsigned image_index = swapchain_get_image_index(swapchain);

    // Record Command Buffer
    if(vkResetCommandBuffer(vk_gcommand, 0) != VK_SUCCESS) return 26;
    if(command_buffer_begin(gcommand) != SUCCESS) return 27;
    render_pass_begin(render_pass, gcommand, vk_framebuffers[image_index]);
    vkCmdBindPipeline(vk_gcommand, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);
    vkCmdDraw(vk_gcommand, 3, 1, 0, 0);
    vkCmdEndRenderPass(vk_gcommand);
    if(vkEndCommandBuffer(vk_gcommand) != VK_SUCCESS) return 28;

    // Submit
    if(queue_submit(graphic_queue, gcommand, (Semaphore[]){ image_signal }, (VkPipelineStageFlags[]){ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }, 1, (Semaphore[]){ render_signal }, 1, frame_signal) != SUCCESS)
      return 29;

    if(queue_present(present_queue, swapchain, (Semaphore[]){ render_signal }, 1) != SUCCESS) return 30;
  }

  return 0;
}
