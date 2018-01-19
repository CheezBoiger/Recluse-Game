// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"


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
  // Sort using alg.
  void                    Sort();  
  void                    Clear() { mRenderList.clear(); }

private:
  std::vector<RenderCmd>  mRenderList;
  RenderCmdCompareFunc    mCompare;
  b8                      mDirty;
};
} // Recluse