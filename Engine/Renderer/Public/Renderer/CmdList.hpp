// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"
#include "RenderCmd.hpp"

#include <functional>


namespace Recluse {


struct RenderCmd;


typedef std::function<bool(RenderCmd& cmd1, RenderCmd& cmd2)> RenderCmdCompareFunc;


class CmdList {
public:
  CmdList(size_t size = 0)
    : mRenderList(size)
    , mCompare(nullptr)
    , m_currIdx(0) { } 

  size_t                  Size() const { return m_currIdx; }
  RenderCmd&              operator[](size_t i) { return mRenderList[i]; }
  RenderCmd&              Get(size_t i) { return mRenderList[i]; }

  void                    Resize(size_t newSize) { mRenderList.resize(newSize); }
  void                    SetSortFunc(RenderCmdCompareFunc compare) { mCompare = compare; }
  void                    PushBack(RenderCmd cmd) { if (m_currIdx >= mRenderList.size()) { Resize(mRenderList.size() << 1); } mRenderList[m_currIdx++] = cmd; }
  void                    Erase(u32 i) { mRenderList.erase(mRenderList.begin() + i); }
  // Sort using alg.
  void                    Sort();  
  void                    Clear() { m_currIdx = 0; }

private:
  std::vector<RenderCmd>  mRenderList;
  RenderCmdCompareFunc    mCompare;
  b32                      mDirty;
  u32                     m_currIdx;
};
} // Recluse