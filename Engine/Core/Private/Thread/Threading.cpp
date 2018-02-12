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
          if (!m_ThreadJobs.empty()) {
            ThreadJob job = m_ThreadJobs.front();
            job.CurrThreadId = id;
            job.Result = ThrResultInProgress;

            {
              std::lock_guard<std::mutex> grd(m_JobMutex);
              m_ThreadJobs.pop();
              
            }

            {
              std::lock_guard<std::mutex> grd(m_ProgressMutex);
              m_ProgressJobs.push_back(job);
            }

            if (job.Work) { job.Work(); }

            {
              std::lock_guard<std::mutex> grd(m_ProgressMutex);
              for (auto it = m_ProgressJobs.begin(); it != m_ProgressJobs.end(); ++it) {
                if (it->CurrThreadId == id) {
                  m_ProgressJobs.erase(it);
                  break;
                }
              }   
            }
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }, static_cast<thread_id_t>(i));
  }
}


void ThreadPool::WaitAll()
{
  while (!m_ThreadJobs.empty() || !m_ProgressJobs.empty()) { }
}


void ThreadPool::AddTask(thr_work_func_t func)
{
  ThreadJob job;
  job.CurrThreadId = 0;
  job.Result = ThrResultIncomplete;
  job.Work = func;

  m_JobMutex.lock();
  m_ThreadJobs.push(job);
  m_JobMutex.unlock();
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