#include "Light.h"

namespace Mc
{
    void SceneLight::SetType(LightType type)
    {
        m_Type = type;
    }

    void SceneLight::SetRadius(float radius)
    {
        // 确保半径非负
        m_Radius = glm::max(0.0f, radius);
    }

    void SceneLight::SetInnerConeAngle(float angle)
    {
        // 确保内锥角在 0 到 PI 之间，且小于外锥角
        m_InnerConeAngle = glm::clamp(angle, 0.0f, glm::pi<float>());
        if (m_InnerConeAngle > m_OuterConeAngle)
        {
            m_OuterConeAngle = m_InnerConeAngle; // 确保外锥角不小于内锥角
        }
    }

    void SceneLight::SetOuterConeAngle(float angle)
    {
        // 确保外锥角在 0 到 PI 之间，且不小于内锥角
        m_OuterConeAngle = glm::clamp(angle, 0.0f, glm::pi<float>());
        if (m_OuterConeAngle < m_InnerConeAngle)
        {
            m_InnerConeAngle = m_OuterConeAngle; // 确保内锥角不大于外锥角
        }
    }
}
