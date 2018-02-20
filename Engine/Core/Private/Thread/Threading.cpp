// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Thread/Threading.hpp"

#include "Logging/Log.hpp"
#include "Exception.hpp"


namespace Recluse {


thread_id_t Thread::currentId = 0;


void Thread::Run(thread_func_t entry, thread_id_t id)
{
  m_Id = id;
  mThread = std::thread(entry, id);
}


void ThreadPool::RunAll()
{
  m_SignalStop = false;
  for (size_t i = 0; i < m_ThreadWorkers.size(); ++i) {
    m_ThreadWorkers[i].Run([&] (thread_id_t id) -> void {
      R_DEBUG(rNotify, "Thread " + std::to_string(i) + " starting...\n");
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
            }

            if (job.Work) { job.Work(); }

            {
              std::unique_lock<std::mutex> grd(m_ProgressMutex);
              for (auto it = m_ProgressJobs.begin(); it != m_ProgressJobs.end(); ++it) {
                if (it->CurrThreadId == id) {
                  m_ProgressJobs.erase(it);
                  break;
                }
              }   
            }
          }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
    }, static_cast<thread_id_t>(i));
  }
}


void ThreadPool::WaitAll()
{
  // Spin lock to acquire the lock. This will cause the main thread to wait for threads to 
  // finish their work, before moving forward.
  while (true) {
    std::unique_lock<std::mutex> lck(m_JobMutex);
    if (lck.owns_lock() && m_ThreadJobs.empty()) {
      break;
    }
  }
}


void ThreadPool::AddTask(thr_work_func_t func)
{
  ThreadJob job;
  job.CurrThreadId = 0;
  job.Result = ThrResultIncomplete;
  job.Work = func;

  std::unique_lock<std::mutex> grd(m_JobMutex);
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