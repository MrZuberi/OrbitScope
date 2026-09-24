#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "rendering/Shader.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void SetViewProjection(const glm::mat4& view, const glm::mat4& projection);
    void SetCameraPosition(const glm::vec3& position);
    void DrawSphere(const glm::mat4& model, const glm::vec3& color, bool isLightSource, unsigned int textureId);
    void DrawOrbitLine(const glm::mat4& model, const glm::vec3& color);
    void DrawDynamicLineLoop(const std::vector<glm::vec3>& points, const glm::vec3& color);
    void DrawRing(const glm::mat4& model, unsigned int textureId);
    void DrawStars();

private:
    void BuildSphereMesh();
    void BuildOrbitMesh();
    void BuildStarMesh();
    void BuildRingMesh();

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

    unsigned int m_DynamicLineVertexArray;
    unsigned int m_DynamicLineVertexBuffer;

    unsigned int m_RingVertexArray;
    unsigned int m_RingVertexBuffer;
    unsigned int m_RingVertexCount;

    std::unique_ptr<Shader> m_Shader;
    glm::mat4 m_View;
    glm::mat4 m_Projection;
    glm::vec3 m_CameraPosition;
};