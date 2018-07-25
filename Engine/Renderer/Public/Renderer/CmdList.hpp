// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "RenderCmd.hpp"

#include <functional>
#include <algorithm>


namespace Recluse {


struct MeshRenderCmd;


template<typename Cmd>
class CmdList {
public:
  typedef std::function<bool(Cmd& cmd1, Cmd& cmd2)> CmdCompareFunc;
  CmdList(size_t size = 0)
    : mCmdList(size)
    , mCompare(nullptr)
    , m_currIdx(0) { } 

  size_t                  Size() const { return m_currIdx; }
  Cmd&                    operator[](size_t i) { return mCmdList[i]; }
  Cmd&                    Get(size_t i) { return mCmdList[i]; }

  void                    Resize(size_t newSize) { mCmdList.resize(newSize); }
  void                    SetSortFunc(CmdCompareFunc compare) { mCompare = compare; }

  // Push back an object. Returns the index of the object stored in this structure.
  size_t                  PushBack(Cmd cmd) { if (m_currIdx >= mCmdList.size()) { Resize(mCmdList.size() << 1); } mCmdList[m_currIdx] = cmd; return m_currIdx++; }
  void                    Erase(u32 i) { mCmdList.erase(mRenderList.begin() + i); }
  // Sort using alg.
  void                    Sort() { if (mCompare && m_currIdx > 0) std::sort(mCmdList.begin(), mCmdList.begin() + (m_currIdx - 1), mCompare); }
  void                    Clear() { m_currIdx = 0; }

private:
  std::vector<Cmd>        mCmdList;
  CmdCompareFunc         mCompare;
  b32                     mDirty;
  u32                    m_currIdx;
};
} // Recluse