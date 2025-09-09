#pragma once

#include "Shader.h"
#include "src/Renderer/VertexArray.h"
#include "src/Renderer/Buffer.h"
#include "src/Renderer/Material.h"

// #define MAX_BONE_INFLUENCE 4

// TODO : 目前只支持 OBJ
namespace Mc
{
    struct ModelVertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;
        // 骨骼
        // int BoneIDs[MAX_BONE_INFLUENCE];
        // float Weights[MAX_BONE_INFLUENCE];
    };

    class Mesh
    {
    public:
        // constructor
        Mesh(std::string name, std::vector<ModelVertex> vertices, std::vector<uint32_t> indices);

        // render the mesh
        void Draw(Ref<Shader> &shader);
        void DrawInstanced(Ref<Shader> &shader, uint32_t instanceCount);

        void SetMaterial(Ref<Material> &material);
        Ref<Material> GetMaterial() { return m_Material; }

        Ref<VertexArray> GetVertexArray() { return m_VertexArray; }
        Ref<IndexBuffer> GetIndexBuffer() { return m_IndexBuffer; }
        std::string GetName() { return m_Name; }

        void SetID(uint32_t id) { m_ID = id; }
        uint32_t GetID() const { return m_ID; }

        static Ref<Mesh> Create(std::string name, std::vector<ModelVertex> vertices, std::vector<uint32_t> indicescount);

    private:
        uint32_t m_ID = 0;
        std::string m_Name;
        // mesh Data
        std::vector<ModelVertex> m_Vertices;
        uint32_t* m_Indices;

        // render data
        Ref<VertexArray> m_VertexArray;
        Ref<VertexBuffer> m_VertexBuffer;
        Ref<IndexBuffer> m_IndexBuffer;

        Ref<Material> m_Material;
    };
}