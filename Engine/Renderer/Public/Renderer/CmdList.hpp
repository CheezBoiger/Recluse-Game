// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once 

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"


namespace Recluse {


class RenderCmd;


typedef bool (*RenderCmdCompareFunc)(const RenderCmd& cmd1, const RenderCmd& cmd2);


class CmdList {
public:
  CmdList(size_t size = 1024)
    : mRenderList(size) { }

  size_t                  Size() const { return mRenderList.size(); }
  RenderCmd*              operator[](size_t i) { return mRenderList[i]; }

  void                    Resize(size_t newSize) { mRenderList.resize(newSize); }
  void                    SetSortFunc(RenderCmdCompareFunc compare) { mCompare = compare; }
  void                    Sort();
private:
  std::vector<RenderCmd*> mRenderList;
  RenderCmdCompareFunc    mCompare;
};
} // Recluse