#pragma once

#include "src/Renderer/Shader.h"
#include "src/Renderer/VertexArray.h"
#include "src/Renderer/Buffer.h"

#include <string>
#include <glm/glm.hpp>
// #include <memory>

namespace Mc
{
    class HDRSkybox
    {
    public:
        HDRSkybox();
        ~HDRSkybox();

        // 加载 HDR 图像并进行所有预计算
        void LoadHDRMap(const std::string &filepath);

        // 渲染天空盒
        void Render(const glm::mat4 &projection, const glm::mat4 &view);
        void Bind(Ref<Shader> shader);
        void Unbind();

        // 获取用于 PBR 的纹理，让外部代码能够绑定它们
        // unsigned int GetIrradianceMap() const { return irradianceMap; }
        // unsigned int GetPrefilterMap() const { return prefilterMap; }
        // unsigned int GetBRDFLUT() const { return brdfLUTMap; }

        std::string GetPath() { return path; }

        static Ref<HDRSkybox> Create();
    private:
        // 预计算步骤的私有方法
        void PrecomputeIrradianceMap();
        void PrecomputePrefilterMap();
        void PrecomputeBRDFLUT();

        // 辅助渲染方法
        void InitCube();
        void InitQuad();

    private:
        std::string path;
        
        unsigned int m_CaptureFBO = 0;
        unsigned int m_CaptureRBO = 0;

        Ref<VertexArray> quadArray;
        Ref<VertexArray> cubeArray;

        unsigned int m_HdrTexture = 0;
        unsigned int m_EnvCubemap = 0;
        unsigned int m_IrradianceMap = 0;
        unsigned int m_PrefilterMap = 0;
        unsigned int m_BrdfLUTMap = 0;

        // shader
        Ref<Shader> m_BackgroundShader;
        Ref<Shader> m_EquirectangularToCubemapShader;
        Ref<Shader> m_IrradianceShader;
        Ref<Shader> m_PrefilterShader;
        Ref<Shader> m_BrdfShader;

        // 用于预计算的辅助矩阵
        glm::mat4 captureProjection;
        glm::mat4 captureViews[6];
    };
}