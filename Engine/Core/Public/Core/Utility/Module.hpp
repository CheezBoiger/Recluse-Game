// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"


namespace Recluse {



// Engine Module allows plugins into our game engine on start up. Engine modules
// must be manually started up, or cleaned up after each use. 
template<typename Type>
class EngineModule {
public:
  virtual ~EngineModule() { }

  void startUp() {
    if (isActive()) return;

    isActive() = true; 
    onStartUp(); 
  }

  void shutDown() {
    if (!isActive()) return; 
    onShutDown(); 
    isActive() = false;
  }

  virtual void onStartUp() { }

  virtual void onShutDown() { }


  static b8& isActive() {
    static b8 active = false;
    return active;
  }

  static Type& instance() {
    static Type kInstance;
    return kInstance; 
  }
private:
};
} // Recluse