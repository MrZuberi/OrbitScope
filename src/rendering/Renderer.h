#pragma once

#include <memory>

#include "rendering/Shader.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void DrawTriangle();

private:
    unsigned int m_VertexArray;
    unsigned int m_VertexBuffer;
    std::unique_ptr<Shader> m_Shader;
};