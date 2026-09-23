#version 330 core

in vec2 vTexCoord;

out vec4 FragColor;

uniform sampler2D uFontAtlas;
uniform vec3 uColor;
uniform float uAlpha;
uniform bool uUseTexture;

void main()
{
    if (uUseTexture)
    {
        float alpha = texture(uFontAtlas, vTexCoord).r;
        FragColor = vec4(uColor, alpha * uAlpha);
    }
    else
    {
        FragColor = vec4(uColor, uAlpha);
    }
}