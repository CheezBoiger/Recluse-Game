// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Animation.hpp"
#include "Skeleton.hpp"
#include "Clip.hpp"

#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Utility/Time.hpp"

namespace Recluse {

const size_t                        Animation::kMaxAnimationThreadCount   = 2;

Animation& gAnimation()
{
  return Animation::Instance();
}


void AnimHandle::Update()
{
  // Make sure these are up to date with the sampler.
  if (!m_pSamplerRef) return;

  Matrix4* dat = m_pSamplerRef->GetOutput().data();
  m_paletteSz = static_cast<u32>(m_pSamplerRef->GetOutput().size());
  memcpy(m_finalPalette, dat, m_paletteSz * sizeof(Matrix4));
}


void Animation::OnStartUp()
{
}


void Animation::OnShutDown()
{
  m_samplers.clear();
  for (auto& it : m_animObjects) {
    delete it.second;
  }
}


void Animation::UpdateState(r64 dt)
{
  r32 globalTime = static_cast<r32>(Time::CurrentTime());
  size_t H = (m_samplers.size() >> 1);
  // Animate samplers.
  for (auto& it : m_samplers) {
    AnimSampler& sampler = it.second;
    sampler.Step(globalTime);
  }

  // Store the final current poses into corresponding anim objects.
  for (auto& it : m_animObjects) {
    it.second->Update();
  }
}


AnimHandle* Animation::CreateAnimObject(uuid64 id)
{
  auto it = m_animObjects.find(id);
  if (it != m_animObjects.end()) return nullptr;
  
  AnimHandle* pObj = new AnimHandle(id);
  m_animObjects[id] = pObj;
  return pObj;
}


void Animation::FreeAnimObject(AnimHandle* pObj)
{
  if (!pObj) return;

  auto it = m_animObjects.find(pObj->GetUUID());
  if (it == m_animObjects.end()) return;

  m_animObjects.erase(it);
  delete pObj;
  pObj = nullptr;
}


AnimSampler* Animation::CreateAnimSampler()
{
  AnimSampler sampler;
  sampler_id_t id = sampler.GetId();
  m_samplers[id] = std::move(sampler);
  return &m_samplers[id];
}


void Animation::FreeAnimSampler(AnimSampler* sampler)
{
  R_ASSERT(sampler, "Sampler null when trying to delete.");
  sampler_id_t id = sampler->GetId();
  auto it = m_samplers.find(id);
  if (it != m_samplers.end()) {
    m_samplers.erase(it);
    delete sampler;
    sampler = nullptr;
  }
}
} // Recluse