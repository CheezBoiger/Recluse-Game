// Copyright (c) 2017 Recluse Project. All rights reserved.
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
    : m_Pool(2) { }


  void OnStartUp() override;
  void OnShutDown() override;

  ThreadPool&     ThrPool() { return m_Pool; }

  // Syncronize threads.
  // TODO():
  void Sync();

private:
  ThreadPool      m_Pool;
};


Core& gCore();
} // Recluse