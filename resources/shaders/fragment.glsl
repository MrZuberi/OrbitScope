#version 330 core

in vec3 vNormal;
in vec3 vFragPos;

out vec4 FragColor;

uniform vec3 uColor;
uniform bool uUseLighting;
uniform bool uIsLightSource;
uniform vec3 uLightPos;

void main()
{
    if (!uUseLighting || uIsLightSource)
    {
        FragColor = vec4(uColor, 1.0);
        return;
    }

    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);
    float diffuseStrength = max(dot(normal, lightDir), 0.0);

    vec3 ambient = 0.15 * uColor;
    vec3 diffuse = diffuseStrength * uColor;

    FragColor = vec4(ambient + diffuse, 1.0);
}