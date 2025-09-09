#pragma once

#include "src/Core/Base.h"
#include "src/Renderer/Shader.h"
#include "src/Renderer/Texture.h"

#include <string>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>

namespace Mc
{
    // 定义纹理类型，与 PBR 材质对应
    enum class TextureType
    {
        Albedo = 0,       // 等同于传统的 Diffuse
        Normal,           // 法线贴图
        Metallic,         // 金属度贴图
        Roughness,        // 粗糙度贴图
        AmbientOcclusion, // AO 贴图
        Emissive,         // 自发光贴图
        Height,           // 高度贴图 (视情况决定是否使用)
        Count             // 纹理类型数量，方便循环
    };

    class Material
    {
    public:
        Material();
        ~Material() = default;

        // Factory method using Ref
        static Ref<Material> Create();

        void SetID(uint32_t id) { m_ID = id; }
        uint32_t GetID() const { return m_ID; }
        
        void SetName(const char *name) { m_Name = name; }
        std::string GetName() { return m_Name; }

        // PBR 属性
        void SetAlbedo(const glm::vec4 &color) { m_Albedo = color; }
        void SetRoughness(float roughness) { m_Roughness = glm::clamp(roughness, 0.0f, 1.0f); }
        void SetMetallic(float metallic) { m_Metallic = glm::clamp(metallic, 0.0f, 1.0f); }
        void SetAO(float ao) { m_AO = glm::clamp(ao, 0.0f, 1.0f); }
        void SetEmissive(const glm::vec3 &emissiveColor) { m_Emissive = emissiveColor; }

        const glm::vec4 &GetAlbedo() const { return m_Albedo; }
        float GetRoughness() const { return m_Roughness; }
        float GetMetallic() const { return m_Metallic; }
        float GetAO() const { return m_AO; }
        const glm::vec3 &GetEmissive() const { return m_Emissive; }

        // 设置纹理，会通过 TextureManager 获取
        void SetTexture(TextureType type, const std::string &texturePath);
        void RemoveTexture(TextureType type); // 移除特定类型纹理，退回到默认纹理

        const Ref<Texture2D> &GetTexture(TextureType type) const;

        std::unordered_map<TextureType, Ref<Texture2D>> Get() { return m_Textures; }

    private:
        uint32_t m_ID = 0;
        std::string m_Name;
        // PBR 材质属性
        glm::vec4 m_Albedo = glm::vec4(1.0f);   // 基色 / 漫反射颜色
        float m_Roughness = 0.5f;               // 粗糙度
        float m_Metallic = 0.0f;                // 金属度
        float m_AO = 1.0f;                      // 环境光遮蔽
        glm::vec3 m_Emissive = glm::vec3(0.0f); // 自发光颜色

        // 存储纹理的映射，使用 Ref<Texture2D> 共享纹理资源
        std::unordered_map<TextureType, Ref<Texture2D>> m_Textures;
        Ref<Texture2D> m_DefaultWhiteTexture;

        friend class Model; // 允许 Model 访问 Material 的私有成员进行初始化
    };
}