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

  Camera(Project type, R32 fov, R32 zNear, R32 zFar);

  Matrix4     getView() { return m_viewMatrix; }
  Matrix4     getProjection() { return m_projectionMatrix; }

  // Update camera's coordinate view space. Must be called manually by the 
  // scene, because of game logic specific work.
  void        update();

  void                setFoV(R32 fov) { m_Fov = fov; }
  void                setProjection(Project proj) { m_ProjType = proj; }
  
  Project             currentProject() const { return m_ProjType; }

  R32                 getPixelWidth() const { return m_PixelWidth; }
  R32                 getPixelHeight() const { return m_PixelHeight; }
  R32                 getAspect() const { return m_Aspect; }
  R32                 getFoV() const { return m_Fov; }
  R32                 getNear() const { return m_ZNear; }
  R32                 getFar() const { return m_ZFar; }
  R32                 getOrthoScale() const { return m_OrthoScale; }

  R32                 getExposure() const { return m_Exposure; }
  R32                 getGamma() const { return m_Gamma; }
  B32                 getBloom() const { return m_Bloom; }
  B32                 getCulling() const { return m_FrustumCull; }
  B32                 getInterleavedVideo() const { return m_interleaveVideo; }
  B32                 getFilmGrain() const { return m_filmGrain; }
  R32                 getFilmGrainSpeed() const { return m_filmGrainSpeed; }
  R32                 getFilmGrainZoom() const { return m_filmGrainZoom; }

  void                resetAspect();
  void                setExposure(R32 exposure) { m_Exposure = exposure; }
  void                setGamma(R32 gamma) { m_Gamma = gamma; }
  void                enableBloom(B32 enable) { m_Bloom = enable; }
  void                enableInterleavedVideo(B32 enable) { m_interleaveVideo = enable; }
  void                enableFilmGrain(B32 enable) { m_filmGrain = enable; }
  void                enableFrustumCull(B32 enable) { m_FrustumCull = enable; }
  void                setOrthoScale(R32 scale) { m_OrthoScale = scale; }
  void                setFilmGrainSpeed(R32 speed) { m_filmGrainSpeed = speed; }
  void                setFilmGrainZoom(R32 zoom) { m_filmGrainZoom = zoom; }

  Vector3             getWorldToScreenProjection(const Vector3& position);

  ViewFrustum         getViewFrustum() const { return m_viewFrustum; }

protected:
  ViewFrustum         m_viewFrustum;
  Matrix4             m_projectionMatrix;
  Matrix4             m_viewMatrix;
  Project             m_ProjType;
  R32                 m_OrthoScale;
  R32                 m_Fov;
  R32                 m_Aspect;
  R32                 m_PixelWidth;
  R32                 m_PixelHeight;
  R32                 m_ZNear;
  R32                 m_ZFar;

  R32                 m_Gamma;
  R32                 m_Exposure;
  R32                 m_filmGrainZoom;
  R32                 m_filmGrainSpeed;
  B32                 m_Bloom;
  B32                 m_FrustumCull;
  B32                 m_interleaveVideo;
  B32                 m_filmGrain;

  // Gpu component update by the engine.
  void                flushToGpuBus();

  friend class Engine;
};
} // Recluse