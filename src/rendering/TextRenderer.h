#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <stb_truetype.h>

#include "rendering/Shader.h"

class TextRenderer
{
public:
    TextRenderer(const std::string& fontPath, float pixelHeight);
    ~TextRenderer();

    void SetScreenSize(int width, int height);
    void RenderText(const std::string& text, float x, float y, const glm::vec3& color);
    void RenderQuad(float x, float y, float width, float height, const glm::vec3& color, float alpha);

private:
    void LoadFont(const std::string& fontPath, float pixelHeight);
    void Flush(const std::vector<float>& vertices, unsigned int textureId, bool useTexture, const glm::vec3& color, float alpha);

    unsigned int m_VertexArray;
    unsigned int m_VertexBuffer;
    unsigned int m_FontTexture;
    std::unique_ptr<Shader> m_Shader;
    std::unique_ptr<stbtt_bakedchar[]> m_BakedChars;
    int m_AtlasWidth;
    int m_AtlasHeight;
    int m_FirstChar;
    int m_CharCount;
    glm::mat4 m_Projection;
};