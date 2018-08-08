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
    : m_handle(nullptr) { } 

  // Add an animation clip to the component to playback in the future.
  void AddClip(AnimClip* clip, const std::string& name);

  // Blend an animation to target weight over time in seconds.
  void BlendPlayback(const std::string& name, r32 targetWeight = 1.0f, r32 fadeLen = 0.3f);
  
  // Signal to play back an animation clip with given name.
  // May also optionally specify at what time to play the animation [0,1] as the normalized time.
  void Playback(const std::string& name, r32 atTime = 0.0f);
  
  // Check if component is playing back a clip.
  b32    PlayingBack(const std::string& name);

  // TODO():
  virtual void              OnInitialize(GameObject* owner) override;

  // TODO():
  virtual void              OnCleanUp() override;

  // TODO():
  virtual void              Serialize(IArchive& archive) override { }

  // TODO():
  virtual void              Deserialize(IArchive& archive) override { }

  // TODO():
  virtual void              Update() override;


  AnimHandle*               GetAnimHandle() { return m_handle; }

  // Set the playback rate of this animation. Current animations will also experience 
  // this rate change as well...
  void                      SetPlaybackRate(r32 rate) { m_handle->_playbackRate = rate; }

  // Get the current playback rate.
  r32                       GetPlaybackRate() const { return m_handle->_playbackRate; }

private:
  std::map<std::string, AnimClip*>    m_clips;
  AnimHandle*                         m_handle;
};
} // Recluse