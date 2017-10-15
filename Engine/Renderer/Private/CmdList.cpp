// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "CmdList.hpp"
#include "Core/Exception.hpp"

#include "RenderCmd.hpp"
#include <algorithm>


namespace Recluse {


void CmdList::Sort()
{
  if (mCompare) std::sort(mRenderList.begin(), mRenderList.end(), mCompare);
}
} // Recluse