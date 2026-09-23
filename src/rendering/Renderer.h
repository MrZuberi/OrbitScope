#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "rendering/Shader.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void SetViewProjection(const glm::mat4& view, const glm::mat4& projection);
    void DrawSphere(const glm::mat4& model, const glm::vec3& color, bool isLightSource);
    void DrawOrbitLine(const glm::mat4& model, const glm::vec3& color);
    void DrawStars();

private:
    void BuildSphereMesh();
    void BuildOrbitMesh();
    void BuildStarMesh();

    unsigned int m_VertexArray;
    unsigned int m_VertexBuffer;
    unsigned int m_IndexBuffer;
    unsigned int m_IndexCount;

    unsigned int m_OrbitVertexArray;
    unsigned int m_OrbitVertexBuffer;
    unsigned int m_OrbitVertexCount;

    unsigned int m_StarVertexArray;
    unsigned int m_StarVertexBuffer;
    unsigned int m_StarVertexCount;

    std::unique_ptr<Shader> m_Shader;
    glm::mat4 m_View;
    glm::mat4 m_Projection;
};