// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>


namespace Recluse {



class ThreadPool;

typedef u64 thread_id_t;

class Thread {
  static thread_id_t currentId;
public:
  typedef std::function<void()>   ThreadFunction;
  Thread() 
    : mId(++currentId) { }

  void                            Start(ThreadFunction entry);
  thread_id_t                     ID() const { return mId; }

private:
  thread_id_t                     mId;
  std::thread                     mThread;
};
} // Recluse