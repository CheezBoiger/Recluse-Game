// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Camera.hpp"
#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Math/Common.hpp"

#include "Engine.hpp"


namespace Recluse {


Camera* Camera::s_pMainCamera = nullptr;


Camera* Camera::GetMain()
{
  return s_pMainCamera;
}


void Camera::SetMain(Camera* pCam)
{
  if (!pCam) return;
  s_pMainCamera = pCam;

  // TODO(): Need to remove previous frustum from engine list.
  gEngine().AddFrustum(&pCam->m_viewFrustum);
}


Camera::Camera(Project type, r32 fov, r32 zNear, r32 zFar)
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
  , m_Exposure(4.5f)
  , m_FrustumCull(false)
  , m_interleaveVideo(false)
{
}


void Camera::ResetAspect()
{
  m_Aspect = m_PixelWidth / m_PixelHeight;
}


void Camera::Update()
{
  if (!GetOwner()) return;

  Transform* transform = GetOwner()->GetTransform();
  Vector3 pos = transform->Position;
  Vector3 right = transform->Right();
  Vector3 up = transform->Up();
  Vector3 front = transform->Front();

  // Update camera and screen info.
  GlobalBuffer* gGlobalBuffer = gRenderer().GlobalData();
  ConfigHDR* hdr = gRenderer().GetHDR()->GetRealtimeConfiguration();
  Window* pWindow = gEngine().GetWindow();

  m_viewMatrix = Matrix4(
     right.x,          up.x,         front.x,        0.0f,
     right.y,          up.y,         front.y,        0.0f,
     right.z,          up.z,         front.z,        0.0f,
    -right.Dot(pos),  -up.Dot(pos), -front.Dot(pos), 1.0f
  );

  m_projectionMatrix = Matrix4();
  if (m_ProjType == ORTHO) {
    m_projectionMatrix = Matrix4::Ortho((m_PixelWidth * 0.01f) * m_OrthoScale,
      (m_PixelHeight * 0.01f) * m_OrthoScale,
      m_ZNear,
      m_ZFar);
  } else {
    m_projectionMatrix = Matrix4::Perspective(m_Fov, m_Aspect, m_ZNear, m_ZFar);
  }

  r32 winPixWidth = r32(pWindow->Width());
  r32 winPixHeight = r32(pWindow->Height());
  if (m_PixelHeight != winPixHeight || m_PixelWidth != winPixWidth) {
    m_PixelWidth = winPixWidth;
    m_PixelHeight = winPixHeight;
    m_Aspect = (m_PixelWidth / m_PixelHeight);
  }

  gGlobalBuffer->_CameraPos = Vector4(pos, 1.0f);
  gGlobalBuffer->_Proj = m_projectionMatrix;
  gGlobalBuffer->_View = m_viewMatrix;
  gGlobalBuffer->_ViewProj = gGlobalBuffer->_View * gGlobalBuffer->_Proj;
  gGlobalBuffer->_InvViewProj = gGlobalBuffer->_ViewProj.Inverse();
  gGlobalBuffer->_InvView = gGlobalBuffer->_View.Inverse();
  gGlobalBuffer->_InvProj = gGlobalBuffer->_Proj.Inverse();
  gGlobalBuffer->_ScreenSize[0] = pWindow->Width();
  gGlobalBuffer->_ScreenSize[1] = pWindow->Height();
  gGlobalBuffer->_BloomEnabled = Bloom();
  gGlobalBuffer->_Exposure = Exposure();
  gGlobalBuffer->_Gamma = Gamma();
  gGlobalBuffer->_MousePos = Vector2((r32)Mouse::X(), (r32)Mouse::Y());
  gGlobalBuffer->_fEngineTime = static_cast<r32>(Time::CurrentTime());
  gGlobalBuffer->_fDeltaTime = static_cast<r32>(Time::DeltaTime);

  // TODO(): Move cam frustum to camera.
  m_viewFrustum.Update(gGlobalBuffer->_ViewProj);
  gGlobalBuffer->_LPlane = m_viewFrustum[ViewFrustum::PLEFT];
  gGlobalBuffer->_RPlane = m_viewFrustum[ViewFrustum::PRIGHT];
  gGlobalBuffer->_BPlane = m_viewFrustum[ViewFrustum::PBOTTOM];
  gGlobalBuffer->_TPlane = m_viewFrustum[ViewFrustum::PTOP];
  gGlobalBuffer->_NPlane = m_viewFrustum[ViewFrustum::PNEAR];
  gGlobalBuffer->_FPlane = m_viewFrustum[ViewFrustum::PFAR];

  // HDR settings.
  hdr->_interleavedVideo.x = r32(InterleavedVideo());
}

#if 0
FlyViewCamera::FlyViewCamera(r32 fov, r32 pixelWidth, r32 pixelHeight, 
  r32 zNear, r32 zFar, Vector3 pos, Vector3 dir)
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


Matrix4 FlyViewCamera::View()
{
  return Matrix4();
}


void FlyViewCamera::Move(Movement movement, r64 dt)
{
}


void FlyViewCamera::Update()
{
}


void FlyViewCamera::Look(r64 x, r64 y)
{
  if (m_FirstLook) {
    m_LastX = (r32)x;
    m_LastY = (r32)y;
    m_FirstLook = false;
  }
  
  r32 xoffset = m_LastX - (r32)x;
  r32 yoffset =  m_LastY - (r32)y;
  m_LastX = (r32)x;
  m_LastY = (r32)y;

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