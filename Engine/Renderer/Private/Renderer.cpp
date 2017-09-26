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

  VkSubmitInfo computeSubmit = { };
  computeSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  computeSubmit.commandBufferCount = 0;
  computeSubmit.pCommandBuffers = nullptr;
  computeSubmit.signalSemaphoreCount = 0;
  computeSubmit.waitSemaphoreCount = 0;

  mRhi->ComputeSubmit(computeSubmit);
}


void Renderer::CleanUp()
{
  CleanUpGraphicsPipelines();
  CleanUpFrameBuffers();

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

  SetUpFrameBuffers();
  SetUpGraphicsPipelines();

  mRhi->SetSwapchainCmdBufferBuild([=] (CommandBuffer& cmdBuffer, VkRenderPassBeginInfo& defaultRenderpass) -> void {
    // Do stuff with the buffer.
    cmdBuffer.BeginRenderPass(defaultRenderpass, VK_SUBPASS_CONTENTS_INLINE);
    cmdBuffer.EndRenderPass();
  });

  mRhi->RebuildCommandBuffers();

  return true;
}


void Renderer::PushCmdList(CmdList* scene)
{
  mCmdList = scene;

  // Rebuild commandbuffers here.

  mRhi->RebuildCommandBuffers();
}



void Renderer::SetUpFrameBuffers()
{
  FrameBuffer* pbrFrameBuffer = mRhi->CreateFrameBuffer();
  mPbrFrameBuffer = gResources().RegisterFrameBuffer("PbrFrameBuffer", pbrFrameBuffer);
}


void Renderer::SetUpGraphicsPipelines()
{
  GraphicsPipeline* mPbrForward = mRhi->CreateGraphicsPipeline();

  VkGraphicsPipelineCreateInfo graphicsPipeline = { };
  graphicsPipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

  mPbrForwardPipeline = gResources().RegisterGraphicsPipeline("PbrForward", mPbrForward);
}


void Renderer::CleanUpGraphicsPipelines()
{
  GraphicsPipeline* pbrPipeline = gResources().UnregisterGraphicsPipeline(mPbrForwardPipeline);
  mRhi->FreeGraphicsPipeline(pbrPipeline);
}


void Renderer::CleanUpFrameBuffers()
{
  FrameBuffer* pbrFrameBuffer = gResources().UnregisterFrameBuffer(mPbrFrameBuffer);
  mRhi->FreeFrameBuffer(pbrFrameBuffer);
}
} // Recluse