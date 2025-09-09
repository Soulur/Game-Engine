#include "MeshManager.h"

namespace Mc
{
    uint32_t MeshManager::AddMesh(Ref<Mesh> mesh)
    {
        if (!mesh)
            return 0;

        // 给网格分配一个唯一ID
        uint32_t meshID = m_NextMeshID++;
        m_Meshes[meshID] = mesh;
        mesh->SetID(meshID);

        return meshID;
    }
    
    Ref<Mesh> MeshManager::GetMeshByID(uint32_t id)
    {
        if (m_Meshes.count(id))
        {
            return m_Meshes.at(id);
        }
        LOG_CORE_WARN("Mesh with ID {0} not found!", id);
        return nullptr;
    }

    void MeshManager::Shutdown()
    {
        m_Meshes.clear();
        m_NextMeshID = 1;
    }
}