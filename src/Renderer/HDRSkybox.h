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

        // 获取用于 PBR 的纹理，让外部代码能够绑定它们
        // unsigned int GetIrradianceMap() const { return irradianceMap; }
        // unsigned int GetPrefilterMap() const { return prefilterMap; }
        // unsigned int GetBRDFLUT() const { return brdfLUTMap; }

        std::string GetPath() { return path; }

        static Scope<HDRSkybox> Create();
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
        
        unsigned int captureFBO = 0;
        unsigned int captureRBO = 0;

        Ref<VertexArray> quadArray;
        Ref<VertexArray> cubeArray;

        unsigned int hdrTexture = 0;
        unsigned int envCubemap = 0;
        unsigned int irradianceMap = 0;
        unsigned int prefilterMap = 0;
        unsigned int brdfLUTMap = 0;

        // shader
        Ref<Shader> backgroundShader;
        Ref<Shader> equirectangularToCubemapShader;
        Ref<Shader> irradianceShader;
        Ref<Shader> prefilterShader;
        Ref<Shader> brdfShader;

        // 用于预计算的辅助矩阵
        glm::mat4 captureProjection;
        glm::mat4 captureViews[6];
    };
}