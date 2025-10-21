#pragma once

#include "src/Core/Base.h"

#include <string>
#include <vector>

#include <glad/glad.h>

namespace Mc
{
    class Texture2D
    {
    public:
        Texture2D(uint32_t width, uint32_t height);
        Texture2D(const std::string &path);
        ~Texture2D();

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        std::string GetPath() const { return m_Path; }
        uint32_t GetRendererID() const { return m_RendererID; }

        void SetTexture();
        void SetData(void *data, uint32_t size);

        void Bind(uint32_t slot = 0) const;

        bool IsLoaded() const { return m_IsLoaded; }

        bool operator==(const Texture2D &other) const
        {
            return m_RendererID == ((Texture2D &)other).m_RendererID;
        }

    private:
        std::string m_Path;
        bool m_IsLoaded = false;
        uint32_t m_RendererID;
        uint32_t m_Width, m_Height;
        GLenum m_InternalFormat, m_DataFormat;
    public:
        static Ref<Texture2D> Create(uint32_t width, uint32_t height);
        static Ref<Texture2D> Create(const std::string &path);
    };
}