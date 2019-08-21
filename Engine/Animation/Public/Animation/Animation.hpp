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
  AnimHandle(UUID64 uuid)
    : _uuid(uuid)
    , _paletteSz(64)
    , _isPerMesh(false) { 
    _currState._bEnabled = true;
    _currState._bLooping = true;
    _currState._fCurrLocalTime = 0;
    _currState._fPlaybackRate = 1.0f;
    _currState._tau = 0;
  }

  Matrix4           _finalPalette[64];
  std::vector<R32>  _finalMorphs;
  U32               _paletteSz;
  U32               _isPerMesh;
  UUID64            _uuid;
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
  R32         _jointWeights[64];
  R32         _weight;
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
  static const size_t kMaxAnimationThreadCount;
public:
  Animation() 
    : m_workers(kMaxAnimationThreadCount) { }

  // On startup event.
  void onStartUp() override;

  // On Shutdown event.
  void onShutDown() override;

  // Update event.
  void updateState(R64 dt);

  // Create an animation object with specified gameobject id.
  AnimHandle* createAnimHandle(UUID64 id);

  // Free an animation object from the animation engine.
  void freeAnimHandle(AnimHandle* pObj);
  void submitJob(const AnimJobSubmitInfo& info);

protected:
  
  void doSampleJob(AnimJobSubmitInfo& job, R32 gt);
  void doBlendJob(AnimJobSubmitInfo& job, R32 gt);
  void doSkeletalAnimation( AnimJobSubmitInfo& job, 
                            Skeleton* pSkeleton, 
                            R32 lt,
                            U32 currPoseIdx, 
                            U32 nextPoseIdx);
  void doMechanicalAnimation( AnimJobSubmitInfo& job,
                              R32 lt,
                              U32 currPoseIdx,
                              U32 nextPoseIdx);
  

  void applySkeletonPose(Matrix4* pOutput, Matrix4 globalMatrix, Skeleton* pSkeleton);
  void getCurrentAndNextPoseIdx(U32* outCurr, U32* outNext, AnimClip* pClip, R32 lt);
  B32  emptyPoseSamples(AnimClip* pClip, I32 currPoseIdx, I32 nextPoseIdx);
  void applyMorphTargets(AnimHandle* pOutput, AnimClip* pClip, I32 currPoseIdx, I32 nextPoseIdx, R32 lt);

private:

  Matrix4 linearInterpolate(JointPose* currPose, JointPose* nextPose, R32 currTime, R32 nextTime, R32 t);

  // Handler to the animation objects generated currently in use.
  std::unordered_map<UUID64, AnimHandle*> m_animObjects;

  std::mutex m_sampleJobMutex;
  std::mutex m_blendJobMutex;
  std::mutex m_layeredJobMutex;

  // Sample jobs currently in place.
  std::vector<AnimJobSubmitInfo> m_sampleJobs;
  //U32                                           m_currSampleJobCount;

  // Number of blend jobs to be executed after animation stepping.
  std::vector<AnimJobSubmitInfo> m_blendJobs;

  // Thread workers for this animation submodule.
  std::vector<std::thread> m_workers;
};


Animation& gAnimation();
} // Recluse