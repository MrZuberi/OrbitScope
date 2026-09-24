#include "rendering/Texture.h"

#include <glad/glad.h>
#include <stb_image.h>

#include <iostream>

Texture::Texture(const std::string& path)
    : m_TextureId(0)
    , m_Valid(false)
{
    stbi_set_flip_vertically_on_load(true);

    int width;
    int height;
    int channels;

    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!data)
    {
        std::cout << "Could not load texture at " << path << ", reason: " << stbi_failure_reason() << ", using a flat color instead" << std::endl;
        return;
    }

    GLenum format = GL_RGB;
    if (channels == 1)
    {
        format = GL_RED;
    }
    else if (channels == 4)
    {
        format = GL_RGBA;
    }

    glGenTextures(1, &m_TextureId);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, channels == 4 ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    m_Valid = true;

    std::cout << "Loaded texture at " << path << " (" << width << "x" << height << ", " << channels << " channels)" << std::endl;
}

Texture::~Texture()
{
    if (m_TextureId != 0)
    {
        glDeleteTextures(1, &m_TextureId);
    }
}

void Texture::Bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);
}

bool Texture::IsValid() const
{
    return m_Valid;
}

unsigned int Texture::GetId() const
{
    return m_TextureId;
}