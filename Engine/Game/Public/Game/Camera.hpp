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


// Virtual camera, which implements the pinhole theory. Camera's allow for user's to view a scene from 
// a standpoint, while also being the bridge between gpu and application data exchange. Cameras are a 
// way to simply visualize to the game programmer, how to view and determine the art of the scene.
class Camera : public Component {
  static Camera* s_pMainCamera;
  RCOMPONENT(Camera)
public:
  // Get the main camera being used by the engine.
  static Camera*        getMain();

  static void           setMain(Camera* pCam);

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

  Matrix4     getView() { return m_viewMatrix; }
  Matrix4     getProjection() { return m_projectionMatrix; }

  // Update camera's coordinate view space. Must be called manually by the 
  // scene, because of game logic specific work.
  void        update();

  void                setFoV(r32 fov) { m_Fov = fov; }
  void                setProjection(Project proj) { m_ProjType = proj; }
  
  Project             currentProject() const { return m_ProjType; }

  r32                 getPixelWidth() const { return m_PixelWidth; }
  r32                 getPixelHeight() const { return m_PixelHeight; }
  r32                 getAspect() const { return m_Aspect; }
  r32                 getFoV() const { return m_Fov; }
  r32                 getNear() const { return m_ZNear; }
  r32                 getFar() const { return m_ZFar; }
  r32                 getOrthoScale() const { return m_OrthoScale; }

  r32                 getExposure() const { return m_Exposure; }
  r32                 getGamma() const { return m_Gamma; }
  b32                 getBloom() const { return m_Bloom; }
  b32                 getCulling() const { return m_FrustumCull; }
  b32                 getInterleavedVideo() const { return m_interleaveVideo; }
  b32                 getFilmGrain() const { return m_filmGrain; }
  r32                 getFilmGrainSpeed() const { return m_filmGrainSpeed; }
  r32                 getFilmGrainZoom() const { return m_filmGrainZoom; }

  void                resetAspect();
  void                setExposure(r32 exposure) { m_Exposure = exposure; }
  void                setGamma(r32 gamma) { m_Gamma = gamma; }
  void                enableBloom(b32 enable) { m_Bloom = enable; }
  void                enableInterleavedVideo(b32 enable) { m_interleaveVideo = enable; }
  void                enableFilmGrain(b32 enable) { m_filmGrain = enable; }
  void                enableFrustumCull(b32 enable) { m_FrustumCull = enable; }
  void                setOrthoScale(r32 scale) { m_OrthoScale = scale; }
  void                setFilmGrainSpeed(r32 speed) { m_filmGrainSpeed = speed; }
  void                setFilmGrainZoom(r32 zoom) { m_filmGrainZoom = zoom; }

  Vector3             getWorldToScreenProjection(const Vector3& position);

  ViewFrustum         getViewFrustum() const { return m_viewFrustum; }

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
  r32                 m_filmGrainZoom;
  r32                 m_filmGrainSpeed;
  b32                 m_Bloom;
  b32                 m_FrustumCull;
  b32                 m_interleaveVideo;
  b32                 m_filmGrain;

  // Gpu component update by the engine.
  void                flushToGpuBus();

  friend class Engine;
};
} // Recluse