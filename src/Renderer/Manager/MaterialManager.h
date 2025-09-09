#pragma once

#include "src/Renderer/Material.h"
#include <unordered_map>

namespace Mc
{
    class MaterialManager
    {
    public:
        // 使用单例模式确保全局唯一实例
        static MaterialManager &Get()
        {
            static MaterialManager instance;
            return instance;
        }

        MaterialManager(const MaterialManager &) = delete;
        void operator=(const MaterialManager &) = delete;
        
        Ref<Material> AddMaterial();
        // Ref<Material> GetMaterial(uint32_t id);
        Ref<Material> Init() { return m_Materials.find(0)->second; };

        // 清除所有材质
        void Shutdown();

    private:
        MaterialManager() = default;

        std::unordered_map<uint32_t, Ref<Material>> m_Materials;
        uint32_t m_NextMaterialID = 0;
    };
}