#pragma once

#include "src/Renderer/Model.h"
#include "src/Core/Base.h"

namespace Mc
{
    class ModelManager
    {
    public:
        static ModelManager &Get()
        {
            static ModelManager instance;
            return instance;
        }

        ModelManager(const ModelManager &) = delete;
        ModelManager &operator=(const ModelManager &) = delete;

        // 从文件路径加载或获取一个模型
        Ref<Model> GetModel(const std::string &filepath);

        void Shutdown();

    private:
        ModelManager() = default;

        std::unordered_map<std::string, Ref<Model>> m_Models;
    };
}

