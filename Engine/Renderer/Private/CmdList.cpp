// Copyright (c) 2017-2018 Recluse Project. All rights reserved.
#include "CmdList.hpp"
#include "Core/Exception.hpp"

#include "RenderCmd.hpp"
#include "Core/Logging/Log.hpp"
#include <algorithm>


namespace Recluse {


  void CmdList::Sort()
  {
    if (mCompare && m_currIdx > 0) std::sort(mRenderList.begin(), mRenderList.begin() + (m_currIdx - 1), mCompare);
  }
} // Recluse