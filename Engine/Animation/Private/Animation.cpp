// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Animation.hpp"
#include "Skeleton.hpp"
#include "Clip.hpp"

#include "Core/Exception.hpp"
#include "Core/Utility/Time.hpp"

namespace Recluse {

skeleton_uuid_t Skeleton::kCurrSkeleCount = 0;
std::map<skeleton_uuid_t, Skeleton> Skeleton::kSkeletons;

Animation& gAnimation()
{
  return Animation::Instance();
}


void Animation::OnStartUp()
{
}


void Animation::OnShutDown()
{
}


void Animation::UpdateState(r64 dt)
{
  for (auto& it : m_samplers) {
    AnimSampler& sampler = it.second;
    sampler.Step(static_cast<r32>(Time::CurrentTime()));
  }
}


AnimObject* Animation::CreateAnimObject(uuid64 id)
{
  auto it = m_animObjects.find(id);
  if (it != m_animObjects.end()) return nullptr;
  
  AnimObject* pObj = new AnimObject(id);
  m_animObjects[id] = pObj;
  m_samplers[id] = AnimSampler();
  pObj->SetSampler(&m_samplers[id]);
  return pObj;
}


void Animation::FreeAnimObject(AnimObject* pObj)
{
  if (!pObj) return;

  auto it = m_animObjects.find(pObj->GetUUID());
  if (it == m_animObjects.end()) return;

  m_samplers.erase(pObj->GetUUID());
  m_animObjects.erase(it);
  delete pObj;
  pObj = nullptr;
}
} // Recluse