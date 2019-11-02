// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Camera.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Math/Common.hpp"

#include "Engine.hpp"


namespace Recluse {


Camera* Camera::s_pMainCamera = nullptr;


Camera* Camera::getMain()
{
  return s_pMainCamera;
}


void Camera::setMain(Camera* pCam)
{
  if (!pCam) return;
  s_pMainCamera = pCam;

  // TODO(): Need to remove previous frustum from engine list.
  gEngine().addFrustum(&pCam->m_viewFrustum);
}


Camera::Camera(Project type, R32 fov, R32 zNear, R32 zFar)
  : m_ProjType(type)
  , m_Fov(fov)
  , m_PixelWidth(1.0f)
  , m_PixelHeight(1.0f)
  , m_Aspect(1.0f / 1.0f)
  , m_ZNear(zNear)
  , m_ZFar(zFar)
  , m_OrthoScale(1.0f)
  , m_Bloom(false)
  , m_Gamma(2.2f)
  , m_Exposure(4.2f)
  , m_filmGrainSpeed(1.0f)
  , m_filmGrainZoom(2.0f)
  , m_FrustumCull(false)
  , m_interleaveVideo(false)
  , m_filmGrain(false)
{
}


void Camera::resetAspect()
{
  m_Aspect = m_PixelWidth / m_PixelHeight;
}


void Camera::update()
{
  if (!getOwner()) return;

  Transform* transform = getOwner()->getTransform();
  // NOTE(): This will effectively cause the game object transform to update twice! 
  // One here, and the other during Scene traversal in Engine object!
  transform->update(); 
  Vector3 pos = transform->_position;

// Testing how to go about water rendering.
/*
  if (Keyboard::KeyPressed(KEY_CODE_LEFT_ARROW)) {
  float dist = 2.0f * pos.y;
  pos.y = pos.y - dist;
  Vector3 euler = transform->_rotation.toEulerAngles();
  euler.x *= -1.0f;
  transform->_rotation = Quaternion::eulerAnglesToQuaternion(euler);
  transform->Update();
  }
*/
  Vector3 right = transform->right();
  Vector3 up = transform->up();
  Vector3 front = transform->front();

  // Update camera and screen info.
  Window* pWindow = gEngine().getWindow();

  m_viewMatrix = Matrix4(
     right.x,          up.x,         front.x,        0.0f,
     right.y,          up.y,         front.y,        0.0f,
     right.z,          up.z,         front.z,        0.0f,
    -right.dot(pos),  -up.dot(pos), -front.dot(pos), 1.0f
  );

  m_projectionMatrix = Matrix4();
  if (m_ProjType == ORTHO) {
    m_projectionMatrix = Matrix4::ortho((m_PixelWidth * 0.01f) * m_OrthoScale,
      (m_PixelHeight * 0.01f) * m_OrthoScale,
      m_ZNear,
      m_ZFar);
  } else {
    m_projectionMatrix = Matrix4::perspective(m_Fov, m_Aspect, m_ZNear, m_ZFar);
  }

  R32 winPixWidth = R32(gRenderer().getRenderWidth());//R32(pWindow->getWidth());
  R32 winPixHeight = R32(gRenderer().getRenderHeight());//R32(pWindow->getHeight());
  if (m_PixelHeight != winPixHeight || m_PixelWidth != winPixWidth) {
    m_PixelWidth = winPixWidth;
    m_PixelHeight = winPixHeight;
    m_Aspect = (m_PixelWidth / m_PixelHeight);
  }
}


void Camera::flushToGpuBus()
{
  GlobalBuffer* gGlobalBuffer = gRenderer().getGlobalData();
  ConfigHDR* hdr = gRenderer().getHDR()->getRealtimeConfiguration();
  Window* pWindow = gEngine().getWindow();

  Transform* transform = getOwner()->getTransform();
  Vector3 pos = transform->_position;
  Vector3 right = transform->right();
  Vector3 up = transform->up();
  Vector3 front = transform->front();

  gGlobalBuffer->_CameraPos = Vector4(pos, 1.0f);
  gGlobalBuffer->_Proj = m_projectionMatrix;
  gGlobalBuffer->_View = m_viewMatrix;
  gGlobalBuffer->_ViewProj = gGlobalBuffer->_View * gGlobalBuffer->_Proj;
  gGlobalBuffer->_InvViewProj = gGlobalBuffer->_ViewProj.inverse();
  gGlobalBuffer->_InvView = gGlobalBuffer->_View.inverse();
  gGlobalBuffer->_InvProj = gGlobalBuffer->_Proj.inverse();
  gGlobalBuffer->_zFar = m_ZFar;
  gGlobalBuffer->_zNear = m_ZNear;
  gGlobalBuffer->_ScreenSize[0] = gRenderer().getRenderWidth( );
  gGlobalBuffer->_ScreenSize[1] = gRenderer().getRenderHeight( );
  gGlobalBuffer->_BloomEnabled = getBloom();
  gGlobalBuffer->_Exposure = getExposure();
  gGlobalBuffer->_Gamma = getGamma();
  gGlobalBuffer->_MousePos = Vector2((R32)Mouse::getX(), (R32)Mouse::getY());
  gGlobalBuffer->_fEngineTime = static_cast<R32>(Time::currentTime());
  gGlobalBuffer->_fDeltaTime = static_cast<R32>(Time::deltaTime);
  gGlobalBuffer->_fScaleTime = static_cast<R32>(Time::scaleTime);

  // TODO(): Move cam frustum to camera.
  m_viewFrustum.update(gGlobalBuffer->_ViewProj);
  gGlobalBuffer->_LPlane = m_viewFrustum[ViewFrustum::PLEFT];
  gGlobalBuffer->_RPlane = m_viewFrustum[ViewFrustum::PRIGHT];
  gGlobalBuffer->_BPlane = m_viewFrustum[ViewFrustum::PBOTTOM];
  gGlobalBuffer->_TPlane = m_viewFrustum[ViewFrustum::PTOP];
  gGlobalBuffer->_NPlane = m_viewFrustum[ViewFrustum::PNEAR];
  gGlobalBuffer->_FPlane = m_viewFrustum[ViewFrustum::PFAR];

  // HDR settings.
  hdr->_bEnable.x = R32(getInterleavedVideo());
  hdr->_bEnable.z = R32(getFilmGrain());
  hdr->_filmGrain = Vector4(m_filmGrainZoom, m_filmGrainSpeed, 0.0f, 0.0f);
}


Vector3 Camera::getWorldToScreenProjection(const Vector3& position)
{
  Vector4 sPos = Vector4(position, 1.0f) * m_viewMatrix;
  if (sPos.z < 0.0f) return Vector3(-1.0f, -1.0f, 0.0f);
  sPos = sPos * m_projectionMatrix;
  // Perspective divide to get clip space coordinates.
  sPos /= sPos.z;
  
  // Multiply by screen size to get screen position.
  sPos.x = (sPos.x + 1.0f) * m_PixelWidth / 2.0f;
  sPos.y = (sPos.y + 1.0f) * m_PixelHeight / 2.0f;
  return Vector3(sPos.x, sPos.y, 0.0f);
}

#if 0
FlyViewCamera::FlyViewCamera(R32 fov, R32 pixelWidth, R32 pixelHeight, 
  R32 zNear, R32 zFar, Vector3 pos, Vector3 dir)
  : m_X_Sensitivity(0.1f)
  , m_Y_Sensitivity(0.1f)
  , m_Yaw(90.0f)
  , m_Pitch(0.0f)
  , m_ConstainedPitch(89.0f)
  , m_FirstLook(true)
  , m_LastX(0.0f)
  , m_LastY(0.0f)
  , m_Speed(5.0f)
  , m_Locked(false)
  , Camera(PERSPECTIVE, fov, pixelWidth, pixelHeight, zNear, zFar, pos, dir)
{
  // TODO(): Need to set the yaw and pitch to the direction of where initial look at is.
}


Matrix4 FlyViewCamera::getView()
{
  return Matrix4();
}


void FlyViewCamera::Move(Movement movement, R64 dt)
{
}


void FlyViewCamera::update()
{
}


void FlyViewCamera::Look(R64 x, R64 y)
{
  if (m_FirstLook) {
    m_LastX = (R32)x;
    m_LastY = (R32)y;
    m_FirstLook = false;
  }
  
  R32 xoffset = m_LastX - (R32)x;
  R32 yoffset =  m_LastY - (R32)y;
  m_LastX = (R32)x;
  m_LastY = (R32)y;

  xoffset *= m_X_Sensitivity;
  yoffset *= m_Y_Sensitivity;
  
  m_Yaw += xoffset;
  m_Pitch += yoffset;

  if (m_Pitch > m_ConstainedPitch) {
    m_Pitch = m_ConstainedPitch;
  }
  if (m_Pitch < -m_ConstainedPitch) {
    m_Pitch = -m_ConstainedPitch;
  }
}
#endif
} // Recluse