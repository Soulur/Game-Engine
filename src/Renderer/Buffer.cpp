#include "Buffer.h"

#include <glad/glad.h>

namespace Mc
{

    // ========================================================================
    // VertexBuffer
    // ========================================================================

    VertexBuffer::VertexBuffer(uint32_t size)
    {
        glCreateBuffers(1, &m_RendererID);
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }

    VertexBuffer::VertexBuffer(void *vertices, uint32_t size)
    {
        auto buffer = size;
        auto ptr = &vertices;

        glCreateBuffers(1, &m_RendererID);
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    }

    VertexBuffer::~VertexBuffer()
    {
        glDeleteBuffers(1, &m_RendererID);
    }

    void VertexBuffer::Bind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    }

    void VertexBuffer::UnBind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void VertexBuffer::SetData(const void *data, uint32_t size)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    }

    Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
    {
        return CreateRef<VertexBuffer>(size);
    }

    Ref<VertexBuffer> VertexBuffer::Create(void *verices, uint32_t size)
    {
        return CreateRef<VertexBuffer>(verices, size);
    }

    // ========================================================================
    // ShaderStorageBuffer
    // ========================================================================

    ShaderStorageBuffer::ShaderStorageBuffer(uint32_t size)
    {
        glCreateBuffers(1, &m_RendererID);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
        glBufferStorage(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_STORAGE_BIT);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_RendererID);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    ShaderStorageBuffer::~ShaderStorageBuffer()
    {
        glDeleteBuffers(1, &m_RendererID);
    }


    void ShaderStorageBuffer::Bind() const
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_RendererID);
    }

    void ShaderStorageBuffer::UnBind() const
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void ShaderStorageBuffer::SetData(const void *data, uint32_t size)
    {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
    }

    void ShaderStorageBuffer::BindBase(uint32_t bindingPoint) const
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_RendererID);
    }

    void ShaderStorageBuffer::Destroy()
    {
        if (m_RendererID != 0) // 确保 ID 有效
        {
            glDeleteBuffers(1, &m_RendererID);
            m_RendererID = 0; // 重置ID以防止悬空指针
        }
    }

    Ref<ShaderStorageBuffer> ShaderStorageBuffer::Create(uint32_t size)
    {
        return CreateRef<ShaderStorageBuffer>(size);
    }

    // ========================================================================
    // IndexBuffer
    // ========================================================================

    IndexBuffer::IndexBuffer(uint32_t *indices, uint32_t count)
        : m_Count(count)
    {
        glCreateBuffers(1, &m_RendererID);
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);

        glBufferData(GL_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);

    }

    IndexBuffer::~IndexBuffer()
    {
        glDeleteBuffers(1, &m_RendererID);
    }

    void IndexBuffer::Bind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
    }

    void IndexBuffer::UnBind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    Ref<IndexBuffer> IndexBuffer::Create(uint32_t *indices, uint32_t count)
    {
        return CreateRef<IndexBuffer>(indices, count);
    }
}