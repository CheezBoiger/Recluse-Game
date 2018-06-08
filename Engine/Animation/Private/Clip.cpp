// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Clip.hpp"
#include "Core/Types.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


AnimSampler::AnimSampler()
  : _pClip(nullptr)
  , m_output(128)
{
  _state._bEnabled          = true;
  _state._bLooping          = true;
  _state._fCurrLocalTime    = 0.0f;
  _state._fPlaybackRate     = 1.0f;
  _state._fWeight           = 1.0f;
}


void AnimSampler::Step(r32 gt)
{
  if (!_state._bEnabled) return;

  R_ASSERT(_pClip, "No clip for this sampler.");
  r32 R = _state._fPlaybackRate;
  r32 t = (gt - _tauS) * R;
  if (_state._bLooping && t > _pClip->_fDuration) {
    Play(gt);
    t = t - _pClip->_fDuration;
  }
  _state._fCurrLocalTime = t;

  // TODO(): Now find the local poses of t.
  Log() << "local time t: " << t << " sec\t\t\r";
}
} // Recluse