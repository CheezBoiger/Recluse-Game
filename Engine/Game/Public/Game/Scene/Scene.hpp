// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Core/Serialize.hpp"
#include "Core/Utility/Vector.hpp"

#include "Renderer/LightDescriptor.hpp"
#include "Renderer/HDR.hpp"

#include "Game/Rendering/Sky.hpp"
#include "Game/GameObject.hpp"

#include <set>

namespace Recluse {


class GameObject;
class Scene;
class ReflectionProbe;

struct LightProbe;

class SceneNode {
public:
  SceneNode()
    : m_pScene(nullptr) { }

  // Adds a child game object to the scene graph root.
  void                      addChild(GameObject* child);

  size_t                    getChildrenCount() const { return m_GameObjects.size(); }

  GameObject*               getChild(size_t idx) { return m_GameObjects[idx]; }

  void                      setSceneOwner(Scene* scene) { m_pScene = scene; }
  Scene*                    getSceneOwner() { return m_pScene; }

private:
  std::vector<GameObject*> m_GameObjects;
  Scene*                    m_pScene;
};

// Scene, used for storing, keeping track off, and maintaining, the current state of the game world. 
// The world game logic is set up by the user, however, scene graph management is handled by the renderer.
// Be sure to add render objects to the scene graph root, in order to see anything on the screen!
class Scene : public ISerializable {
  static std::string        default_name;
public:

  // Load a scene from file. Will generate full scene graph for this object. Callers will still need to figure out which 
  // nodes to add scripts for. May be formatted in GLTF or FBX.
  static B32                loadFromFile(Scene* pOut, const std::string& filename);

  Scene(std::string name = default_name);

  ~Scene() { }

  // Set the name of this scene.
  void                      setName(std::string name) { m_SceneName = name; }

  // Get the root of the scene graph, this is the root node used by the engine to determine what 
  // to render, and how to render it.
  SceneNode*                getRoot() { return &m_Root; }

  // Get the name of this scene!
  std::string               getName() const { return m_SceneName; }

  // Default set up for scene.
  virtual void              setUp() { }

  // Start up is optional, since most work may be done in setUp().
  virtual void              startUp() { }

  // Update the scene. Game logic override goes here.
  virtual void              update(R32 tick) { }

  virtual void              cleanUp() { }

  void                      serialize(IArchive& archive) override { }
  void                      deserialize(IArchive& archive) override  { }

  Sky*                      getSky() { return &m_sky; }

  void                      setGlobalEnvMap(TextureCube* pCube) { m_globalEnvMap = pCube; }

private:
  Sky                       m_sky;

  std::string               m_SceneName;

  // The root to the scene graph. This is used by the Engine to set up the 
  // transformation update.
  SceneNode                 m_Root;

  // Global illumination Lighting information cache.
  std::vector<LightProbe*>          m_localLightProbes;
  std::vector<ReflectionProbe*>     m_localReflectionProbes;
  std::vector<TextureCube*>         m_localEnvMaps;
  TextureCube*                      m_globalEnvMap;
  LightProbe*                       m_globalLightProbe;
  ReflectionProbe*                  m_globalReflectionProbe;

  // Physics based information may go here as well.
};
} // Recluse 