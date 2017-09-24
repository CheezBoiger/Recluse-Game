// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"


namespace Recluse {


class RenderCmd;


typedef void (*RenderCmdCompareFunc)(RenderCmd* cmd1, RenderCmd* cmd2);


class CmdList {
public:
  CmdList(size_t size = 1024)
    : mRenderCmdList(size) { }

  size_t                  Size() const { return mRenderCmdList.size(); }
  RenderCmd*              operator[](size_t i) { return mRenderCmdList[i]; }

  void                    Resize(size_t newSize) { mRenderCmdList.resize(newSize); }
  void                    SetSortFunc(RenderCmdCompareFunc compare) { mCompare = compare; }
private:
  std::vector<RenderCmd*> mRenderCmdList;
  RenderCmdCompareFunc    mCompare;
};
} // Recluse