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

  void StartUp() {
    if (IsActive()) return;

    IsActive() = true; 
    OnStartUp(); 
  }

  void ShutDown() {
    if (!IsActive()) return; 
    OnShutDown(); 
    IsActive() = false;
  }

  virtual void OnStartUp() { }

  virtual void OnShutDown() { }


  static b8& IsActive() {
    static b8 active = false;
    return active;
  }

  static Type& Instance() {
    static Type instance;
    return instance; 
  }
private:
};
} // Recluse