// Copyright (c) 2018 Recluse Project. All rights reserved.

// Simple Orbiting Object Script for Game Test.
#include "Game/Engine.hpp"
#include "Game/MeshComponent.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/RendererComponent.hpp"
#include "Game/PointLightComponent.hpp"

using namespace Recluse;


// Script to orbit a game object. This is a test demo script.
class OrbitObjectScript : public IScript {
  RSCRIPT(OrbitObjectScript);
public:
  r32     acc;

  void Awake() override
  {
    Log() << "Waking: " + GetOwner()->GetName() + "\n";
    acc = 0.0f;
    if (GetOwner()->GetParent()) {
      GetOwner()->GetTransform()->LocalScale = Vector3(0.4f, 0.4f, 0.4f);
      GetOwner()->GetTransform()->LocalPosition = Vector3(2.0f, 0.0f, 0.0f);
    }

    Texture2D* emission = nullptr;
    TextureCache::Get("BoxEmissive", &emission);

    RendererComponent* rc = GetOwner()->GetComponent<RendererComponent>();
    Material* mat = GetOwner()->GetComponent<MaterialComponent>()->GetMaterial();
    mat->SetEmissive(emission);
    mat->SetBaseMetal(0.0f);
    mat->SetBaseRough(1.0f);
    mat->SetBaseEmissive(10.0f);
    mat->EnableEmissive(true);
    rc->ReConfigure();

    // Add a point light.
    GetOwner()->AddComponent<PointLightComponent>();
    PointLightComponent* pointLight = GetOwner()->GetComponent<PointLightComponent>();
    pointLight->SetRange(50.0f);
    pointLight->SetIntensity(10.0f);
  }

  void Update() override
  {
#if 1
    // Test a swirling sphere...
    Transform* transform = GetOwner()->GetTransform();
    r32 sDt = static_cast<r32>(Time::FixTime * Time::ScaleTime);
    // If object has parent, swirl in it's local position.
    if (GetOwner()->GetParent()) {
      transform->LocalPosition.x = transform->LocalPosition.x
        - sinf(Radians(acc)) * 1.0f * sDt;
      transform->LocalPosition.y = transform->LocalPosition.y
        - cosf(Radians(acc)) * 1.0f * sDt;
      // Calculates the curvature.
      acc += 30.0f * sDt;

      transform->LocalRotation *= Quaternion::AngleAxis(Radians(20.0f * sDt), Vector3::UP);
    }
#endif
  }
};