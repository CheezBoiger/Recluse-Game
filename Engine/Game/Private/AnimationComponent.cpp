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
  gAnimation().FreeAnimHandle(m_handle);
  m_handle = nullptr;
  UNREGISTER_COMPONENT(AnimationComponent);
}


void AnimationComponent::OnInitialize(GameObject* owner)
{
  m_handle = gAnimation().CreateAnimHandle(owner->GetId());
  REGISTER_COMPONENT(AnimationComponent, this);
}


void AnimationComponent::AddClip(AnimClip* clip, const std::string& name)
{
  m_clips[name] = clip;
}


void AnimationComponent::Playback(const std::string& name,  r32 rate, r32 atTime)
{
  auto it = m_clips.find(name);
  if (it == m_clips.end()) return;
  m_currClip = it->second;
  m_handle->_currState._tau = 0;
  m_handle->_currState._next = 0;
  m_handle->_currState._fCurrLocalTime = atTime * m_currClip->_fDuration;
  m_handle->_currState._fPlaybackRate = rate;
}


void AnimationComponent::Update()
{
  if ( m_currClip ) {
    AnimClip* clip = m_currClip;
    AnimJobSubmitInfo submit = {};
    submit._type = ANIM_JOB_TYPE_SAMPLE;
    submit._pBaseClip = clip;
    submit._output = m_handle;
    gAnimation().SubmitJob(submit);
  }
}


void AnimationComponent::BlendPlayback(const std::string& name, r32 targetWeight, r32 fadeLen)
{
  AnimJobSubmitInfo info{}; 
}


void AnimationComponent::SetPlaybackRate(r32 rate)
{
  m_handle->_currState._fPlaybackRate = rate;
}


r32 AnimationComponent::GetPlaybackRate() const
{
  return m_handle->_currState._fPlaybackRate;
}
} // Recluse