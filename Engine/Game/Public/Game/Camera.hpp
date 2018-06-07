// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Matrix4.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Core/Math/Ray.hpp"
#include "Component.hpp"

#include "Core/Utility/Vector.hpp"
#include "Core/Math/ViewFrustum.hpp"


namespace Recluse {


class GameObject;


// Virtual camera, which implements the pinhole theory.
class Camera : public Component {
  static Camera* s_pMainCamera;
  RCOMPONENT(Camera)
public:
  // Get the main camera being used by the engine.
  static Camera*        GetMain();

  static void           SetMain(Camera* pCam);

  // Movement for the camera.
  enum Movement {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    FORWARD,
    BACK
  };

  enum Project {
    ORTHO,
    PERSPECTIVE
  };

  ~Camera() { }

  Camera(Project type, r32 fov, r32 zNear, r32 zFar);

  Matrix4     View() { return m_viewMatrix; }
  Matrix4     Projection() { return m_projectionMatrix; }

  // Update camera's coordinate view space.
  void        Update();

  void                SetFoV(r32 fov) { m_Fov = fov; }
  void                SetProjection(Project proj) { m_ProjType = proj; }
  
  Project             CurrentProject() const { return m_ProjType; }

  r32                 PixelWidth() const { return m_PixelWidth; }
  r32                 PixelHeight() const { return m_PixelHeight; }
  r32                 Aspect() const { return m_Aspect; }
  r32                 FoV() const { return m_Fov; }
  r32                 Near() const { return m_ZNear; }
  r32                 Far() const { return m_ZFar; }
  r32                 OrthoScale() const { return m_OrthoScale; }

  r32                 Exposure() const { return m_Exposure; }
  r32                 Gamma() const { return m_Gamma; }
  b32                 Bloom() const { return m_Bloom; }
  b32                 Culling() const { return m_FrustumCull; }

  void                ResetAspect();
  void                SetExposure(r32 exposure) { m_Exposure = exposure; }
  void                SetGamma(r32 gamma) { m_Gamma = gamma; }
  void                EnableBloom(b32 enable) { m_Bloom = enable; }
  void                EnableFrustumCull(b32 enable) { m_FrustumCull = enable; }
  void                SetOrthoScale(r32 scale) { m_OrthoScale = scale; }
protected:
  ViewFrustum         m_viewFrustum;
  Matrix4             m_projectionMatrix;
  Matrix4             m_viewMatrix;
  Project             m_ProjType;
  r32                 m_OrthoScale;
  r32                 m_Fov;
  r32                 m_Aspect;
  r32                 m_PixelWidth;
  r32                 m_PixelHeight;
  r32                 m_ZNear;
  r32                 m_ZFar;

  r32                 m_Gamma;
  r32                 m_Exposure;
  b32                 m_Bloom;
  b32                 m_FrustumCull;
};
} // Recluse