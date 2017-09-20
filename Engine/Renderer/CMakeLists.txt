cmake_minimum_required(VERSION 3.0)
project("Renderer")

if (NOT Vulkan_FOUND)
  message(ERROR_FATAL "Renderer Requires Vulkan, which is not found on your machine!")
endif()


set(RECLUSE_RENDERER_PRIVATE_DIR    Private)
set(RECLUSE_RENDERER_PUBLIC_DIR     Public/Renderer)

include_directories(
  ${RECLUSE_ENGINE_INCLUDE_DIRS}
  ${RECLUSE_RENDERER_INCLUDE_DIR}/Renderer
  ${Vulkan_INCLUDE_DIRS}
)


set(RECLUSE_RENDERER_FILES
  ${RECLUSE_RENDERER_PUBLIC_DIR}/Mesh.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RenderObject.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/Renderer.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/UserParams.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/GpuParams.hpp

  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/Buffer.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/Commandbuffer.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/ComputePipeline.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/Framebuffer.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/GraphicsPipeline.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/Renderpass.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/Rendertarget.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/Shader.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/Swapchain.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/Texture.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/VulkanConfigs.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/VulkanRHI.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/VulkanContext.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/PhysicalDevice.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/LogicalDevice.hpp

  
  ${RECLUSE_RENDERER_PRIVATE_DIR}/Renderer.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/Buffer.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/Commandbuffer.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/ComputePipeline.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/Framebuffer.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/GraphicsPipeline.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/Renderpass.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/Rendertarget.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/Shader.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/Swapchain.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/Texture.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/VulkanRHI.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/VulkanContext.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/PhysicalDevice.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/LogicalDevice.cpp
)


add_library(${RECLUSE_RENDERER}
  ${RECLUSE_RENDERER_FILES}
)


target_link_libraries(${RECLUSE_RENDERER}
  ${Vulkan_LIBRARIES}
)



# Doing some stuff for organization.
if (MSVC)
  foreach(source IN LISTS RECLUSE_RENDERER_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
  endforeach()
endif()

# Visual Studio organization. For Good purposes.
if (WIN32)
  set_property(TARGET ${RECLUSE_RENDERER} 
    PROPERTY 
    FOLDER ${RECLUSE_NAME})
endif()