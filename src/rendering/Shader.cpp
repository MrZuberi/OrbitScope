// Contains source code for the Shader.cpp file
#include "rendering/Shader.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
    : m_ProgramId(0)
{
    std::string vertexSource = ReadFile(vertexPath);
    std::string fragmentSource = ReadFile(fragmentPath);
    m_ProgramId = CreateProgram(vertexSource, fragmentSource);
}

Shader::~Shader()
{
    glDeleteProgram(m_ProgramId);
}

std::string Shader::ReadFile(const std::string& path)
{
    std::ifstream file(path);
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

unsigned int Shader::CompileShader(unsigned int type, const std::string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int success;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(id, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }

    return id;
}

unsigned int Shader::CreateProgram(const std::string& vertexSource, const std::string& fragmentSource)
{
    unsigned int vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void Shader::Bind() const
{
    glUseProgram(m_ProgramId);
}

void Shader::Unbind() const
{
    glUseProgram(0);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& value) const
{
    int location = glGetUniformLocation(m_ProgramId, name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    int location = glGetUniformLocation(m_ProgramId, name.c_str());
    glUniform3fv(location, 1, glm::value_ptr(value));
}

void Shader::SetBool(const std::string& name, bool value) const
{
    int location = glGetUniformLocation(m_ProgramId, name.c_str());
    glUniform1i(location, value ? 1 : 0);
}

void Shader::SetFloat(const std::string& name, float value) const
{
    int location = glGetUniformLocation(m_ProgramId, name.c_str());
    glUniform1f(location, value);
}

void Shader::SetInt(const std::string& name, int value) const
{
    int location = glGetUniformLocation(m_ProgramId, name.c_str());
    glUniform1i(location, value);
}