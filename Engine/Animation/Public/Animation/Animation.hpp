// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Module.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Thread/Threading.hpp"
#include "Clip.hpp"

#include <vector>
#include <unordered_map>

namespace Recluse {


class AnimHandle;
class AnimSampler;
struct AnimClip;
struct AnimClipState;


// AnimHandle holds information about the sampler responsible for generating the matrix palette,
// any blend jobs that may need to be incorporated to the animation poses, and handle to the game
// object that is associated with it.
class AnimHandle {
public:
  AnimHandle(uuid64 uuid)
    : m_pSamplerRef(nullptr)
    , m_uuid(uuid)
    , m_paletteSz(0) { }


  AnimSampler*      GetSampler() { return m_pSamplerRef; }
  void              SetSampler(AnimSampler* sampler) { if (sampler) m_pSamplerRef = sampler; }

  Matrix4*          GetPalette() { return m_finalPalette; }
  u32               GetPaletteSz() { return m_paletteSz; }
  
  uuid64            GetUUID() const { return m_uuid; }

  void              Update();

private:
  Matrix4           m_finalPalette[64];
  u32               m_paletteSz;
  AnimSampler*      m_pSamplerRef;
  uuid64            m_uuid;
};


// Generalized blend job. Grabs two inputs to produce the final output.
struct BlendJob {
  AnimSampler*    _pBaseSampler;
  AnimSampler*    _pBlendSampler;
  Matrix4*        _pOutputPalette;
  u32             _blendSz;
  u32             _baseSz;
  r32             _dt;              // dT mix for the base and blending palettes.
};


// A more fine grained blend job used to determine which joints are 
// going to blend within two animations.
struct LayeredBlendJob {
  AnimSampler*      _pBaseSampler;
  AnimSampler*      _pBlendSampler;
  Matrix4*          _finalPose;
  r32               _blendWeight;
  r32               _blendDepth;
};


// Animation sampling jobs are done by this engine module.
// 
class Animation : public EngineModule<Animation> {
  static const size_t     kMaxAnimationThreadCount;
public:
  Animation() 
    : m_workers(kMaxAnimationThreadCount) { }

  // On startup event.
  void          OnStartUp() override;

  // On Shutdown event.
  void          OnShutDown() override;

  // Update event.
  void          UpdateState(r64 dt);

  // Create an animation object with specified gameobject id.
  AnimHandle*   CreateAnimObject(uuid64 id);

  // Free an animation object from the animation engine.
  void          FreeAnimObject(AnimHandle* pObj);

  AnimSampler*  CreateAnimSampler();

  void          FreeAnimSampler(AnimSampler* sampler);

private:
  // Samplers to sample current animations during game runtime.
  std::unordered_map<sampler_id_t, AnimSampler> m_samplers;
  // Handler to the animation objects generated currently in use.
  std::unordered_map<uuid64, AnimHandle*>       m_animObjects;

  // Number of blend jobs to be executed after animation stepping.
  std::vector<BlendJob>                         m_blendJobs;

  // Number of layered blend jobs to be executred after animation stepping.
  std::vector<LayeredBlendJob>                  m_layeredBlendJobs;

  // Thread workers for this animation submodule.
  std::vector<std::thread>                      m_workers;
};


Animation& gAnimation();
} // Recluse