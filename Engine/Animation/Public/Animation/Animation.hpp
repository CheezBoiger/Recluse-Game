// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Math/Matrix4.hpp"

#include "Clip.hpp"

#include <vector>
#include <unordered_map>

namespace Recluse {


class AnimObject;
class AnimSampler;
struct AnimClip;
struct AnimClipState;


class AnimObject {
public:
  AnimObject(uuid64 uuid)
    : m_pSamplerRef(nullptr)
    , m_uuid(uuid) { }


  AnimSampler*      GetSampler() { return m_pSamplerRef; }
  void              SetSampler(AnimSampler* sampler) { m_pSamplerRef = sampler; }

  uuid64            GetUUID() const { return m_uuid; }
private:
  AnimSampler*      m_pSamplerRef;
  uuid64            m_uuid;
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
  void          FreeAnimObject(AnimObject* pObj);

private:
  // Samplers to sample current animations during game runtime.
  std::unordered_map<uuid64, AnimSampler>      m_samplers;
  // Handler to the animation objects generated currently in use.
  std::unordered_map<uuid64, AnimObject*>       m_animObjects;
};


Animation& gAnimation();
} // Recluse