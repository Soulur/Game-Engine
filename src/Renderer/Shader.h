#pragma once

#include "src/Core/Base.h"
#include <string>

#include <glm/glm.hpp>

typedef unsigned int GLenum;

namespace Mc
{
    class Shader
    {
    public:
        Shader(const std::string &filePath);
        ~Shader();

        void Bind() const;
        void Unbind() const;

        uint32_t Get() { return m_RendererID; }

    private:
        std::string LoadShaderFile(const std::string &filepath);
        std::unordered_map<GLenum, std::string> PreProcess(const std::string &source);
        void Complie(const std::unordered_map<GLenum, std::string> &shaderSourc);
    private:
    	uint32_t m_RendererID;
		std::string m_Name;
    public:
        static Ref<Shader> Create(const std::string &filePath);

    public:
        void SetInt(const std::string &name, int value);
        void SetBool(const std::string &name, bool value);
        void SetIntArray(const std::string &name, int *values, uint32_t count);
        void SetFloat(const std::string &name, float value);
        void SetFloat2(const std::string &name, const glm::vec2 &value);
        void SetFloat3(const std::string &name, const glm::vec3 &value);
        void SetFloat4(const std::string &name, const glm::vec4 &value);
        void SetMat4(const std::string &name, const glm::mat4 &value);
        void SetMat4Array(const std::string &name, const glm::mat4 *values, uint32_t count);

    public:
        void UploadUniformInt(const std::string &name, int values);
        void UploadUniformBool(const std::string &name, bool values);
        void UploadUniformIntArray(const std::string &name, int *values, uint32_t count);
        void UploadUniformFloat(const std::string &name, float values);
        void UploadUniformFloat2(const std::string &name, const glm::vec2 &values);
        void UploadUniformFloat3(const std::string &name, const glm::vec3 &values);
        void UploadUniformFloat4(const std::string &name, const glm::vec4 &values);
        void UploadUniformMat4(const std::string &name, const glm::mat4 &matrix);
    };
}