// Copyright (c) 2017 Recluse Project. All rights reserved.
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
    , mCompare(nullptr) { } 

  size_t                  Size() const { return mRenderList.size(); }
  RenderCmd&              operator[](size_t i) { return mRenderList[i]; }
  RenderCmd&              Get(size_t i) { return mRenderList[i]; }

  void                    Resize(size_t newSize) { mRenderList.resize(newSize); }
  void                    SetSortFunc(RenderCmdCompareFunc compare) { mCompare = compare; }
  void                    PushBack(RenderCmd cmd) { mRenderList.push_back(cmd); }
  void                    Erase(u32 i) { mRenderList.erase(mRenderList.begin() + i); }
  // Sort using alg.
  void                    Sort();  
  void                    Clear() { mRenderList.clear(); }

private:
  std::vector<RenderCmd>  mRenderList;
  RenderCmdCompareFunc    mCompare;
  b32                      mDirty;
};
} // Recluse