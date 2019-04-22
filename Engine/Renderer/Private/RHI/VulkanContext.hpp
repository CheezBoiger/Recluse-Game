// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "VulkanConfigs.hpp"
#include "PhysicalDevice.hpp"


#define ENGINE_NAME "Recluse"


namespace Recluse {


class Context {
public:
  Context()
    : mInstance(VK_NULL_HANDLE)
    , mDebugReportCallback(VK_NULL_HANDLE)
    , mDebugEnabled(false) { }


  b32                              createInstance(const char* appName);
  VkInstance                      currentInstance() { return mInstance; }
  VkSurfaceKHR                    createSurface(void* handle);

  void                            destroySurface(VkSurfaceKHR surface);
  void                            enableDebugMode();
  void                            cleanUp();
  
  // You get to find all gpus on this machine, how cool is that...
  std::vector<VkPhysicalDevice>&  enumerateGpus();

private:
  void                            setUpDebugCallback();
  void                            cleanUpDebugCallback();
  VkInstance                      mInstance;
  std::vector<VkPhysicalDevice>   mGpus;
  VkDebugReportCallbackEXT        mDebugReportCallback;
  b32                              mDebugEnabled;
};
} // Recluse