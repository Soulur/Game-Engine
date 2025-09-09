// Material.cpp
#include "MaterialManager.h"

#include <iostream>

namespace Mc
{
    

    Ref<Material> MaterialManager::AddMaterial()
    {
        // 如果不存在，创建新材质并添加到集合中
        Ref<Material> newMaterial = Material::Create();
        uint32_t id = m_NextMaterialID++;
        m_Materials[id] = newMaterial;
        newMaterial->SetID(id);

        return newMaterial;
    }

    // Ref<Material> MaterialManager::GetMaterial(uint32_t id)
    // {
    //     // 查找是否已存在
    //     auto it = m_Materials.find(id);
    //     if (it != m_Materials.end())
    //     {
    //         // 如果存在，直接返回
    //         return it->second;
    //     }
    // }

    void MaterialManager::Shutdown()
    {
        m_Materials.clear();
        LOG_CORE_INFO("All materials cleared.");
    }

}