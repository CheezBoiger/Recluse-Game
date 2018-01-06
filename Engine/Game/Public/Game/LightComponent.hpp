//
#include "Engine.hpp"
#include "Component.hpp"

namespace Recluse {


class LightComponent : public Component {
  RCOMPONENT(LightComponent);
public:
  enum LightType {
    POINT_LIGHT,
    DIRECTION_LIGHT
  };

  virtual void              Initialize() { }

  u32                       GetId() const { return m_Id; }
  Renderer*                 GetRenderer() const { return m_pRenderer; }

  virtual void              SetIntensity(r32 intensity) { }
protected:
  LightType                 m_Type;
  u32                       m_Id;
 Renderer*                  m_pRenderer;
 LightBuffer*               m_pLightDataRef;
};
} // Recluse