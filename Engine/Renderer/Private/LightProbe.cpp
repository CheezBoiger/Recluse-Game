//
#include "LightProbe.hpp"
#include "TextureType.hpp"
#include "RHI/VulkanRHI.hpp"
#include "RHI/Buffer.hpp"
#include "RHI/Texture.hpp"

#include "Core/Logging/Log.hpp"
#include "Filesystem/Filesystem.hpp"

namespace Recluse {


Vector3 GetWorldDirection(u32 faceIdx, u32 px, u32 py, u32 width, u32 height)
{
  Vector3 worldDir = Vector3();

  return worldDir;
}


Vector3 GetWorldNormalFromCubeFace(u32 idx)
{
  // Get cube face normal.
  Vector3 n = Vector3(0.0f);
  n[idx >> 1] = (idx & 1) ? 1.0f : -1.0f;
  return n;
}


void LightProbe::GenerateSHCoefficients(VulkanRHI* rhi, TextureCube* texCube)
{
  u32 width = texCube->WidthPerFace();
  u32 height = texCube->HeightPerFace();
  r32 pixelA = (1.0f / static_cast<r32>(width)) * (1.0f / static_cast<r32>(height));
  u8* data = new u8[width * height * 4];

  {
    VkCommandBuffer cmdBuf;
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = rhi->GraphicsCmdPool(0);
    allocInfo.commandBufferCount = 1;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(rhi->LogicDevice()->Native(), &allocInfo, &cmdBuf);
    
    // Read image data through here.
    // TODO(): 

    vkFreeCommandBuffers(rhi->LogicDevice()->Native(), rhi->GraphicsCmdPool(0), 1, &cmdBuf);
  }
  // Reference by Jian Ru's Laugh Engine implementation: https://github.com/jian-ru/laugh_engine
  // Research information though https://cseweb.ucsd.edu/~ravir/papers/envmap/envmap.pdf
  //
  for (u32 fi = 0; fi < 6; ++fi ) {
    Vector3 n = GetWorldNormalFromCubeFace(fi);
    for (u32 py = 0; py < height; ++py) {
      for (u32 px = 0; px < width; ++px) {
        
        Vector3 wi = GetWorldDirection(fi, px, py, width, height);
        r32 dist2 = wi.Dot(wi); 
        wi = wi.Normalize();
        // Obtain our solid angle differential.
        r32 dw = pixelA * n.Dot(-wi) / dist2;
        Vector3 L = Vector3(data[py * width + px]);

        // Compute our coefficients by taking the sum of each lobe and applying over the distance lighting distribution.
        // which calculates the overall irradiance E(). 
        _shcoeff[0] += L * 0.282095f * dw;
        _shcoeff[1] += L * 0.488603f * dw * wi.y;
        _shcoeff[2] += L * 0.488603f * dw * wi.z;
        _shcoeff[3] += L * 0.488603f * dw * wi.x;
        _shcoeff[4] += L * 1.092548f * dw * wi.x * wi.y;
        _shcoeff[5] += L * 1.092548f * dw * wi.y * wi.z;
        _shcoeff[6] += L * 1.092548f * dw * wi.x * wi.z;
        _shcoeff[7] += L * 0.315392f * dw * (3.0f * (wi.z * wi.z) - 1.0f);
        _shcoeff[8] += L * 0.546274f * dw * (wi.x * wi.x - wi.y * wi.y); 
      }
    }
  }
  
  delete[] data;
}


b32 LightProbe::SaveToFile(const std::string& filename)
{
  // TODO(): 
  return false;
}


b32 LightProbe::LoadFromFile(const std::string& filename)
{
  memset(_shcoeff, 0, sizeof(_shcoeff));
  _position = Vector3();
  _r = 0.0f;

  // Load up coefficients here.

  // TODO():
  return false;
}
} // Recluse