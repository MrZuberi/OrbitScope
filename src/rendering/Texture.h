// Implements the Texture class that loads and manages texture images
#pragma once

#include <string>

class Texture
{
public:
    explicit Texture(const std::string& path);
    ~Texture();

    void Bind(unsigned int slot) const;
    bool IsValid() const;
    unsigned int GetId() const;

private:
    unsigned int m_TextureId;
    bool m_Valid;
};