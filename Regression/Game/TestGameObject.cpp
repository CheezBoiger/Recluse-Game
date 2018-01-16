// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "TestGameObject.hpp"
#include "../Tester.hpp"
#include "Core/Exception.hpp"
#include "Core/Utility/Archive.hpp"
#include "Game/GameObject.hpp"
#include "Game/RendererComponent.hpp"
#include "Game/Engine.hpp"


namespace Test {

Vector3 TestPosition = Vector3(1.0f, 1.22f, -7.0222022f);


// Testing the script maker.
class TestScript : public IScript {
  RSCRIPT(TestScript);
public:

  void Awake() override 
  {
    Log() << "I am awake!\n";  
  }
};

void AddRendererComponents(GameObject* obj)
{
  if (!obj) {
    return;
  }

  obj->AddComponent<RendererComponent>();  
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
    AddRendererComponents(gameObj);
    AddRendererComponents(gameObj2);

    RendererComponent* component = gameObj->GetComponent<RendererComponent>();
    RendererComponent* component2 = gameObj2->GetComponent<RendererComponent>();
    RendererComponent* component3 = gameObj3->GetComponent<RendererComponent>();

    if (!component || !component2 || component3) {
      Log(rError) << "Failed to create component!";
      return false;
    }

    TASSERT_E(gameObj, component->GetOwner());
    TASSERT_E(gameObj2, component2->GetOwner());
    TASSERT_E(nullptr, component3);

    Log(rNotify) << "Removing Skinned Mesh Component from gameObj.\n";
    gameObj->DestroyComponent<RendererComponent>();
    RendererComponent* destroyed = gameObj->GetComponent<RendererComponent>();
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

    gameObj->AddComponent<TestScript>();
    gameObj->AddComponent<TestScript>();

    Archive archive;
    gameObj->Serialize(archive);

    // NOTE(): Possible memory leak when clearing all?
    GameObject::DestroyAll();

    Log() << "Finished with game object test.\n";
  }
  return true;
}
} // Test