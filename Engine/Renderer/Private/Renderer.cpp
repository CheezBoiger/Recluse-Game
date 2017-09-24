// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Renderer.hpp"
#include "CmdList.hpp"

#include "RHI/VulkanRHI.hpp"
#include "RHI/GraphicsPipeline.hpp"
#include "Core/Core.hpp"
#include "Core/Exception.hpp"

namespace Recluse {


Renderer& gRenderer() { 
  return Renderer::Instance();
}


Renderer::Renderer()
  : mRhi(nullptr)
{
}


void SetUpDefault(CommandBuffer& cmdBuffer)
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
  if (!mRhi) mRhi = new VulkanRHI();
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
  mRhi->AcquireNextImage();
}


void Renderer::EndFrame()
{
  mRhi->Present();
}


void Renderer::Render()
{
  // TODO(): Signal a beginning and end callback or so, when performing 
  // any rendering.
  BeginFrame();
    mRhi->SubmitCurrSwapchainCmdBuffer();
  EndFrame();
}


void Renderer::CleanUp()
{
  if (mRhi) {
    mRhi->CleanUp();
    delete mRhi;
    mRhi = nullptr;
  }
}


b8 Renderer::Initialize(Window* window)
{
  if (!window) return false;
  
  mWindowHandle = window;
  mRhi->Initialize(window->Handle());

  mRhi->SetSwapchainCmdBufferBuild(SetUpDefault);

  return true;
}


void Renderer::PushCmdList(CmdList* scene)
{
}
} // Recluse