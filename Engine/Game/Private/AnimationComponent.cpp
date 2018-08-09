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
  m_rate = rate;
  auto it = m_clips.find(name);
  if (it == m_clips.end()) return;
  AnimClip* clip = it->second;
  AnimJobSubmitInfo submit = { };
  submit._type = ANIM_JOB_TYPE_SAMPLE;
  submit._pBaseClip = clip;
  submit._timeRatio = atTime;
  submit._playbackRate = rate;
  submit._uuid = m_handle->_uuid;
  gAnimation().SubmitJob(submit);
}


void AnimationComponent::Update()
{
  AnimSampleJob* job = gAnimation().GetCurrentSampleJob(m_handle->_uuid);
  if (!job) return;
  job->_clipState._fPlaybackRate = m_rate;
  memcpy(m_handle->_finalPalette, job->_output, sizeof(Matrix4) * job->_sz);
}


void AnimationComponent::BlendPlayback(const std::string& name, r32 targetWeight, r32 fadeLen)
{
  AnimJobSubmitInfo info{};
  
}
} // Recluse