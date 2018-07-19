// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Image.hpp"


namespace Recluse {


class DescriptorSet;


class UIDescriptor {
public:

  // TODO(): For UI Images and textures to be sent to the UI Overlay. May also contain transformation
  // details for it as well..

private:

  DescriptorSet* m_pSet;
};
} // Recluse