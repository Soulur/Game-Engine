#pragma once

#include "src/Renderer/Mesh.h"

namespace Mc
{
    class MeshManager
    {
    public:
        static MeshManager &Get()
        {
            static MeshManager instance;
            return instance;
        }

        MeshManager(const MeshManager &) = delete;
        MeshManager &operator=(const MeshManager &) = delete;

        // 为一个网格分配ID并将其添加到管理器
        uint32_t AddMesh(Ref<Mesh> mesh);

        // 通过ID获取网格
        Ref<Mesh> GetMeshByID(uint32_t id);

        void Shutdown();

    private:
        MeshManager() = default;

        std::unordered_map<uint32_t, Ref<Mesh>> m_Meshes;
        uint32_t m_NextMeshID = 1;
    };
}