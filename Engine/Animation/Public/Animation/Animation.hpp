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


struct AnimHandle;
class AnimSampler;
struct AnimClip;
struct AnimClipState;


// AnimHandle holds information about the sampler responsible for generating the matrix palette,
// any blend jobs that may need to be incorporated to the animation poses, and handle to the game
// object that is associated with it.
struct AnimHandle {
  AnimHandle(uuid64 uuid)
    : _uuid(uuid)
    , _paletteSz(64)
    , _playbackRate(1.0f) { }

  Matrix4           _finalPalette[64];
  u32               _paletteSz;
  r32               _playbackRate;
  uuid64            _uuid;
};


enum AnimJobType {
  ANIM_JOB_TYPE_SAMPLE,
  ANIM_JOB_TYPE_BLEND,
  ANIM_JOB_TYPE_LAYERED_BLEND
};


struct AnimJobSubmitInfo {
  AnimJobType _type;
  AnimHandle* _pHandle;
  AnimClip*   _pBaseClip;
  AnimClip*   _pBlendClip;
  r32         _blendWeight;
  r32         _blendDepth;
};


struct AnimJob {
  AnimHandle*   _pHandle;
  AnimClip*     _pClip;
  AnimClipState _clipState;
};


// Generalized blend job. Grabs two inputs to produce the final output.
struct BlendJob : public AnimJob {
  AnimClip*       _pBlendClip;
  AnimClipState   _blendClipState;
  r32             _dt;              // dT mix for the base and blending palettes.
};


// A more fine grained blend job used to determine which joints are 
// going to blend within two animations.
struct LayeredBlendJob : public AnimJob {
  AnimClip*         _pBlendClip;
  AnimClipState     _blendClipState;
  r32               _blendWeight;
  r32               _blendDepth;
};


struct ClipState {
};


// Animation sampling jobs are done by this engine module.
// 
class Animation : public EngineModule<Animation> {
  static const size_t     kMaxAnimationThreadCount;
public:
  Animation() 
    : m_workers(kMaxAnimationThreadCount)
    , m_currSampleJobCount(0) { }

  // On startup event.
  void          OnStartUp() override;

  // On Shutdown event.
  void          OnShutDown() override;

  // Update event.
  void          UpdateState(r64 dt);

  // Create an animation object with specified gameobject id.
  AnimHandle*   CreateAnimHandle(uuid64 id);

  // Free an animation object from the animation engine.
  void          FreeAnimHandle(AnimHandle* pObj);

  void          SubmitJob(const AnimJobSubmitInfo& info);

protected:
  
  void          DoSampleJob(AnimJob& job, r32 gt);
  void          DoBlendJob(BlendJob& job, r32 gt);
  void          DoLayeredBlendJob(LayeredBlendJob& job, r32 gt);

  void          PushSampleJob(AnimJob& animJob) { m_sampleJobs.push_back(animJob); }
  void          PushBlendJob(BlendJob& blendJob) { m_blendJobs.push_back(blendJob); }


  void          ApplySkeletonPose(AnimHandle* pHandle, Skeleton* pSkeleton);

private:

  Matrix4          LinearInterpolate(JointPose* currPose, JointPose* nextPose, r32 currTime, r32 nextTime, r32 t);

  // Handler to the animation objects generated currently in use.
  std::unordered_map<uuid64, AnimHandle*>       m_animObjects;

  // Handles to final palattes, added when a job is pushed to this animation engine.
  std::vector<AnimHandle*>                      m_currHandles;

  std::mutex                                    m_sampleJobMutex;
  std::mutex                                    m_blendJobMutex;
  std::mutex                                    m_layeredJobMutex;

  // Sample jobs currently in place.
  std::vector<AnimJob>                          m_sampleJobs;
  u32                                           m_currSampleJobCount;

  // Number of blend jobs to be executed after animation stepping.
  std::vector<BlendJob>                         m_blendJobs;

  // Number of layered blend jobs to be executred after animation stepping.
  std::vector<LayeredBlendJob>                  m_layeredBlendJobs;

  // Thread workers for this animation submodule.
  std::vector<std::thread>                      m_workers;
};


Animation& gAnimation();
} // Recluse