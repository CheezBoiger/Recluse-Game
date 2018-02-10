// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Serialize.hpp"


namespace Recluse {


class MeshData;
class Mesh;
class Material;

class MeshCache {
public:

  void      CleanUp();
};


class MaterialCache {
public:

  void      CleanUp();
};
} // Recluse