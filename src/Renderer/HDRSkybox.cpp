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
        backgroundShader = Shader::Create("Assets/shaders/hdr/background.glsl");
        equirectangularToCubemapShader = Shader::Create("Assets/shaders/hdr/cubemap.glsl");
        irradianceShader = Shader::Create("Assets/shaders/hdr/irradiance_convolution.glsl");
        prefilterShader = Shader::Create("Assets/shaders/hdr/prefilter.glsl");
        brdfShader = Shader::Create("Assets/shaders/hdr/brdf.glsl");

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
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);
    }

    HDRSkybox::~HDRSkybox()
    {
        glDeleteTextures(1, &hdrTexture);
        glDeleteTextures(1, &envCubemap);
        glDeleteTextures(1, &irradianceMap);
        glDeleteTextures(1, &prefilterMap);
        glDeleteTextures(1, &brdfLUTMap);
        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);
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
            glGenTextures(1, &hdrTexture);
            glBindTexture(GL_TEXTURE_2D, hdrTexture);
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
        glGenTextures(1, &envCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
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
        equirectangularToCubemapShader->Bind();
        equirectangularToCubemapShader->SetInt("equirectangularMap", 0);
        equirectangularToCubemapShader->SetMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);

        glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            equirectangularToCubemapShader->SetMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            cubeArray->Bind();
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
        
        // --- 状态恢复 ---
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        // --- 状态恢复结束 ---

        // then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
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
        backgroundShader->Bind();
        backgroundShader->SetMat4("projection", projection);
        backgroundShader->SetMat4("view", view);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        // glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        // glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        backgroundShader->SetInt("environmentMap", 0);
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
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        shader->SetInt("irradianceMap", 32);

        glActiveTexture(GL_TEXTURE0 + 33);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        shader->SetInt("prefilterMap", 33);

        glActiveTexture(GL_TEXTURE0 + 34);
        glBindTexture(GL_TEXTURE_2D, brdfLUTMap);
        shader->SetInt("brdfLUT", 34);

        shader->Unbind();
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
        glGenTextures(1, &irradianceMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

        // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
        // -----------------------------------------------------------------------------
        irradianceShader->Bind();
        irradianceShader->SetInt("environmentMap", 0);
        irradianceShader->SetMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

        glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            irradianceShader->SetMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
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
        glGenTextures(1, &prefilterMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
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
        prefilterShader->Bind();
        prefilterShader->SetInt("environmentMap", 0);
        prefilterShader->SetMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        unsigned int maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
        {
            // reisze framebuffer according to mip-level size.
            unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            prefilterShader->SetFloat("roughness", roughness);
            for (unsigned int i = 0; i < 6; ++i)
            {
                prefilterShader->SetMat4("view", captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

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
        glGenTextures(1, &brdfLUTMap);

        // pre-allocate enough memory for the LUT texture.
        glBindTexture(GL_TEXTURE_2D, brdfLUTMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 512, 512, 0, GL_RGBA, GL_FLOAT, 0);
        // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTMap, 0);

        glViewport(0, 0, 512, 512);
        brdfShader->Bind();
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

    Scope<HDRSkybox> HDRSkybox::Create()
    {
        return CreateScope<HDRSkybox>();
    }
}