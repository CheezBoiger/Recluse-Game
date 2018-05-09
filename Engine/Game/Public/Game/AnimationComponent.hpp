// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Animation/Clip.hpp"
#include "Animation/Skeleton.hpp"

#include "Core/Types.hpp"
#include "Component.hpp"


namespace Recluse {


// Component responsible for handling animation playback, blending, and 
// updating of game component features fom the animation engine.
class AnimationComponent : public Component {
  RCOMPONENT(AnimationComponent)
public:
  
  // Add an animation clip to the component to playback in the future.
  void AddClip(AnimationClip* clip, const char* name);

  // Blend an animation to target weight over time in seconds.
  void BlendPlayback(const char* name, r32 targetWeight = 1.0f, r32 fadeLen = 0.3f);
  
  // Signal to play back an animation clip with given name.
  void Playback(const char* name);
  
  // Check if component is playing back a clip.
  b8    PlayingBack(const char* name);
};
} // Recluse