// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "Core/Utility/Module.hpp"


namespace Recluse {



class Animation : public EngineModule<Animation> {
public:
  Animation() { }


  void OnStartUp() override;
  void OnShutDown() override;

  void UpdateState(r64 dt);

private:

};


Animation& gAnimation();
} // Recluse