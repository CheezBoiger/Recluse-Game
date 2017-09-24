// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Renderer.hpp"
#include "RHI/VulkanRHI.hpp"
#include "Core/Core.hpp"
#include "Core/Exception.hpp"

namespace Recluse {


Renderer& gRenderer() { 
  return Renderer::Instance();
}


Renderer::Renderer()
  : mRHI(nullptr)
{
}


Renderer::~Renderer()
{
}

void Renderer::OnStartUp()
{
  if (!gCore().IsActive()) {
    R_DEBUG("ERROR: Core is not active! Start up the core first!\n");
    return;
  }
  VulkanRHI::CreateContext();
  VulkanRHI::FindPhysicalDevice();
  if (!mRHI) mRHI = new VulkanRHI();
}


void Renderer::OnShutDown()
{
  CleanUp();

  // Shutdown globals.
  VulkanRHI::gPhysicalDevice.CleanUp();
  VulkanRHI::gContext.CleanUp();
}


void Renderer::BeginFrame()
{
  mRHI->AcquireNextImage();
}


void Renderer::EndFrame()
{
  mRHI->Present();
}


void Renderer::Render()
{
  // TODO(): Signal a beginning and end callback or so, when performing 
  // any rendering.
  BeginFrame(); 

    VkSemaphore signalSemaphores[] = { mRHI->SwapchainObject()->GraphicsFinishedSemaphore() };
    VkSemaphore waitSemaphores[] = { mRHI->SwapchainObject()->ImageAvailableSemaphore() };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 0;
    submitInfo.pCommandBuffers = nullptr;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    
    mRHI->GraphicsSubmit(submitInfo);



  EndFrame();
}


void Renderer::CleanUp()
{
  if (mRHI) {
    mRHI->CleanUp();
    delete mRHI;
    mRHI = nullptr;
  }
}


b8 Renderer::Initialize(Window* window)
{
  if (!window) return false;
  
  mWindowHandle = window;
  mRHI->Initialize(window->Handle());

  return true;
}
} // Recluse