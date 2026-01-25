#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aWeights;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_BONES = 250;
uniform mat4 finalBonesMatrices[MAX_BONES];
uniform bool useAnimation;

void main()
{
    vec4 totalPosition = vec4(0.0);
    vec3 totalNormal = vec3(0.0);
    
    if (useAnimation)
    {
        bool hasValidBone = false;
        // (Línea 29 nueva)
        for (int i = 0; i < 4; i++)
        {
            // AÑADIMOS: && aWeights[i] > 0.0
            // Esto evita que intentemos leer una matriz si el hueso no influye en nada.
            if (aBoneIDs[i] >= 0 && aBoneIDs[i] < MAX_BONES && aWeights[i] > 0.0)
            {
                hasValidBone = true;
                vec4 localPosition = finalBonesMatrices[aBoneIDs[i]] * vec4(aPos, 1.0);
                totalPosition += localPosition * aWeights[i];
                
                vec3 localNormal = mat3(finalBonesMatrices[aBoneIDs[i]]) * aNormal;
                totalNormal += localNormal * aWeights[i];
            }
        }
        
        if (!hasValidBone)
        {
            totalPosition = vec4(aPos, 1.0);
            totalNormal = aNormal;
        }
    }
    else
    {
        totalPosition = vec4(aPos, 1.0);
        totalNormal = aNormal;
    }
    
    FragPos = vec3(model * totalPosition);
    Normal = mat3(transpose(inverse(model))) * totalNormal;
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * model * totalPosition;
}