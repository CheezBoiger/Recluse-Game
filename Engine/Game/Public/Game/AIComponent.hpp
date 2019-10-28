// Copyright (c) 2019 Recluse Project. All rights reserved
#pragma once

#include "Component.hpp"
#include "AI/PathFinding.hpp"
#include "AI/BehaviorGraph.hpp"


namespace Recluse {



class AIComponent : public Component {
public:

  void onInitialize(GameObject* owner) override { }
  void onCleanUp() override { }
  void onEnable() override { }
  void update() { }

  // Set the time trigger for this ai component to update.
  void setPerUpdateTick(R32 tick) { }
};
} // namespace Recluse