// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Animation/Clip.hpp"
#include "Animation/Skeleton.hpp"
#include "Animation//Animation.hpp"

#include "Core/Types.hpp"
#include "Component.hpp"

#include <map>


namespace Recluse {


// Component responsible for handling animation playback, blending, and 
// updating of game component features fom the animation engine.
class AnimationComponent : public Component {
  RCOMPONENT(AnimationComponent)
public:
  AnimationComponent()
    : m_object(nullptr)
    , m_playbackRate(1.0f)
    , m_currPlaybackClip(nullptr) { }

  // Add an animation clip to the component to playback in the future.
  void AddClip(AnimClip* clip, const std::string& name);

  // Blend an animation to target weight over time in seconds.
  void BlendPlayback(const std::string& name, r32 targetWeight = 1.0f, r32 fadeLen = 0.3f);
  
  // Signal to play back an animation clip with given name.
  void Playback(const std::string& name);
  
  // Check if component is playing back a clip.
  b32    PlayingBack(const std::string& name);

  // Set an animation sampler used for sampling animation.
  void                      SetSampler(AnimSampler* sampler) { m_object->SetSampler(sampler); }

  // Get the sampler object that may be used by other animation components.
  AnimSampler*              GetSampler() { return m_object->GetSampler(); }

  // TODO():
  virtual void              OnInitialize(GameObject* owner) override;

  // TODO():
  virtual void              OnCleanUp() override;

  // TODO():
  virtual void              Serialize(IArchive& archive) override { }

  // TODO():
  virtual void              Deserialize(IArchive& archive) override { }

  // TODO():
  virtual void              Update() override {
    AnimSampler* pSampler = m_object->GetSampler();
    if (pSampler) {
      AnimClipState* state = pSampler->GetClipState();
      state->_fPlaybackRate = m_playbackRate;
      if (m_currPlaybackClip != pSampler->GetClip()) {
        pSampler->SetClip(m_currPlaybackClip);
      }
    }
  }


  AnimObject*               GetAnimObject() { return m_object; }

  // Set the playback rate of this animation. Current animations will also experience 
  // this rate change as well...
  void                      SetPlaybackRate(r32 rate) { m_playbackRate = rate; }

  // Get the current playback rate.
  r32                       GetPlaybackRate() const { return m_playbackRate; }

private:
  std::map<std::string, AnimClip*>    m_clips;
  AnimClip*                           m_currPlaybackClip;
  AnimObject*                         m_object;
  r32                                 m_playbackRate;
};
} // Recluse