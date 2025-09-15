#include "HdrManager.h"

namespace Mc
{
    void HdrManager::SetActive(const std::string &path)
    {
        if (path.empty())
        {
            m_ActiveHdrMap.reset();
            return;
        }

        if (m_ActiveHdrMap && m_ActiveHdrMap->GetPath() == path) return;

        auto it = m_LoadedHdrMaps.find(path);
        if (it != m_LoadedHdrMaps.end())
            m_ActiveHdrMap = it->second;
        else
        {
            Ref<HDRSkybox> newHdrMap = HDRSkybox::Create();
            newHdrMap->LoadHDRMap(path);

            m_LoadedHdrMaps[path] = newHdrMap;
            m_ActiveHdrMap = newHdrMap;
        }
    }

    void HdrManager::UnloadAll()
    {
        m_LoadedHdrMaps.clear();
        m_ActiveHdrMap.reset();
    }
}