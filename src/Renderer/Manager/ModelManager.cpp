#include "ModelManager.h"
#include "src/Renderer/Manager/MeshManager.h"

namespace Mc
{
    Ref<Model> ModelManager::GetModel(const std::string &filepath)
    {
        // 如果模型已加载，直接返回缓存的实例
        if (m_Models.count(filepath))
        {
            return m_Models.at(filepath);
        }

        // 否则，从硬盘加载
        Ref<Model> newModel = Model::Create(filepath);

        if (newModel)
        {
            // 遍历新模型的每个网格，并将其添加到 MeshManager 中
            for (const auto &mesh : newModel->GetMeshs())
            {
                MeshManager::Get().AddMesh(mesh);
            }

            // 将新模型添加到缓存中
            m_Models[filepath] = newModel;
            return newModel;
        }

        return nullptr;
    }

    void ModelManager::Shutdown()
    {
        m_Models.clear();
    }
}