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
    , _paletteSz(64) { }

  Matrix4           _finalPalette[64];
  u32               _paletteSz;
  uuid64            _uuid;
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
  uuid64                      _uuid;            // uuid belonging to this anim submittal.
  AnimClip*                   _pBaseClip;       // base clip to use during sampling.
  r32                         _blendWeight;     // global blend weight.
  r32                         _timeRatio;       // [0,1] time ratio interval.
  r32                         _playbackRate;    // Initial playback rate.
  b32                         _looping;         // is this job going to loop?
  std::vector<AnimBlendLayer> _layers;
  std::vector<AnimBlendLayer> _additiveLayers;
};


struct AnimSampleJob {
  Matrix4       _output[64];
  u32           _sz;
  AnimClip*     _pClip;
  AnimClipState _clipState;
};


// Generalized blend job. Grabs two inputs to produce the final output.
struct AnimBlendJob {
  std::vector<AnimBlendLayer>       _layers;
  std::vector<AnimBlendLayer>       _additiveLayers;
  skeleton_uuid_t                   _skeletonId;
  Matrix4                           _output[64];
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

  AnimSampleJob*  GetCurrentSampleJob(uuid64 uuid);
  AnimBlendJob*   GetCurrentBlendJob(uuid64 uuid);

protected:
  
  void          DoSampleJob(AnimSampleJob& job, r32 gt);
  void          DoBlendJob(AnimBlendJob& job, r32 gt);

  void          PushSampleJob(AnimSampleJob& animJob) { m_sampleJobs.push_back(animJob); }
  void          PushBlendJob(AnimBlendJob& blendJob) { m_blendJobs.push_back(blendJob); }


  void          ApplySkeletonPose(Matrix4* pOutput, Skeleton* pSkeleton);

private:

  Matrix4          LinearInterpolate(JointPose* currPose, JointPose* nextPose, r32 currTime, r32 nextTime, r32 t);

  // Handler to the animation objects generated currently in use.
  std::unordered_map<uuid64, AnimHandle*>       m_animObjects;

  // Handles to final palattes, added when a job is pushed to this animation engine.
  std::map<uuid64, AnimSampleJob*>         m_sampleJobMaps;
  std::map<uuid64, AnimBlendJob*>          m_blendJobMaps;

  std::mutex                                    m_sampleJobMutex;
  std::mutex                                    m_blendJobMutex;
  std::mutex                                    m_layeredJobMutex;

  // Sample jobs currently in place.
  std::vector<AnimSampleJob>                          m_sampleJobs;
  u32                                           m_currSampleJobCount;

  // Number of blend jobs to be executed after animation stepping.
  std::vector<AnimBlendJob>                         m_blendJobs;

  // Thread workers for this animation submodule.
  std::vector<std::thread>                      m_workers;
};


Animation& gAnimation();
} // Recluse