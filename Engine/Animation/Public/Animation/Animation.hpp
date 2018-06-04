// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Math/Matrix4.hpp"

#include "Clip.hpp"

#include <vector>

namespace Recluse {


class AnimObject;
class AnimSampler;
struct AnimClip;
struct AnimClipState;


class AnimObject {
public:
  AnimObject()
    : m_pSamplerRef(nullptr) { }


  AnimSampler*      GetSampler() { return m_pSamplerRef; }
private:
  AnimSampler*      m_pSamplerRef;
};


// Animation sampling jobs are done by this engine module.
// 
class Animation : public EngineModule<Animation> {
public:
  Animation() { }

  // On startup event.
  void          OnStartUp() override;

  // On Shutdown event.
  void          OnShutDown() override;

  // Update event.
  void          UpdateState(r64 dt);

  // Create an animation object with specified gameobject id.
  AnimObject*   CreateAnimObject(uuid64 id);

  // Free an animation object from the animation engine.
  void          FreeAnimObject(AnimObject* obj);

private:
  // Samplers to sample current animations during game runtime.
  std::vector<AnimSampler>      m_samplers;
  // Handler to the animation objects generated currently in use.
  std::vector<AnimObject*>       m_animObjects;
};


Animation& gAnimation();
} // Recluse