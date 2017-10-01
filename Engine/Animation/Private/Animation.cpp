// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Animation.hpp"

#include "Core/Exception.hpp"

namespace Recluse {


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