#include "rendering/Renderer.h"

#include <glad/glad.h>

#include <cmath>
#include <vector>

namespace
{
    const int SphereStacks = 24;
    const int SphereSectors = 36;
    const float Pi = 3.14159265358979323846f;
}

Renderer::Renderer()
    : m_VertexArray(0)
    , m_VertexBuffer(0)
    , m_IndexBuffer(0)
    , m_IndexCount(0)
    , m_View(1.0f)
    , m_Projection(1.0f)
{
    BuildSphereMesh();
    m_Shader = std::make_unique<Shader>("resources/shaders/vertex.glsl", "resources/shaders/fragment.glsl");
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &m_VertexArray);
    glDeleteBuffers(1, &m_VertexBuffer);
    glDeleteBuffers(1, &m_IndexBuffer);
}

void Renderer::BuildSphereMesh()
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int stack = 0; stack <= SphereStacks; ++stack)
    {
        float stackAngle = Pi / 2.0f - stack * (Pi / SphereStacks);
        float xy = cosf(stackAngle);
        float z = sinf(stackAngle);

        for (int sector = 0; sector <= SphereSectors; ++sector)
        {
            float sectorAngle = sector * (2.0f * Pi / SphereSectors);

            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    for (int stack = 0; stack < SphereStacks; ++stack)
    {
        int k1 = stack * (SphereSectors + 1);
        int k2 = k1 + SphereSectors + 1;

        for (int sector = 0; sector < SphereSectors; ++sector, ++k1, ++k2)
        {
            if (stack != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (stack != (SphereStacks - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    m_IndexCount = static_cast<unsigned int>(indices.size());

    glGenVertexArrays(1, &m_VertexArray);
    glGenBuffers(1, &m_VertexBuffer);
    glGenBuffers(1, &m_IndexBuffer);

    glBindVertexArray(m_VertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Renderer::SetViewProjection(const glm::mat4& view, const glm::mat4& projection)
{
    m_View = view;
    m_Projection = projection;
}

void Renderer::DrawSphere(const glm::mat4& model, const glm::vec3& color)
{
    m_Shader->Bind();
    m_Shader->SetMat4("uModel", model);
    m_Shader->SetMat4("uView", m_View);
    m_Shader->SetMat4("uProjection", m_Projection);
    m_Shader->SetVec3("uColor", color);

    glBindVertexArray(m_VertexArray);
    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    m_Shader->Unbind();
}