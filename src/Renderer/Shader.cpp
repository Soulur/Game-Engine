#include "Shader.h"

#include <fstream>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace Mc
{
    static GLenum ShaderTypeFromString(const std::string &type)
    {
        if (type == "vertex")
            return GL_VERTEX_SHADER;
        if (type == "fragment" || type == "pixel")
            return GL_FRAGMENT_SHADER;
        else if (type == "geometry")
            return GL_GEOMETRY_SHADER;
        return 0;
    }

    Shader::Shader(const std::string &filePath)
    {
        std::string source = LoadShaderFile(filePath);
        auto shaderSoures = PreProcess(source);
        Complie(shaderSoures);

        // Extract name from filepath
        auto lastSlash = filePath.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = filePath.rfind('.');
        auto count = lastDot == std::string::npos ? filePath.size() - lastSlash : lastDot - lastSlash;
        m_Name = filePath.substr(lastSlash, count);
    }

    Shader::~Shader()
    {
        glDeleteProgram(m_RendererID);
    }

    void Shader::Bind() const
    {
        glUseProgram(m_RendererID);
    }

    void Shader::Unbind() const
    {
        glUseProgram(0);
    }

    std::string Shader::LoadShaderFile(const std::string &filepath)
    {
        std::string result;
        std::ifstream in(filepath, std::ios::in | std::ios::binary);
        if (in)
        {
            in.seekg(0, std::ios::end);
            size_t size = in.tellg();
            if (size != -1)
            {
                result.resize(size);
                in.seekg(0, std::ios::beg);
                in.read(&result[0], size);
                in.close();
            }
        }
        return result;
    }

    std::unordered_map<GLenum, std::string> Shader::PreProcess(const std::string &source)
    {
        std::unordered_map<GLenum, std::string> shaderSources;

        const char *typeToken = "#type";
        size_t typeTokenLength = strlen(typeToken);
        size_t pos = source.find(typeToken, 0);
        while (pos != std::string::npos)
        {
            size_t eol = source.find_first_of("\r\n", pos);
            size_t begin = pos + typeTokenLength + 1;
            std::string type = source.substr(begin, eol - begin);

            size_t nextLinePos = source.find_first_not_of("\r\n", eol);
            pos = source.find(typeToken, nextLinePos);

            shaderSources[ShaderTypeFromString(type)] = (pos == std::string::npos)
                                                            ? source.substr(nextLinePos)
                                                            : source.substr(nextLinePos, pos - nextLinePos);
        }
        return shaderSources;
    }

    void Shader::Complie(const std::unordered_map<GLenum, std::string> &shaderSourc)
    {
        GLuint program = glCreateProgram();
        std::array<GLenum, 3> glShaderIDs{};
        int glShaderIDIndex = 0;
        for (auto &kv : shaderSourc)
        {
            GLenum type = kv.first;
            const std::string &source = kv.second;

            GLuint shader = glCreateShader(type);
            const GLchar *sourceCstr = source.c_str();
            glShaderSource(shader, 1, &sourceCstr, 0);
            glCompileShader(shader);

            GLint isCompiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
            if (isCompiled == GL_FALSE)
            {
                GLint maxLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
                std::vector<GLchar> infoLog(maxLength);
                glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);
                glDeleteShader(shader);
                break;
            }

            glAttachShader(program, shader);
            glShaderIDs[glShaderIDIndex++] = shader;
        }

        m_RendererID = program;
        glLinkProgram(program);

        GLint isLinked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, (int *)&isLinked);
        if (isLinked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
            std::vector<GLchar> infoLog(maxLength);
            glGetProgramInfoLog(program, maxLength, &maxLength, &infoLog[0]);
            glDeleteProgram(program);
            for (auto id : glShaderIDs)
                glDeleteShader(id);
            return;
        }

        // Always detach shaders after a successful link.
        for (auto id : glShaderIDs)
        {
            glDetachShader(program, id);
            glDeleteShader(id);
        }
    }

    Ref<Shader> Shader::Create(const std::string &filePath)
    {
        return CreateRef<Shader>(filePath);
    }

    void Shader::SetInt(const std::string &name, int value)
    {
        UploadUniformInt(name, value);
    }

    void Shader::SetBool(const std::string &name, bool value)
    {
        UploadUniformBool(name, value);
    }

    void Shader::SetIntArray(const std::string &name, int *values, uint32_t count)
    {
        UploadUniformIntArray(name, values, count);
    }

    void Shader::SetFloat(const std::string &name, float value)
    {
        UploadUniformFloat(name, value);
    }

    void Shader::SetFloat2(const std::string &name, const glm::vec2 &value)
    {
        UploadUniformFloat2(name, value);
    }

    void Shader::SetFloat3(const std::string &name, const glm::vec3 &value)
    {
        UploadUniformFloat3(name, value);
    }

    void Shader::SetFloat4(const std::string &name, const glm::vec4 &value)
    {
        UploadUniformFloat4(name, value);
    }

    void Shader::SetMat4(const std::string &name, const glm::mat4 &value)
    {
        UploadUniformMat4(name, value);
    }

    void Shader::UploadUniformInt(const std::string &name, int values)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1i(location, values);
    }

    void Shader::UploadUniformBool(const std::string &name, bool values)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1i(location, values);
    }

    void Shader::UploadUniformIntArray(const std::string &name, int *values, uint32_t count)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1iv(location, count, values);
    }

    void Shader::UploadUniformFloat(const std::string &name, float values)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform1f(location, values);
    }

    void Shader::UploadUniformFloat2(const std::string &name, const glm::vec2 &values)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform2f(location, values.x, values.y);
    }

    void Shader::UploadUniformFloat3(const std::string &name, const glm::vec3 &values)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform3f(location, values.x, values.y, values.z);
    }

    void Shader::UploadUniformFloat4(const std::string &name, const glm::vec4 &values)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniform4f(location, values.x, values.y, values.z, values.w);
    }
    
    void Shader::UploadUniformMat4(const std::string &name, const glm::mat4 &matrix)
    {
        GLint location = glGetUniformLocation(m_RendererID, name.c_str());
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    }
}
