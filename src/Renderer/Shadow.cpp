#include "Shadow.h"

#include "src/Core/Log.h"

#include <glad/glad.h>

namespace Mc
{
    PointShadowMap::PointShadowMap(unsigned int resolution)
        : m_Resolution(resolution)
    {
        Init();
    }

    PointShadowMap::~PointShadowMap()
    {
        glDeleteFramebuffers(1, &m_DepthMapFBO);
        glDeleteTextures(1, &m_DepthCubemap);
    }

    void PointShadowMap::Init()
    {
        glGenFramebuffers(1, &m_DepthMapFBO);
        glGenTextures(1, &m_DepthCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_DepthCubemap);

        for (unsigned int i = 0; i < 6; ++i)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32F, m_Resolution, m_Resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        // attach depth texture as FBO's depth buffer
        glBindFramebuffer(GL_FRAMEBUFFER, m_DepthMapFBO);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_DepthCubemap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            LOG_CORE_WARN("Framebuffer not complete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void PointShadowMap::Bind()
    {
        glViewport(0, 0, m_Resolution, m_Resolution);
        glBindFramebuffer(GL_FRAMEBUFFER, m_DepthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void PointShadowMap::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void PointShadowMap::BindTexture(uint32_t slot)
    {
        glActiveTexture(GL_TEXTURE0 + 35 + slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_DepthCubemap);
    }

    Ref<PointShadowMap> PointShadowMap::Create(unsigned int resolution)
    {
        return CreateRef<PointShadowMap>(resolution);
    }
}
