// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Animation.hpp"
#include "Skeleton.hpp"
#include "Clip.hpp"

#include "Core/Exception.hpp"

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
}
} // Recluse