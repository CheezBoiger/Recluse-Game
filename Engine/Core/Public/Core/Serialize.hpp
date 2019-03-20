// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

namespace Recluse {


class IArchive;


class ISerializable {
public:
  virtual ~ISerializable() { }

  virtual b8 Serializable() { return true; }

  virtual void serialize(IArchive& archive) { }

  virtual void deserialize(IArchive& archive) { }
};
} // Recluse