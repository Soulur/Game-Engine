#include "src/Core/Base.h"

namespace Mc
{
    class SceneLight
    {
    public:
        // 定义光源类型枚举
        enum class LightType
        {
            Point = 0,       // 点光源 (Point Light)
            Directional = 1, // 定向光 (Directional Light)
            Spot = 2         // 聚光灯 (Spot Light)
        };
    public:

        SceneLight() = default;
        virtual ~SceneLight() = default;

        LightType GetType() const { return m_Type; }
        void SetType(LightType type);

        glm::vec3 GetColor() const { return m_Color; }
        void SetColor(glm::vec3 &color) { m_Color = color; }

        // 强度 (所有光源类型共享)
        float GetIntensity() const { return m_Intensity; }
        void SetIntensity(float intensity) { m_Intensity = intensity; }

        // 投射阴影 (所有光源类型共享，但实际实现可能针对特定类型)
        bool CastsShadows() const { return m_CastsShadows; }
        void SetCastsShadows(bool castsShadows) { m_CastsShadows = castsShadows; }

        // --- 点光源/聚光灯特有属性 ---

        // 半径/影响范围 (点光源和聚光灯)
        //======================   Point  ==============================
        float GetRadius() const { return m_Radius; }
        void SetRadius(float radius); // 会自动将半径限制在合理范围

        //======================   Spot  ==============================

        // 内锥角 (聚光灯，弧度制)
        float GetInnerConeAngle() const { return m_InnerConeAngle; }
        void SetInnerConeAngle(float angle); // 会自动将角度限制在合理范围

        // 外锥角 (聚光灯，弧度制)
        float GetOuterConeAngle() const { return m_OuterConeAngle; }
        void SetOuterConeAngle(float angle); // 会自动将角度限制在合理范围

        // 辅助方法：将角度从弧度转换为度数 (便于调试或UI显示)
        float GetInnerConeAngleDegrees() const { return glm::degrees(m_InnerConeAngle); }
        void SetInnerConeAngleDegrees(float angle) { SetInnerConeAngle(glm::radians(angle)); }

        float GetOuterConeAngleDegrees() const { return glm::degrees(m_OuterConeAngle); }
        void SetOuterConeAngleDegrees(float angle) { SetOuterConeAngle(glm::radians(angle)); }

    private:
        LightType m_Type = LightType::Point;
        glm::vec3 m_Color = glm::vec3(1.0f); // 光源颜色
        float m_Intensity = 1.0f;   // 光源强度
        bool m_CastsShadows = false; // 是否投射阴影

        // 点光源 / 聚光灯 属性
        float m_Radius = 1.0f; // 光源影响范围，点光源和聚光灯使用

        // 聚光灯 属性
        float m_InnerConeAngle = glm::radians(10.0f); // 聚光灯内锥角 (弧度)
        float m_OuterConeAngle = glm::radians(20.0f); // 聚光灯外锥角 (弧度)

        // 注意：位置 (Position) 和方向 (Direction) 通常不直接存储在ght 类中，
        // 而是通过 LightComponent 所附加的 Entity 的 TransformComp Lionent 来获取。
        // 因为光照作为场景中的一个实体，其空间位置和方向应由变换组件统一管理。
    };
}