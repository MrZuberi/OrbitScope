#include "rendering/TextRenderer.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_truetype.h>

#include <fstream>
#include <iostream>

namespace
{
    const int AtlasSize = 512;
}

TextRenderer::TextRenderer(const std::string& fontPath, float pixelHeight)
    : m_VertexArray(0)
    , m_VertexBuffer(0)
    , m_FontTexture(0)
    , m_AtlasWidth(AtlasSize)
    , m_AtlasHeight(AtlasSize)
    , m_FirstChar(32)
    , m_CharCount(96)
    , m_Projection(1.0f)
{
    LoadFont(fontPath, pixelHeight);

    m_Shader = std::make_unique<Shader>("resources/shaders/text.vert", "resources/shaders/text.frag");

    glGenVertexArrays(1, &m_VertexArray);
    glGenBuffers(1, &m_VertexBuffer);

    glBindVertexArray(m_VertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

TextRenderer::~TextRenderer()
{
    glDeleteTextures(1, &m_FontTexture);
    glDeleteBuffers(1, &m_VertexBuffer);
    glDeleteVertexArrays(1, &m_VertexArray);
}

void TextRenderer::LoadFont(const std::string& fontPath, float pixelHeight)
{
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);

    if (!file)
    {
        std::cerr << "Could not open font file at " << fontPath << std::endl;

        std::vector<unsigned char> blankPixel(4, 255);
        m_BakedChars = std::make_unique<stbtt_bakedchar[]>(m_CharCount);

        glGenTextures(1, &m_FontTexture);
        glBindTexture(GL_TEXTURE_2D, m_FontTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, blankPixel.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> fontBuffer(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(fontBuffer.data()), size);

    std::vector<unsigned char> bitmap(static_cast<size_t>(m_AtlasWidth * m_AtlasHeight));

    m_BakedChars = std::make_unique<stbtt_bakedchar[]>(m_CharCount);

    stbtt_BakeFontBitmap(fontBuffer.data(), 0, pixelHeight, bitmap.data(), m_AtlasWidth, m_AtlasHeight, m_FirstChar, m_CharCount, m_BakedChars.get());

    glGenTextures(1, &m_FontTexture);
    glBindTexture(GL_TEXTURE_2D, m_FontTexture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_AtlasWidth, m_AtlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRenderer::SetScreenSize(int width, int height)
{
    m_Projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f);
}

void TextRenderer::Flush(const std::vector<float>& vertices, unsigned int textureId, bool useTexture, const glm::vec3& color, float alpha)
{
    if (vertices.empty())
    {
        return;
    }

    m_Shader->Bind();
    m_Shader->SetMat4("uProjection", m_Projection);
    m_Shader->SetVec3("uColor", color);
    m_Shader->SetFloat("uAlpha", alpha);
    m_Shader->SetBool("uUseTexture", useTexture);

    if (useTexture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
    }

    glBindVertexArray(m_VertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(vertices.size() / 4));

    glBindVertexArray(0);
    m_Shader->Unbind();
}

void TextRenderer::RenderText(const std::string& text, float x, float y, const glm::vec3& color)
{
    std::vector<float> vertices;
    float penX = x;
    float penY = y;

    for (char character : text)
    {
        if (character < m_FirstChar || character >= m_FirstChar + m_CharCount)
        {
            continue;
        }

        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(m_BakedChars.get(), m_AtlasWidth, m_AtlasHeight, character - m_FirstChar, &penX, &penY, &quad, 1);

        vertices.push_back(quad.x0); vertices.push_back(quad.y0); vertices.push_back(quad.s0); vertices.push_back(quad.t0);
        vertices.push_back(quad.x1); vertices.push_back(quad.y0); vertices.push_back(quad.s1); vertices.push_back(quad.t0);
        vertices.push_back(quad.x1); vertices.push_back(quad.y1); vertices.push_back(quad.s1); vertices.push_back(quad.t1);

        vertices.push_back(quad.x0); vertices.push_back(quad.y0); vertices.push_back(quad.s0); vertices.push_back(quad.t0);
        vertices.push_back(quad.x1); vertices.push_back(quad.y1); vertices.push_back(quad.s1); vertices.push_back(quad.t1);
        vertices.push_back(quad.x0); vertices.push_back(quad.y1); vertices.push_back(quad.s0); vertices.push_back(quad.t1);
    }

    Flush(vertices, m_FontTexture, true, color, 1.0f);
}

void TextRenderer::RenderQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha)
{
    std::vector<float> vertices = {
        x, y, 0.0f, 0.0f,
        x + width, y, 0.0f, 0.0f,
        x + width, y + height, 0.0f, 0.0f,

        x, y, 0.0f, 0.0f,
        x + width, y + height, 0.0f, 0.0f,
        x, y + height, 0.0f, 0.0f
    };

    Flush(vertices, 0, false, color, alpha);
}