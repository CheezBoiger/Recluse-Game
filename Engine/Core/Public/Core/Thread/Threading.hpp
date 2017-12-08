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
typedef u32 error_t;
typedef error_t(*thr_work_func_t)(void);

class Thread {
  // Id 0 is not a valid id.
  static thread_id_t currentId;
public:
  Thread() 
    : mId(++currentId) { }

  void                            Start(thr_work_func_t entry);
  thread_id_t                     ID() const { return mId; }

private:
  thread_id_t                     mId;
  std::thread                     mThread;
};


// ThreadPool object, used for engine modules in need of assistance, for quicker task completion.
class ThreadPool {
  enum ThreadResult {
    ThrResultInProgress,
    ThrResultSuccess,
    ThrResultFail,
    ThrResultIncomplete
  };

public:
  ThreadPool(u32 InitThreadCount);

  void                  Run();
  void                  AddJob(thr_work_func_t WorkFunc);
  void                  ClearJobs();
  void                  StopAll();  

  b8                    AllDone() const;
  b8                    Finished() const;

  void                  WaitAll();

private:

  struct ThreadJob {
    thread_id_t         CurrThreadId;
    thr_work_func_t     Work;
    ThreadResult        Result;
  };


  std::vector<Thread>     m_ThreadWorkers;
  std::queue<ThreadJob>   m_ThreadJobs;
  std::mutex              m_Mutex;
  std::condition_variable m_Cond;
  u32                     m_CurrentTaskCount;
};
} // Recluse