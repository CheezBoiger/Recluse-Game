// Copyright (c) 2017 Recluse Project.
#pragma once


#include "Core/Types.hpp"

#include "Core/Thread/Threading.hpp"
#include "Core/Thread/CoreThread.hpp"

#include "Core/Utility/Module.hpp"
#include "Core/Utility/Time.hpp"
#include "Core/Utility/Image.hpp"

#include "Core/Win32/Window.hpp"
#include "Core/Win32/Keyboard.hpp"
#include "Core/Win32/Mouse.hpp"


namespace Recluse {



// Ensures the core itself is initialized.
class Core : public EngineModule<Core> {
public:
  Core() 
    : mPool(2) { }


  void OnStartUp() override;
  void OnShutDown() override;

  // Syncronize threads.
  // TODO():
  void Sync() { }

private:
  ThreadPool      mPool;
};


Core& gCore();
} // Recluse