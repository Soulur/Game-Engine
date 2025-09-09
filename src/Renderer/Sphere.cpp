#include "Sphere.h"

namespace Mc
{
    Sphere::Sphere()
    {
        m_Material = Material::Create();
    }

    // Sphere 构造函数实现
    Sphere::Sphere(std::vector<SphereVertex> &vertices, std::vector<uint32_t> &indices)
        : m_IndexCount((uint32_t)indices.size()) // 初始化索引数量
    {
        m_VertexArray = VertexArray::Create();
        m_VertexArray->Bind();

        m_VertexBuffer = VertexBuffer::Create(vertices.data(), (uint32_t)vertices.size() * sizeof(SphereVertex));
        m_VertexBuffer->SetLayout({
        	{ShaderDataType::Float3, "a_Position"},
        	{ShaderDataType::Float3, "a_Normal"},
        	{ShaderDataType::Float2, "a_TexCoord"},
        	{ShaderDataType::Float3, "a_Tangent"},
        	{ShaderDataType::Float3, "a_Bitangent"}
        });
        m_VertexArray->AddVertexBuffer(m_VertexBuffer);

        m_IndexBuffer = IndexBuffer::Create(indices.data(), indices.size());
        m_VertexArray->SetIndexBuffer(m_IndexBuffer);

        m_Material = Material::Create();
    }

    Sphere::~Sphere()
    {
    }

    Ref<Sphere> Sphere::Create()
    {
        return CreateRef<Sphere>();
    }

    Ref<Sphere> Sphere::Create(std::vector<SphereVertex> &vertices, std::vector<uint32_t> &indices)
    {
        return CreateRef<Sphere>(vertices, indices);
    }
}
