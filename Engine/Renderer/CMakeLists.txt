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
  ${RECLUSE_RENDERER_PUBLIC_DIR}/Resources.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/CmdList.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/TextureCache.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/GpuParams.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/Vertex.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/Buffer.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/CommandBuffer.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/ComputePipeline.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/FrameBuffer.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/GraphicsPipeline.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/Shader.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/Swapchain.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/Texture.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/VulkanConfigs.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/VulkanRHI.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/VulkanContext.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/PhysicalDevice.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/LogicalDevice.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RHI/DescriptorSet.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/RenderCmd.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/ScreenQuad.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/VertexBuffer.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/IndexBuffer.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/StructuredBuffer.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/Material.hpp
  ${RECLUSE_RENDERER_PUBLIC_DIR}/TextureType.hpp

  ${RECLUSE_RENDERER_PRIVATE_DIR}/RendererData.hpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RendererData.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/TextureType.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/ScreenQuad.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/DescriptorSet.cpp  
  ${RECLUSE_RENDERER_PRIVATE_DIR}/Renderer.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/Resources.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/Buffer.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/CommandBuffer.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/ComputePipeline.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/FrameBuffer.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/GraphicsPipeline.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/Shader.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/Swapchain.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/Texture.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/VulkanRHI.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/VulkanContext.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/PhysicalDevice.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RHI/LogicalDevice.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/CmdList.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/RenderCmd.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/VertexBuffer.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/IndexBuffer.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/Material.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/Mesh.cpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/UIOverlay.hpp
  ${RECLUSE_RENDERER_PRIVATE_DIR}/UIOverlay.cpp
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