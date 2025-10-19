#include "Mesh.h"

#include "src/Renderer/Manager/MaterialManager.h"

namespace Mc
{
    Mesh::Mesh(std::string name, std::vector<ModelVertex> vertices, std::vector<uint32_t> indices)
    {
        m_Name = name;

        m_VertexArray = VertexArray::Create();
        m_VertexArray->Bind();

        m_VertexBuffer = VertexBuffer::Create(vertices.data(), (uint32_t)(vertices.size() * sizeof(ModelVertex)));

        m_VertexBuffer->SetLayout({
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float3, "a_Normal"},
            {ShaderDataType::Float2, "a_TexCoords"},
            {ShaderDataType::Float3, "a_Tangent"},
            {ShaderDataType::Float3, "a_Bitangent"}
            // {ShaderDataType::Int4, "a_BoneIDs"}, // 如果有骨骼动画
            // {ShaderDataType::Float4, "a_Weights"}
        });
        m_VertexArray->AddVertexBuffer(m_VertexBuffer);

        m_IndexBuffer = IndexBuffer::Create(indices.data(), (uint32_t)indices.size());
        m_VertexArray->SetIndexBuffer(m_IndexBuffer);

        m_VertexArray->UnBind(); // 解绑 VAO
    }

    void Mesh::Draw(Ref<Shader> &shader)
    {
        m_VertexArray->Bind();
        glDrawElements(GL_TRIANGLES, m_IndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);
        m_VertexArray->UnBind();
    }

    void Mesh::DrawInstanced(Ref<Shader> &shader, uint32_t instanceCount)
    {       
        m_VertexArray->Bind();
        glDrawElementsInstanced(GL_TRIANGLES, m_IndexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr, instanceCount);
        m_VertexArray->UnBind();
    }

    Ref<Mesh> Mesh::Create(std::string name, std::vector<ModelVertex> vertices, std::vector<uint32_t> indices)
    {
        return CreateRef<Mesh>(name, vertices, indices);
    }
}