// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Component.hpp"
#include "Engine.hpp"
#include "GameObject.hpp"

#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


Transform* Component::getTransform()
{
  if (!getOwner()) return nullptr;
  return getOwner()->getTransform();
}


void Transform::update()
{
  GameObject* parent = getOwner()->GetParent();
  if (parent) {
    Transform* parentTransform = parent->getTransform();
    Matrix4 _T = Matrix4::translate(Matrix4::identity(), _localPosition);
    Matrix4 _R = _localRotation.toMatrix4();
    Matrix4 _S = Matrix4::scale(Matrix4::identity(), _localScale);
    Matrix4 localToWorldMatrix = _S * _R * _T;
    m_localToWorldMatrix = localToWorldMatrix * parentTransform->getLocalToWorldMatrix();

    _position = Vector3(m_localToWorldMatrix[3][0], m_localToWorldMatrix[3][1], m_localToWorldMatrix[3][2]);
    _rotation = _localRotation * parentTransform->_rotation;
  } else {
    Matrix4 _T = Matrix4::translate(Matrix4::identity(), _position);
    Matrix4 _R = _rotation.toMatrix4();
    Matrix4 _S = Matrix4::scale(Matrix4::identity(), _scale);
    Matrix4 localToWorldMatrix = _S * _R * _T;
    m_localToWorldMatrix = localToWorldMatrix;
  }

  // Update local coordinates.
  Vector3 u = Vector3(_rotation.x, _rotation.y, _rotation.z);
  R32 s = _rotation.w;
  m_front = u * (u.dot(Vector3::FRONT) * 2.0f)  + (Vector3::FRONT * (s*s - u.dot(u))) + ((u ^ Vector3::FRONT) * s * 2.0f);
  m_right = u * (u.dot(Vector3::RIGHT) * 2.0f)  + (Vector3::RIGHT * (s*s - u.dot(u))) + ((u ^ Vector3::RIGHT) * s * 2.0f);
  m_up =    u * (u.dot(Vector3::UP) * 2.0f)     + (Vector3::UP * (s*s - u.dot(u)))    + ((u ^ Vector3::UP) * s * 2.0f);

  m_worldToLocalMatrix = m_localToWorldMatrix.inverse();
}
} // Recluse