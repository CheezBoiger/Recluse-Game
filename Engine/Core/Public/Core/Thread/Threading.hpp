// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <list>
#include <functional>


namespace Recluse {



class ThreadPool;

enum ThreadResult {
  ThrResultInProgress,
  ThrResultSuccess,
  ThrResultFail,
  ThrResultIncomplete
};

typedef u64 thread_id_t;
typedef u32 error_t;
typedef std::function<void()> thr_work_func_t;
typedef std::function<void(thread_id_t)> thread_func_t;

class Thread {
  // Id 0 is not a valid id.
  static thread_id_t currentId;
public:
  Thread() { }

  void                            Run(thread_func_t func, thread_id_t id);

  void                            Join() { mThread.join(); }
  void                            Detach() { mThread.detach(); }
  thread_id_t                     GetId() { return m_Id; }

private:
  std::thread                     mThread;
  thread_id_t                     m_Id;
};


// ThreadPool object, used for engine modules in need of assistance, for quicker task completion.
class ThreadPool {

public:
  ThreadPool(u32 InitThreadCount = 2) 
    : m_CurrentTaskCount(0)
    , m_ThreadWorkers(InitThreadCount)
    , m_BusyThreadCount(0)
    , m_SignalStop(true) { }

  void                  RunAll();
  void                  AddTask(thr_work_func_t WorkFunc);
  void                  ClearTasks();
  void                  StopAll();  

  b8                    AllDone() const;

  void                  WaitAll();

private:

  struct ThreadJob {
    thread_id_t         CurrThreadId;
    thr_work_func_t     Work;
    ThreadResult        Result;
  };


  std::vector<Thread>     m_ThreadWorkers;
  std::queue<ThreadJob>   m_ThreadJobs;
  std::list<ThreadJob>    m_ProgressJobs;
  std::mutex              m_JobMutex;
  std::mutex              m_ProgressMutex;
  std::condition_variable m_Cond;
  u32                     m_CurrentTaskCount;
  u32                     m_BusyThreadCount;
  u32                     m_SignalStop;
};
} // Recluse