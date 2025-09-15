#pragma once

#include "src/Renderer/HDRSkybox.h"

namespace Mc
{
    class HdrManager
    {
    public:
        static HdrManager &Get()
        {
            static HdrManager instance;
            return instance;
        }

        HdrManager(const HdrManager &) = delete;
        HdrManager &operator=(const HdrManager &) = delete;

        void SetActive(const std::string &path);
        Ref<HDRSkybox> GetActive() { return m_ActiveHdrMap; }

        void UnloadAll();

    private:
        HdrManager() = default;

    private:
        std::unordered_map<std::string, Ref<HDRSkybox>> m_LoadedHdrMaps;
        Ref<HDRSkybox> m_ActiveHdrMap;
    };
}