#pragma once

#include "src/Renderer/VertexArray.h"
#include "src/Renderer/Buffer.h"
#include "src/Renderer/Material.h"

namespace Mc
{
    class Sphere
    {
    public:
        struct SphereVertex
        {
            glm::vec3 Position;
            glm::vec3 Normal;
            glm::vec2 TexCoord;
            glm::vec3 Tangents;
            glm::vec3 Bitangents;
        };

        Sphere();
        Sphere(std::vector<SphereVertex> &vertices, std::vector<uint32_t> &indices);
        ~Sphere(); // 析构函数，Ref智能指针会处理OpenGL资源的释放

        void DrawInstanced(Ref<Shader> &shader, uint32_t instanceCount)
        {
            m_VertexArray->Bind();
            // glDrawElementsInstanced(GL_TRIANGLES, m_IndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr, instanceCount);
            glDrawElementsInstanced(GL_TRIANGLE_STRIP, m_IndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr, instanceCount);

            m_VertexArray->UnBind();
        }

        // 获取该球体网格的渲染数据，供Renderer3D使用
        Ref<VertexArray> GetVertexArray() { return m_VertexArray; }
        uint32_t GetIndexCount() const { return m_IndexCount; } // 获取该球体网格的索引数量

        // 材质（如果每个Sphere实例可以有自己的材质）
        void SetMaterial(Ref<Material> &material) { m_Material = material; }
        Ref<Material> GetMaterial() { return m_Material; }

        static Ref<Sphere> Create();
        static Ref<Sphere> Create(std::vector<SphereVertex> &vertices, std::vector<uint32_t> &indices);

    private:
        // Sphere类现在拥有并管理其自身的OpenGL渲染数据
        Ref<VertexArray> m_VertexArray;
        Ref<VertexBuffer> m_VertexBuffer;
        Ref<IndexBuffer> m_IndexBuffer;
        uint32_t m_IndexCount; // 存储索引数量

        Ref<Material> m_Material;
    };
}