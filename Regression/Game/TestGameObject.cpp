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


class TestObject : public GameObject 
{
public:

  virtual void OnStartUp() override
  {
    m_pMeshComponent = new MeshComponent();
    m_pMaterialComponent = new MaterialComponent();
    m_pRendererComponent = new RendererComponent();
    
    m_pMeshComponent->Initialize(this);
    m_pMaterialComponent->Initialize(this);

    m_pRendererComponent->Initialize(this);
    m_pRendererComponent->SetMeshComponent(m_pMeshComponent);
  }


  virtual void Update(r32 tick) override
  {
    Transform* transform = GetTransform();
    transform->Position += Vector3(1.0f, 0.0f, 0.0f) * tick;

    Log() << "Position: " << transform->Position;
  }


  virtual void OnCleanUp() override 
  {
    m_pMeshComponent->CleanUp();
    m_pMaterialComponent->CleanUp();
    m_pRendererComponent->CleanUp();

    delete m_pMeshComponent;
    delete m_pMaterialComponent;
    delete m_pRendererComponent;
  }

private:
  RendererComponent*  m_pRendererComponent;
  MaterialComponent*  m_pMaterialComponent;
  MeshComponent*      m_pMeshComponent;
};


b8 TestGameObject()
{

  return true;
}
} // Test