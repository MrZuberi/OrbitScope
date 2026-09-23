#include "rendering/Renderer.h"

#include <glad/glad.h>

#include <cmath>
#include <cstdlib>
#include <vector>

namespace
{
    const int SphereStacks = 24;
    const int SphereSectors = 36;
    const int OrbitSegments = 128;
    const int StarCount = 800;
    const float StarShellRadius = 150.0f;
    const float Pi = 3.14159265358979323846f;
}

Renderer::Renderer()
    : m_VertexArray(0)
    , m_VertexBuffer(0)
    , m_IndexBuffer(0)
    , m_IndexCount(0)
    , m_OrbitVertexArray(0)
    , m_OrbitVertexBuffer(0)
    , m_OrbitVertexCount(0)
    , m_StarVertexArray(0)
    , m_StarVertexBuffer(0)
    , m_StarVertexCount(0)
    , m_View(1.0f)
    , m_Projection(1.0f)
{
    BuildSphereMesh();
    BuildOrbitMesh();
    BuildStarMesh();
    m_Shader = std::make_unique<Shader>("resources/shaders/vertex.glsl", "resources/shaders/fragment.glsl");
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &m_VertexArray);
    glDeleteBuffers(1, &m_VertexBuffer);
    glDeleteBuffers(1, &m_IndexBuffer);
    glDeleteVertexArrays(1, &m_OrbitVertexArray);
    glDeleteBuffers(1, &m_OrbitVertexBuffer);
    glDeleteVertexArrays(1, &m_StarVertexArray);
    glDeleteBuffers(1, &m_StarVertexBuffer);
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

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Renderer::BuildOrbitMesh()
{
    std::vector<float> vertices;

    for (int i = 0; i < OrbitSegments; ++i)
    {
        float angle = 2.0f * Pi * (static_cast<float>(i) / static_cast<float>(OrbitSegments));
        vertices.push_back(cosf(angle));
        vertices.push_back(0.0f);
        vertices.push_back(sinf(angle));
    }

    m_OrbitVertexCount = static_cast<unsigned int>(vertices.size() / 3);

    glGenVertexArrays(1, &m_OrbitVertexArray);
    glGenBuffers(1, &m_OrbitVertexBuffer);

    glBindVertexArray(m_OrbitVertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, m_OrbitVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Renderer::BuildStarMesh()
{
    srand(1337);

    std::vector<float> vertices;

    for (int i = 0; i < StarCount; ++i)
    {
        float theta = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * Pi;
        float phi = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * Pi;

        float x = StarShellRadius * sinf(theta) * cosf(phi);
        float y = StarShellRadius * cosf(theta);
        float z = StarShellRadius * sinf(theta) * sinf(phi);

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
    }

    m_StarVertexCount = static_cast<unsigned int>(vertices.size() / 3);

    glGenVertexArrays(1, &m_StarVertexArray);
    glGenBuffers(1, &m_StarVertexBuffer);

    glBindVertexArray(m_StarVertexArray);

    glBindBuffer(GL_ARRAY_BUFFER, m_StarVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Renderer::SetViewProjection(const glm::mat4& view, const glm::mat4& projection)
{
    m_View = view;
    m_Projection = projection;
}

void Renderer::DrawSphere(const glm::mat4& model, const glm::vec3& color, bool isLightSource)
{
    m_Shader->Bind();
    m_Shader->SetMat4("uModel", model);
    m_Shader->SetMat4("uView", m_View);
    m_Shader->SetMat4("uProjection", m_Projection);
    m_Shader->SetVec3("uColor", color);
    m_Shader->SetVec3("uLightPos", glm::vec3(0.0f, 0.0f, 0.0f));
    m_Shader->SetBool("uUseLighting", true);
    m_Shader->SetBool("uIsLightSource", isLightSource);

    glBindVertexArray(m_VertexArray);
    glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    m_Shader->Unbind();
}

void Renderer::DrawOrbitLine(const glm::mat4& model, const glm::vec3& color)
{
    m_Shader->Bind();
    m_Shader->SetMat4("uModel", model);
    m_Shader->SetMat4("uView", m_View);
    m_Shader->SetMat4("uProjection", m_Projection);
    m_Shader->SetVec3("uColor", color);
    m_Shader->SetBool("uUseLighting", false);
    m_Shader->SetBool("uIsLightSource", false);

    glBindVertexArray(m_OrbitVertexArray);
    glDrawArrays(GL_LINE_LOOP, 0, m_OrbitVertexCount);
    glBindVertexArray(0);

    m_Shader->Unbind();
}

void Renderer::DrawStars()
{
    m_Shader->Bind();
    m_Shader->SetMat4("uModel", glm::mat4(1.0f));
    m_Shader->SetMat4("uView", m_View);
    m_Shader->SetMat4("uProjection", m_Projection);
    m_Shader->SetVec3("uColor", glm::vec3(1.0f, 1.0f, 1.0f));
    m_Shader->SetBool("uUseLighting", false);
    m_Shader->SetBool("uIsLightSource", false);

    glPointSize(2.0f);
    glBindVertexArray(m_StarVertexArray);
    glDrawArrays(GL_POINTS, 0, m_StarVertexCount);
    glBindVertexArray(0);

    m_Shader->Unbind();
}