#include "renderer.h"
#include "util/resource.h"

#include "window.h"
#include <stdlib.h>
#include <vulkan/vulkan_core.h>

Window window;
Renderer renderer;

void clear() {
  const Device device = renderer_get_device(renderer);
  const VkDevice vk_device = device_get_handle(device);

  vkDeviceWaitIdle(vk_device);

  renderer_destroy(renderer);
  window_destroy(window);
  resources_destroy();
}

int main() {
  atexit(clear);

  if(resources_load() != SUCCESS)
    return RESOURCES_LOAD_ERROR;

  if(window_create(&window, "PXCube", 1000, 700) != SUCCESS)
    return WINDOW_CREATE_ERROR;

  if(renderer_create(&renderer, window, "PXCube", VK_MAKE_API_VERSION(0, 0, 0, 0)) != SUCCESS)
    return RENDERER_CREATE_ERROR;

  while(!window_is_closed(window)) {
    window_update(window);

    if(renderer_draw(renderer) != SUCCESS)
      return RENDERER_DRAW_ERROR;
  }

  return 0;
}
