// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once


#include "AnimationComponent.hpp"
#include "GameObject.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Utility/Time.hpp"

namespace Recluse {


void AnimationComponent::OnCleanUp()
{
  gAnimation().FreeAnimObject(m_object);
  m_object = nullptr;
}


void AnimationComponent::OnInitialize(GameObject* owner)
{
  m_object = gAnimation().CreateAnimObject(owner->GetId());
}


void AnimationComponent::AddClip(AnimClip* clip, const std::string& name)
{
  m_clips[name] = clip;
}


void AnimationComponent::Playback(const std::string& name)
{
  auto it = m_clips.find(name);
  if (it == m_clips.end()) return;
  R_ASSERT(m_object, "Anim object was null.");
  AnimClip* clip = it->second;
  AnimSampler* pSampler = m_object->GetSampler();
  pSampler->SetClip(clip);
  pSampler->Play(static_cast<r32>(Time::CurrentTime()));
}
} // Recluse