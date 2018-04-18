// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Component.hpp"
#include "Engine.hpp"
#include "GameObject.hpp"

#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


DEFINE_COMPONENT_MAP(Transform);


Transform* Component::GetTransform()
{
  if (!GetOwner()) return nullptr;
  return GetOwner()->GetTransform();
}


void Transform::Update()
{
  if (LocalPosition == Position && LocalRotation == Rotation && LocalScale == Scale && !GetOwner()->GetParent()) return;

  GameObject* parent = GetOwner()->GetParent();

  if (parent) {
    Matrix4 _T = Matrix4::Translate(Matrix4::Identity(), LocalPosition);
    Matrix4 _R = LocalRotation.ToMatrix4();
    Matrix4 _S = Matrix4::Scale(Matrix4::Identity(), LocalScale);
    Matrix4 localToWorldMatrix = _S * _R * _T;
    m_LocalToWorldMatrix = localToWorldMatrix;
    Transform* parentTransform = parent->GetTransform();
    if (parentTransform) { 
      m_LocalToWorldMatrix =  m_LocalToWorldMatrix * parentTransform->GetLocalToWorldMatrix();
    } 

    Position = Vector3(m_LocalToWorldMatrix[3][0], m_LocalToWorldMatrix[3][1], m_LocalToWorldMatrix[3][2]);
  } else {
    LocalPosition = Position;
    LocalRotation = Rotation;
    LocalScale = Scale;
    Matrix4 _T = Matrix4::Translate(Matrix4::Identity(), LocalPosition);
    Matrix4 _R = LocalRotation.ToMatrix4();
    Matrix4 _S = Matrix4::Scale(Matrix4::Identity(), LocalScale);
    Matrix4 localToWorldMatrix = _S * _R * _T;
    m_LocalToWorldMatrix = localToWorldMatrix;
  }

  // Update local coordinates.
  Vector3 u = Vector3(LocalRotation.x, LocalRotation.y, LocalRotation.z);
  r32 s = LocalRotation.w;
  m_Front = u * (u.Dot(Vector3::FRONT) * 2.0f)  + (Vector3::FRONT * (s*s - u.Dot(u))) + ((u ^ Vector3::FRONT) * s * 2.0f);
  m_Right = u * (u.Dot(Vector3::RIGHT) * 2.0f)  + (Vector3::RIGHT * (s*s - u.Dot(u))) + ((u ^ Vector3::RIGHT) * s * 2.0f);
  m_Up =    u * (u.Dot(Vector3::UP) * 2.0f)     + (Vector3::UP * (s*s - u.Dot(u)))    + ((u ^ Vector3::UP) * s * 2.0f);

  m_WorldToLocalMatrix = m_LocalToWorldMatrix.Inverse();
}


void Transform::OnInitialize(GameObject* owner)
{
  REGISTER_COMPONENT(Transform, this);
}


void Transform::OnCleanUp()
{
  UNREGISTER_COMPONENT(Transform);
}
} // Recluse