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
    : m_object(nullptr) { }

  // Add an animation clip to the component to playback in the future.
  void AddClip(AnimClip* clip, const std::string& name);

  // Blend an animation to target weight over time in seconds.
  void BlendPlayback(const std::string& name, r32 targetWeight = 1.0f, r32 fadeLen = 0.3f);
  
  // Signal to play back an animation clip with given name.
  void Playback(const std::string& name);
  
  // Check if component is playing back a clip.
  b32    PlayingBack(const std::string& name);

  // 
  virtual void              OnInitialize(GameObject* owner) override;

  // 
  virtual void              OnCleanUp() override;

  //
  virtual void              Serialize(IArchive& archive) override { }

  //
  virtual void              Deserialize(IArchive& archive) override { }

  //
  virtual void              Update() override;


  AnimObject*               GetAnimObject() { return m_object; }

private:
  std::map<std::string, AnimClip*>    m_clips;
  AnimObject*                         m_object;
};
} // Recluse