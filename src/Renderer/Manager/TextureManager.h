#pragma once

#include "src/Core/Base.h"
#include "src/Renderer/Texture.h"

#include <string>
#include <vector>

#include <glad/glad.h>

namespace Mc
{
    class TextureManager
    {
    public:
        static TextureManager &Get()
        {
            static TextureManager instance;
            return instance;
        }

        Ref<Texture2D> GetTexture(const std::string &filePath);

        // 显式清除所有缓存纹理（例如在游戏结束或场景切换时）
        void Shutdown();

        // 根据路径卸载单个纹理（如果引用计数允许）
        bool UnloadTexture(const std::string &filePath);

    private:
        // 私有构造函数和析构函数，防止外部直接创建或删除实例
        TextureManager() = default;
        ~TextureManager() = default;

        // 禁止拷贝构造和赋值操作
        TextureManager(const TextureManager &) = delete;
        TextureManager &operator=(const TextureManager &) = delete;

        // 存储已加载纹理的缓存
        std::unordered_map<std::string, Ref<Texture2D>> m_LoadedTextures;
    };
}