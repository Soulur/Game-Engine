#include "HDRSkybox.h"

#include "src/Core/Application.h"
#include "src/Renderer/Framebuffer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

namespace Mc
{
    HDRSkybox::HDRSkybox()
    {
        path = "";

        // create shader
        m_BackgroundShader = Shader::Create("Assets/shaders/hdr/background.glsl");
        m_EquirectangularToCubemapShader = Shader::Create("Assets/shaders/hdr/cubemap.glsl");
        m_IrradianceShader = Shader::Create("Assets/shaders/hdr/irradiance_convolution.glsl");
        m_PrefilterShader = Shader::Create("Assets/shaders/hdr/prefilter.glsl");
        m_BrdfShader = Shader::Create("Assets/shaders/hdr/brdf.glsl");

        InitCube();
        InitQuad();

        captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
          
        {
            captureViews[0] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
            captureViews[1] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
            captureViews[2] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            captureViews[3] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
            captureViews[4] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
            captureViews[5] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        }

        // 生成 FBO 和 RBO
        glGenFramebuffers(1, &m_CaptureFBO);
        glGenRenderbuffers(1, &m_CaptureRBO);
    }

    HDRSkybox::~HDRSkybox()
    {
        glDeleteTextures(1, &m_HdrTexture);
        glDeleteTextures(1, &m_EnvCubemap);
        glDeleteTextures(1, &m_IrradianceMap);
        glDeleteTextures(1, &m_PrefilterMap);
        glDeleteTextures(1, &m_BrdfLUTMap);
        glDeleteFramebuffers(1, &m_CaptureFBO);
        glDeleteRenderbuffers(1, &m_CaptureRBO);
    }

    void HDRSkybox::LoadHDRMap(const std::string &filepath)
    {
        path = filepath;

        // --- 状态保存 ---
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        GLint currentFBO;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
        // --- 状态保存结束 ---
        
        // pbr: load the HDR environment map
        // ---------------------------------
        stbi_set_flip_vertically_on_load(true);
        int width, height, nrComponents;
        float *data = stbi_loadf(filepath.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glGenTextures(1, &m_HdrTexture);
            glBindTexture(GL_TEXTURE_2D, m_HdrTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            LOG_CORE_ERROR("Failed to load HDR image.");
        }

        // pbr: setup cubemap to render to and attach to framebuffer
        // ---------------------------------------------------------
        glGenTextures(1, &m_EnvCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvCubemap);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        // pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
        // ----------------------------------------------------------------------------------------------
        m_EquirectangularToCubemapShader->Bind();
        m_EquirectangularToCubemapShader->SetInt("equirectangularMap", 0);
        m_EquirectangularToCubemapShader->SetMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_HdrTexture);

        glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
        glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            m_EquirectangularToCubemapShader->SetMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_EnvCubemap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            cubeArray->Bind();
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
        
        // --- 状态恢复 ---
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        // --- 状态恢复结束 ---

        // then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvCubemap);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        // 预计算辐射度图
        PrecomputeIrradianceMap();

        // 预计算预过滤环境贴图
        PrecomputePrefilterMap();

        // 预计算 BRDF LUT
        PrecomputeBRDFLUT();
    }

