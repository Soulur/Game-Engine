#pragma once

#include "src/Core/Base.h"
#include "src/Renderer/Shader.h"

namespace Mc
{
    class PointShadowMap
    {
    public:
        PointShadowMap(unsigned int resolution);
        ~PointShadowMap();

        void Init();

        void Bind();
        void Unbind();
        void BindTexture(uint32_t slot);

        unsigned int GetDepthCubemap() const { return m_DepthCubemap; }

        static Ref<PointShadowMap> Create(unsigned int resolution);
    private:
        unsigned int m_Resolution;
        unsigned int m_DepthMapFBO;
        unsigned int m_DepthCubemap;
    };

    class DirectionalShadowMap
    {
    public:
        DirectionalShadowMap(unsigned int resolution);
        ~DirectionalShadowMap();

        void Init();

        void Bind();
        void Unbind();
        void BindTexture(uint32_t slot);

        static Ref<DirectionalShadowMap> Create(unsigned int resolution);

    private:
        unsigned int m_Resolution;
        unsigned int m_DepthMapFBO;
        unsigned int m_DepthMap;
    };

    class SpotShadowMap
    {
    public:
        SpotShadowMap(unsigned int resolution);
        ~SpotShadowMap();

        void Init();

        void Bind();
        void Unbind();
        void BindTexture(uint32_t slot);

        static Ref<SpotShadowMap> Create(unsigned int resolution);

    private:
        unsigned int m_Resolution;
        unsigned int m_DepthMapFBO;
        unsigned int m_DepthMap;
    };
}