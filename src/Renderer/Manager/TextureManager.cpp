#include "TextureManager.h"

namespace Mc
{
    Ref<Texture2D> TextureManager::GetTexture(const std::string &filePath)
    {
        // 尝试从缓存中查找
        auto it = m_LoadedTextures.find(filePath);
        if (it != m_LoadedTextures.end())
        {
            // 找到了，直接返回缓存中的共享指针
            // LOG_TEXTURE_INFO("Texture '{0}' retrieved from cache. Ref count: {1}", filePath, it->second.use_count());
            return it->second;
        }

        // 缓存中没有，需要加载
        LOG_CORE_INFO("Loading new texture: {0}", filePath);
        Ref<Texture2D> newTexture = Texture2D::Create(filePath);

        // 成功加载，存入缓存
        m_LoadedTextures[filePath] = newTexture;
        LOG_CORE_INFO("Texture '{0}' loaded and cached. Ref count: {1}", filePath, newTexture.use_count());
        return newTexture;
    }

    void TextureManager::Shutdown()
    {
        // shared_ptr 会在引用计数归零时自动销毁 Texture 对象，从而释放 GPU 资源
        m_LoadedTextures.clear();
        LOG_CORE_INFO("All cached textures cleared.");
    }

    bool TextureManager::UnloadTexture(const std::string &filePath)
    {
        auto it = m_LoadedTextures.find(filePath);
        if (it != m_LoadedTextures.end())
        {
            // 如果还有其他地方引用这个纹理，那么它不会被立即销毁
            if (it->second.use_count() == 1) // 只有 TextureManager 自己的引用
            {
                LOG_CORE_INFO("Unloading texture: {0}. No other references.", filePath);
            }
            else
            {
                LOG_CORE_INFO("Texture '{0}' has {1} active references. Not immediately unloaded.", filePath, it->second.use_count() - 1);
            }
            m_LoadedTextures.erase(it); // 从 map 中移除，引用计数减少
            return true;
        }
        LOG_CORE_INFO("Texture '{0}' not found in cache for unloading.", filePath);
        return false;
    }
}