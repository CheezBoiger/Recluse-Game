// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Threading.hpp"

namespace Recluse {


class ThreadTask {
public:

  
  void                      Start();

  b8                        OnStart();
  b8                        OnFinish();
private:
  std::function<void()>     mTask;
};


class ThreadPool {
public:
  ThreadPool(u32 numWorkerThreads)
    : mWorkers(numWorkerThreads) { }


  void                      Start();
  void                      Finish();

private:
  std::vector<Thread>       mWorkers;
  std::vector<ThreadTask>   mTasks;

  std::mutex                mTaskMutex;
  // Thread ids.
  thread_id_t               mCoreThreadId;
  thread_id_t               mSimThreadId;
  Thread                    mCoreThread;
  Thread                    mSimThread;
};
} // Recluse