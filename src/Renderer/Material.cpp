// Material.cpp
#include "Material.h"
#include "src/Renderer/Manager/TextureManager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>

namespace Mc
{
    // 全局默认白色纹理实例
    static Ref<Texture2D> s_DefaultWhiteTexture;

    // 默认纹理的初始化函数
    void InitDefaultMaterialResources()
    {
        if (!s_DefaultWhiteTexture)
        {
            s_DefaultWhiteTexture = Texture2D::Create(1, 1);
            uint32_t whiteTextureData = 0xffffffff; // RGBA white (0xFFRRGGBB)
            s_DefaultWhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));
            // LOG_CORE_INFO("Default White Texture Initialized."); // 你的日志
        }
    }

    Material::Material()
    {
        // 确保默认白色纹理已被初始化
        // 最好在渲染器初始化时调用一次 InitDefaultMaterialResources()，
        // 而不是在每个 Material 构造函数中都检查
        InitDefaultMaterialResources();                // 暂时放在这里，确保可用
        m_DefaultWhiteTexture = s_DefaultWhiteTexture; // 引用全局默认纹理

        // 默认情况下，将所有纹理槽位设置为默认白色纹理
        for (int i = 0; i < (int)TextureType::Count; ++i)
        {
            m_Textures[(TextureType)i] = m_DefaultWhiteTexture;
        }
    }

    Ref<Material> Material::Create()
    {
        return CreateRef<Material>();
    }

    void Material::SetTexture(TextureType type, const std::string &texturePath)
    {
        // 通过 TextureManager 获取纹理，实现共享
        // 假设 TextureManager::GetInstance() 是可访问的
        Ref<Texture2D> loadedTexture = TextureManager::Get().GetTexture(texturePath);

        if (loadedTexture)
        {
            m_Textures[type] = loadedTexture;
        }
        else
        {
            // 如果加载失败，回退到默认白色纹理
            std::cerr << "Warning: Failed to load texture '" << texturePath << "'. Using default white texture for type " << static_cast<int>(type) << std::endl;
            m_Textures[type] = m_DefaultWhiteTexture;
        }
    }

    void Material::RemoveTexture(TextureType type)
    {
        m_Textures[type] = m_DefaultWhiteTexture;
    }

    const Ref<Texture2D> &Material::GetTexture(TextureType type) const
    {
        auto it = m_Textures.find(type);
        if (it != m_Textures.end())
        {
            return it->second;
        }
        return m_DefaultWhiteTexture; // 如果找不到，返回默认白色纹理
    }

    // Helper: 将 Assimp 纹理类型映射到你的 TextureType
    // 这是一个内部函数，用于 Model::LoadAllSceneMaterials
    // static TextureType GetTextureTypeFromAssimp(aiTextureType aiType)
    // {
    //     switch (aiType)
    //     {
    //     case aiTextureType_DIFFUSE:
    //         return TextureType::Albedo;
    //     case aiTextureType_NORMALS: // Assimp 常用 NORAML 为法线贴图
    //     case aiTextureType_HEIGHT:  // 有时 HEIGHT 也用作法线或高度
    //         return TextureType::Normal;
    //     case aiTextureType_SPECULAR:
    //         return TextureType::Metallic; // 粗略映射：高光可能映射到金属度或粗糙度
    //     // Assimp 没有直接的 Roughness/AO/Metallic 类型，需要通过纹理文件名约定或读取自定义属性来判断
    //     // 这里只是一个简化示例
    //     case aiTextureType_EMISSIVE:
    //         return TextureType::Emissive;
    //     case aiTextureType_AMBIENT_OCCLUSION:
    //         return TextureType::AmbientOcclusion; // Assimp 5.x 有此类型
    //     default:
    //         return TextureType::Count; // 未知类型
    //     }
    // }
}