    void HDRSkybox::Render(const glm::mat4 &projection, const glm::mat4 &view)
    {
        glDepthFunc(GL_LEQUAL);
        
        // render skybox (render as last to prevent overdraw)
        m_BackgroundShader->Bind();
        m_BackgroundShader->SetMat4("projection", projection);
        m_BackgroundShader->SetMat4("view", view);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvCubemap);
        // glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        // glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        m_BackgroundShader->SetInt("environmentMap", 0);
        cubeArray->Bind();
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // brdfShader->Bind();
        // quadArray->Bind();
        // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glDepthFunc(GL_LESS);
    }

    void HDRSkybox::Bind(Ref<Shader> shader)
    {
        shader->Bind();

        glActiveTexture(GL_TEXTURE0 + 32);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceMap);
        shader->SetInt("irradianceMap", 32);

        glActiveTexture(GL_TEXTURE0 + 33);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_PrefilterMap);
        shader->SetInt("prefilterMap", 33);

        glActiveTexture(GL_TEXTURE0 + 34);
        glBindTexture(GL_TEXTURE_2D, m_BrdfLUTMap);
        shader->SetInt("brdfLUT", 34);

        shader->Unbind();
    }

    void HDRSkybox::Unbind()
    {
        glActiveTexture(GL_TEXTURE0 + 32);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        glActiveTexture(GL_TEXTURE0 + 33);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        glActiveTexture(GL_TEXTURE0 + 34);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void HDRSkybox::PrecomputeIrradianceMap()
    {
        // --- 状态保存 ---
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        GLint currentFBO;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
        // --- 状态保存结束 ---
        
        // pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
        // --------------------------------------------------------------------------------
        // unsigned int irradianceMap;
        glGenTextures(1, &m_IrradianceMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceMap);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

        // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
        // -----------------------------------------------------------------------------
        m_IrradianceShader->Bind();
        m_IrradianceShader->SetInt("environmentMap", 0);
        m_IrradianceShader->SetMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvCubemap);

        glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
        glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            m_IrradianceShader->SetMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_IrradianceMap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            cubeArray->Bind();
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
        
        // --- 状态恢复 ---
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        // --- 状态恢复结束 ---
    }

    void HDRSkybox::PrecomputePrefilterMap()
    {
        // --- 状态保存 ---
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        GLint currentFBO;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
        // --- 状态保存结束 ---
        
        // pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
        // --------------------------------------------------------------------------------
        // unsigned int prefilterMap;
        glGenTextures(1, &m_PrefilterMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_PrefilterMap);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
        // ----------------------------------------------------------------------------------------------------
        m_PrefilterShader->Bind();
        m_PrefilterShader->SetInt("environmentMap", 0);
        m_PrefilterShader->SetMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvCubemap);

        glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
        unsigned int maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
        {
            // reisze framebuffer according to mip-level size.
            unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            m_PrefilterShader->SetFloat("roughness", roughness);
            for (unsigned int i = 0; i < 6; ++i)
            {
                m_PrefilterShader->SetMat4("view", captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_PrefilterMap, mip);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                cubeArray->Bind();
                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            }
        }
        
        // --- 状态恢复 ---
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        // --- 状态恢复结束 ---
    }

    void HDRSkybox::PrecomputeBRDFLUT()
    {
        // --- 状态保存 ---
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        GLint currentFBO;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
        // --- 状态保存结束 ---
        
        // pbr: generate a 2D LUT from the BRDF equations used.
        // ----------------------------------------------------
        // unsigned int brdfLUTTexture;
        glGenTextures(1, &m_BrdfLUTMap);

        // pre-allocate enough memory for the LUT texture.
        glBindTexture(GL_TEXTURE_2D, m_BrdfLUTMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 512, 512, 0, GL_RGBA, GL_FLOAT, 0);
        // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
        glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BrdfLUTMap, 0);

        glViewport(0, 0, 512, 512);
        m_BrdfShader->Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        quadArray->Bind();
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // --- 状态恢复 ---
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        // --- 状态恢复结束 ---
    }

    void HDRSkybox::InitCube()
    {
        float cubeVertices[] = {
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f,  1.0f, 1.0f,
             1.0f,  1.0f, 1.0f,
             1.0f, -1.0f, 1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,

            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, 1.0f, -1.0f,
             1.0f, 1.0f, -1.0f,
             1.0f, 1.0f,  1.0f,
            -1.0f, 1.0f,  1.0f,
        };

        uint32_t cubeIndices[] = {
            // Back face (反向)
            2, 1, 0, 0, 3, 2,
            // Front face (反向)
            6, 5, 4, 4, 7, 6,
            // Left face (反向)
            10, 9, 8, 8, 11, 10,
            // Right face (反向)
            14, 13, 12, 12, 15, 14,
            // Bottom face (反向)
            18, 17, 16, 16, 19, 18,
            // Top face (反向)
            22, 21, 20, 20, 23, 22
        };
        // cube
        cubeArray = VertexArray::Create();

        Ref<VertexBuffer> cubeBuffer = VertexBuffer::Create(cubeVertices, sizeof(cubeVertices));
        cubeBuffer->SetLayout({
            {ShaderDataType::Float3, "a_Position"},
        });
        cubeArray->AddVertexBuffer(cubeBuffer);
        Ref<IndexBuffer> cubeIndexBuffer = IndexBuffer::Create(cubeIndices, 36);
        cubeArray->SetIndexBuffer(cubeIndexBuffer);
    }


    void HDRSkybox::InitQuad()
    {
        // quad
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        uint32_t quadIndices[] = {
            // 2, 1, 0, 0, 3, 2,
            0, 1, 2, 2, 3, 0,
        };

        quadArray = VertexArray::Create();

        Ref<VertexBuffer> quadBuffer = VertexBuffer::Create(quadVertices, sizeof(quadVertices));
        quadBuffer->SetLayout({
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float2, "a_TexCoords"},
        });
        quadArray->AddVertexBuffer(quadBuffer);
        Ref<IndexBuffer> quadIndexBuffer = IndexBuffer::Create(quadIndices, 6);
        quadArray->SetIndexBuffer(quadIndexBuffer);
    }

    Ref<HDRSkybox> HDRSkybox::Create()
    {
        return CreateRef<HDRSkybox>();
    }
}