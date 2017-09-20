// Copyright (c) 2017 Recluse Project.
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
}


void Renderer::EndFrame()
{
}


void Renderer::Render()
{
  // TODO(): Signal a beginning and end callback or so, when performing 
  // any rendering.
  BeginFrame();


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