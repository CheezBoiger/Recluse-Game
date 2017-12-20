// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "TestGameObject.hpp"
#include "../Tester.hpp"
#include "Core/Exception.hpp"
#include "Core/Utility/Archive.hpp"
#include "Game/GameObject.hpp"
#include "Game/SkinnedMeshComponent.hpp"


namespace Test {

Vector3 TestPosition = Vector3(1.0f, 1.22f, -7.0222022f);

void AddSkinnedMeshComponents(GameObject* obj)
{
  if (!obj) {
    return;
  }

  obj->AddComponent<SkinnedMeshComponent>();  
}


b8 TestGameObject()
{
  {
    GameObject* gameObj = nullptr;
    GameObject* gameObj2 = nullptr;
    GameObject* gameObj3 = nullptr;
    {
      gameObj = GameObject::Instantiate();
      gameObj2 = GameObject::Instantiate(); 
      gameObj3 = GameObject::Instantiate();
    }

    // Only gameObj and gameObj2 get a skinned mesh component.
    AddSkinnedMeshComponents(gameObj);
    AddSkinnedMeshComponents(gameObj2);

    SkinnedMeshComponent* component = gameObj->GetComponent<SkinnedMeshComponent>();
    SkinnedMeshComponent* component2 = gameObj2->GetComponent<SkinnedMeshComponent>();
    SkinnedMeshComponent* component3 = gameObj3->GetComponent<SkinnedMeshComponent>();

    if (!component || !component2 || component3) {
      Log(rError) << "Failed to create component!";
      return false;
    }

    TASSERT_E(gameObj, component->GetOwner());
    TASSERT_E(gameObj2, component2->GetOwner());
    TASSERT_E(nullptr, component3);

    Log(rNotify) << "Removing Skinned Mesh Component from gameObj.\n";
    gameObj->DestroyComponent<SkinnedMeshComponent>();
    SkinnedMeshComponent* destroyed = gameObj->GetComponent<SkinnedMeshComponent>();
    if (destroyed) {
      Log(rError) << "Failed to destory skinned mesh component from gameObj!\n";
      return false;
    }

    Transform* transform = gameObj->GetComponent<Transform>();
    TASSERT_E(transform, nullptr);

    {
      gameObj->AddComponent<Transform>();
    }

    transform = gameObj->GetTransform();
    TASSERT_NE(transform, nullptr);
    transform->Position = TestPosition;
  
    Log(rNotify) << "Testing duplicate component avoidance...\n";
    gameObj->AddComponent<Transform>();

    Log(rNotify) << "Testing component comparison with game object.\n";
    Transform* testTransform = gameObj->GetComponent<Transform>();
    TASSERT_E(testTransform->Position, TestPosition);
    TASSERT_E(transform->Position, testTransform->Position);

    Archive archive;
    gameObj->Serialize(archive);

    // NOTE(): Possible memory leak when clearing all?
    GameObject::DestroyAll();

    Log() << "Finished with game object test.\n";
  }
  
  return true;
}
} // Test