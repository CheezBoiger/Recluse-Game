// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "Core/Utility/Module.hpp"
#include "Core/Math/Matrix4.hpp"

namespace Recluse {


class AnimObject;


class Animation : public EngineModule<Animation> {
public:
  Animation() { }


  void OnStartUp() override;
  void OnShutDown() override;

  void UpdateState(r64 dt);

  // Create an animation object with specified gameobject id.
  AnimObject*   CreateAnimObject(uuid64 id);

  // Free an animation object from the animation engine.
  void          FreeAnimObject(AnimObject* obj);

private:

};


Animation& gAnimation();
} // Recluse