#version 330 core

in vec3 vNormal;
in vec3 vFragPos;
in vec2 vTexCoord;

out vec4 FragColor;

uniform vec3 uColor;
uniform bool uUseLighting;
uniform bool uIsLightSource;
uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform sampler2D uTexture;
uniform bool uUseTexture;

void main()
{
    vec3 baseColor = uUseTexture ? texture(uTexture, vTexCoord).rgb : uColor;

    if (!uUseLighting || uIsLightSource)
    {
        FragColor = vec4(baseColor, 1.0);
        return;
    }

    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float diffuseStrength = max(dot(normal, lightDir), 0.0);
    float specularStrength = pow(max(dot(normal, halfwayDir), 0.0), 32.0) * 0.35;
    float rimStrength = pow(1.0 - max(dot(normal, viewDir), 0.0), 3.0) * 0.25;

    vec3 ambient = 0.15 * baseColor;
    vec3 diffuse = diffuseStrength * baseColor;
    vec3 specular = vec3(specularStrength);
    vec3 rim = rimStrength * baseColor;

    FragColor = vec4(ambient + diffuse + specular + rim, 1.0);
}