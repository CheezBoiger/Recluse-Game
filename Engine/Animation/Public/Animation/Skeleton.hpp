// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Serialize.hpp"
#include "Core/Math/Matrix4.hpp"

#include <map>
#include <vector>

namespace Recluse {

typedef i32      skeleton_uuid_t;


// Joint represents the overall skeleton pose of our model. 
// this information defines how the rig of an animation is posed at rest.
struct Joint {
  static const u8 kNoParentId = 0xff;

  Matrix4       _InvBindPose;     // Bind pose transform of this joint.
  Matrix4       _invGlobalTransform;  // Bind shape matrix, transform that is relative to its parent.
  std::string   _name;            // name of the joint.
  u8            _iParent;         // Joint parent, represented as an id in byte form.
  DEBUG_OP(u8   _id);             // node id test.
};


// Represents the skinned animation of a mesh object in a game.
// The skeleton defines all joints and transformation offsets 
// that make up the animation within the game.
// This is a simple skeleton layout for the architecture of our 
// engine.
struct Skeleton {
private:
  static skeleton_uuid_t                          kCurrSkeleCount;
  static std::map<skeleton_uuid_t, Skeleton>      kSkeletons;
public:

  static const skeleton_uuid_t      kNoSkeletonId = -1;

  static Skeleton*            GetSkeleton(skeleton_uuid_t id) { return &kSkeletons[id]; }
  static void                 PushSkeleton(const Skeleton& skele) { kSkeletons[skele._uuid] = skele; }
  static void                 RemoveSkeleton(skeleton_uuid_t id) { 
    auto it = kSkeletons.find(id);
    if (it == kSkeletons.end()) return;
    kSkeletons.erase(it);
  }

  Skeleton()
    : _uuid(kCurrSkeleCount++) { }

  size_t                      NumJoints() const { return _joints.size(); }

  // The name of this skeleton.
  std::string                 _name;
  // Full joint transformation that corresponds to a bone.
  std::vector<Joint>          _joints;
  // This skeleton's unique id.
  skeleton_uuid_t             _uuid;
  b32                         _rootInJoints;
};
} // Recluse