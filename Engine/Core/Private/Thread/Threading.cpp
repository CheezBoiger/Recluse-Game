// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Thread/Threading.hpp"

#include "Logging/Log.hpp"
#include "Exception.hpp"
#include <atomic>


namespace Recluse {


thread_id_t Thread::currentId = 0;
std::atomic_uint32_t a_BusyThreadCount = 0;


void Thread::run(thread_func_t entry, thread_id_t id)
{
  m_Id = id;
  mThread = std::thread(entry, id);
}


void ThreadPool::RunAll()
{
  m_SignalStop = false;
  for (size_t i = 0; i < m_ThreadWorkers.size(); ++i) {
    m_ThreadWorkers[i].run([&] (thread_id_t id) -> void {
      R_DEBUG(rNotify, "Thread " + std::to_string(id) + " starting...\n");
      while (!m_SignalStop) {
        {
          std::unique_lock<std::mutex> grd(m_JobMutex);
          if (!m_ThreadJobs.empty()) {
            ThreadJob job = m_ThreadJobs.front();
            job.CurrThreadId = id;
            job.Result = ThrResultInProgress;

            {
              m_ThreadJobs.pop();    
            }

            grd.unlock();

            {
              std::unique_lock<std::mutex> grd(m_ProgressMutex);
              m_ProgressJobs.push_back(job);
              a_BusyThreadCount++;
            }

            if (job.Work) { job.Work(); }

            {
              std::unique_lock<std::mutex> grd(m_ProgressMutex);
              for (auto it = m_ProgressJobs.begin(); it != m_ProgressJobs.end(); ++it) {
                if (it->CurrThreadId == id) {
                  m_ProgressJobs.erase(it);
                  --a_BusyThreadCount;
                  break;
                }
              }   
            }
          }
        }
      }
    }, static_cast<thread_id_t>(i));
  }
}


void ThreadPool::WaitAll()
{
  // Spin lock to acquire the lock. This will cause the main thread to wait for threads to 
  // finish their work, before moving forward.
  while (true) {
    // Check for any busy threads.
    R_DEBUG(rVerbose, "Spinlock syncing...\n");
    if (!a_BusyThreadCount) { 
      break;
    }
  }
  R_DEBUG(rVerbose, "Thread sync complete.\n");
}


void ThreadPool::AddTask(thr_work_func_t func)
{
  ThreadJob job;
  job.CurrThreadId = 0;
  job.Result = ThrResultIncomplete;
  job.Work = func;

  m_ThreadJobs.push(job);
}


void ThreadPool::StopAll()
{
  m_SignalStop = true;
  for (size_t i = 0; i < m_ThreadWorkers.size(); ++i) {
    m_ThreadWorkers[i].Join();
  }
}


void ThreadPool::ClearTasks()
{
  while (!m_ThreadJobs.empty()) {
    m_ThreadJobs.pop();
  }
}
} // Recluse