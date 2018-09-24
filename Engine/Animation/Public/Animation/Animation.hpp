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
    , _paletteSz(64) { 
    _currState._bEnabled = true;
    _currState._bLooping = true;
    _currState._fCurrLocalTime = 0;
    _currState._fPlaybackRate = 1.0f;
    _currState._tau = 0;
  }

  Matrix4           _finalPalette[64];
  std::vector<r32>  _finalMorphs;
  u32               _paletteSz;
  uuid64            _uuid;
  AnimClipState     _currState;
};


enum AnimJobType {
  ANIM_JOB_TYPE_SAMPLE,
  ANIM_JOB_TYPE_BLEND,
  ANIM_JOB_TYPE_LAYERED_BLEND
};


struct AnimBlendLayer {
  // Local transforms that are outputted by the AnimSample job.
  Matrix4     _transforms[64];
  // Individual joint weight for per joint blending.
  r32         _jointWeights[64];
  r32         _weight;
};


struct AnimJobSubmitInfo {
  AnimJobType                 _type;
  AnimHandle*                 _output;            // uuid belonging to this anim submittal.
  AnimClip*                   _pBaseClip;       // base clip to use during sampling.
  std::vector<AnimBlendLayer> _layers;
  std::vector<AnimBlendLayer> _additiveLayers;
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
  AnimHandle*   CreateAnimHandle(uuid64 id);

  // Free an animation object from the animation engine.
  void          FreeAnimHandle(AnimHandle* pObj);

  void          SubmitJob(const AnimJobSubmitInfo& info);

protected:
  
  void          DoSampleJob(AnimJobSubmitInfo& job, r32 gt);
  void          DoBlendJob(AnimJobSubmitInfo& job, r32 gt);

  void          ApplySkeletonPose(Matrix4* pOutput, Skeleton* pSkeleton);

private:

  Matrix4          LinearInterpolate(JointPose* currPose, JointPose* nextPose, r32 currTime, r32 nextTime, r32 t);

  // Handler to the animation objects generated currently in use.
  std::unordered_map<uuid64, AnimHandle*>       m_animObjects;

  std::mutex                                    m_sampleJobMutex;
  std::mutex                                    m_blendJobMutex;
  std::mutex                                    m_layeredJobMutex;

  // Sample jobs currently in place.
  std::vector<AnimJobSubmitInfo>                    m_sampleJobs;
  //u32                                           m_currSampleJobCount;

  // Number of blend jobs to be executed after animation stepping.
  std::vector<AnimJobSubmitInfo>                     m_blendJobs;

  // Thread workers for this animation submodule.
  std::vector<std::thread>                      m_workers;
};


Animation& gAnimation();
} // Recluse