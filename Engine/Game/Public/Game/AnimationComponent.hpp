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
    : m_handle(nullptr)
    , m_currClip(nullptr) 
  {
  } 

  // Add an animation clip to the component to playback in the future.
  void addClip(AnimClip* clip, const std::string& name);

  // Blend an animation to target weight over time in seconds.
  void blendPlayback(const std::string& name, r32 targetWeight = 1.0f, r32 fadeLen = 0.3f);
  
  // Signal to play back an animation clip with given name.
  // May also optionally specify at what time to play the animation [0,1] as the normalized time.
  void playback(const std::string& name, r32 rate = 1.0f, r32 atTime = 0.0f);
  
  // Check if component is playing back a clip.
  b32    isPlayingBack(const std::string& name);

  // TODO():
  virtual void              onInitialize(GameObject* owner) override;

  // TODO():
  virtual void              onCleanUp() override;

  // TODO():
  virtual void              serialize(IArchive& archive) override { }

  // TODO():
  virtual void              deserialize(IArchive& archive) override { }

  // TODO():
  virtual void              update() override;


  AnimHandle*               getAnimHandle() { return m_handle; }

  // Set the playback rate of this animation. Current animations will also experience 
  // this rate change as well...
  void                      setPlaybackRate(r32 rate);

  // Get the current playback rate.
  r32                       getPlaybackRate() const;

private:
  std::map<std::string, AnimClip*>    m_clips;
  AnimHandle*                         m_handle;
  AnimClip*                           m_currClip;
};
} // Recluse