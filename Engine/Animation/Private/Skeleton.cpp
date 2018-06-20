// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Skeleton.hpp"
#include "Animation.hpp"

#include "Core/Exception.hpp"
#include "Core/Math/Matrix4.hpp"

namespace Recluse {


skeleton_uuid_t                     Skeleton::kCurrSkeleCount             = 0;
std::map<skeleton_uuid_t, Skeleton> Skeleton::kSkeletons;
} // Recluse