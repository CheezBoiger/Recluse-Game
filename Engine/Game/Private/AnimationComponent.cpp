// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once


#include "AnimationComponent.hpp"
#include "GameObject.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Utility/Time.hpp"

namespace Recluse {


DEFINE_COMPONENT_MAP(AnimationComponent);


void AnimationComponent::OnCleanUp()
{
  gAnimation().FreeAnimObject(m_object);
  m_object = nullptr;
  UNREGISTER_COMPONENT(AnimationComponent);
}


void AnimationComponent::OnInitialize(GameObject* owner)
{
  m_object = gAnimation().CreateAnimObject(owner->GetId());
  REGISTER_COMPONENT(AnimationComponent, this);
}


void AnimationComponent::AddClip(AnimClip* clip, const std::string& name)
{
  m_clips[name] = clip;
}


void AnimationComponent::Playback(const std::string& name)
{
  auto it = m_clips.find(name);
  if (it == m_clips.end()) return;
  AnimClip* clip = it->second;
  m_currPlaybackClip = clip;
}
} // Recluse