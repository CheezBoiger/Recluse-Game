// Copyright (c) 2018 Recluse Project. All rights reserved.

// Simple Move Object Script for Game Test.
#include "Game/Engine.hpp"
#include "Game/MeshComponent.hpp"
#include "Game/Scene/Scene.hpp"


using namespace Recluse;

// Simple Move Object Script, allows the user to input key press calls
// to move our object in 3D space. This is a simple demo to test how
// scripts work in the engine.
class MoveScript : public IScript {
public:

  // Initialization stage of the script component.
  // This is called once on Wake().
  void Awake() override {
    Transform* transform = GetOwner()->GetTransform();
    transform->Rotation = Quaternion::AngleAxis(Radians(0.0f), Vector3::UP);
    transform->Scale = Vector3(5.0f, 5.0f, 5.0f);
  }

  // This function is called every frame.
  void Update() override {
    Transform* transform = GetOwner()->GetTransform();
    r32 sDt = static_cast<r32>(Time::DeltaTime * Time::ScaleTime);
    if (Keyboard::KeyPressed(KEY_CODE_UP_ARROW)) {
      transform->Position += transform->Forward() * 5.0f * sDt;
    }
    if (Keyboard::KeyPressed(KEY_CODE_DOWN_ARROW)) {
      transform->Position -= transform->Forward() * 5.0f * sDt;
    }
    if (Keyboard::KeyPressed(KEY_CODE_LEFT_ARROW)) {
      transform->Rotation *= Quaternion::AngleAxis(-Radians(20.0f * sDt), Vector3::UP);
    }
    if (Keyboard::KeyPressed(KEY_CODE_RIGHT_ARROW)) {
      transform->Rotation *= Quaternion::AngleAxis(Radians(20.0f * sDt), Vector3::UP);
    }
    if (Keyboard::KeyPressed(KEY_CODE_Q)) {
      transform->Rotation *= Quaternion::AngleAxis(Radians(20.0f * sDt), Vector3::RIGHT);
    }
    if (Keyboard::KeyPressed(KEY_CODE_E)) {
      transform->Rotation *= Quaternion::AngleAxis(-Radians(20.0f * sDt), Vector3::RIGHT);
    }
    //transform->Rotation *= Quaternion::AngleAxis(Radians(20.0f * sDt), Vector3::FRONT);
  }
};