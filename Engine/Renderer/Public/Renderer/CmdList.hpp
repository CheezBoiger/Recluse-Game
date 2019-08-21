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
  CmdList(size_t size = 1)
    : mCmdList(size)
    , mCompare(nullptr)
    , m_currIdx(0) { } 

  size_t                  Size() const { return m_currIdx; }
  Cmd&                    operator[](size_t i) { return mCmdList[i]; }
  Cmd&                    get(size_t i) { return mCmdList[i]; }

  void                    resize(size_t newSize) { mCmdList.resize(newSize); }
  void                    setSortFunc(CmdCompareFunc compare) { mCompare = compare; }

  // Push back an object. Returns the index of the object stored in this structure.
  size_t                  pushBack(Cmd cmd) { if (m_currIdx >= mCmdList.size()) { resize(mCmdList.size() << 1); } mCmdList[m_currIdx] = cmd; return m_currIdx++; }
  void                    erase(U32 i) { mCmdList.erase(mRenderList.begin() + i); }
  // Sort using alg.
  void                    sort() { if (mCompare && m_currIdx > 0) std::sort(mCmdList.begin(), mCmdList.begin() + (m_currIdx), mCompare); }
  void                    clear() { m_currIdx = 0; }

  const Cmd*              getData() { return mCmdList.data(); }
private:
  std::vector<Cmd>        mCmdList;
  CmdCompareFunc         mCompare;
  B32                     mDirty;
  U32                    m_currIdx;
};
} // Recluse