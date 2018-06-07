// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once


#include "AnimationComponent.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"

namespace Recluse {


void AnimationComponent::OnCleanUp()
{
}


void AnimationComponent::OnInitialize(GameObject* owner)
{

}


void AnimationComponent::AddClip(AnimClip* clip, const std::string& name)
{
  m_clips[name] = clip;
}
} // Recluse