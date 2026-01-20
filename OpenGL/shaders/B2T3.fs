/* 
#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform sampler2D texture1;

void main() 
{
    // Ambiente
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * vec3(1.0);

    // Difusa
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);

    // Especular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1.0);

    vec3 lighting = ambient + diffuse + specular;
    vec3 texColor = texture(texture1, TexCoords).rgb;

    FragColor = vec4(lighting * texColor, 1.0);
} 
*/
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightPos;      // luz del cubo
uniform vec3 viewPos;
uniform sampler2D texture1;

// ===== LINTERNAS =====
uniform bool linterna;
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform float spotCutOff;      // corte interno
uniform float spotOuterCutOff; // corte externo para suavizado

void main()
{
    // --- TEXTURA ---
    vec3 texColor = texture(texture1, TexCoords).rgb;

    // --- AMBIENTE OSCURO ---
    float ambientStrength = 0.1;
    vec3 ambientColor = vec3(0.05, 0.05, 0.1);
    vec3 ambient = ambientStrength * ambientColor;

    // --- DIFUSA CUBO ---
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuseColor = vec3(0.4, 0.4, 0.5); // más intensa que antes
    vec3 diffuse = diff * diffuseColor;

    // --- ESPECULAR CUBO ---
    float specularStrength = 0.6;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    vec3 specularColor = vec3(0.8, 0.8, 0.9);
    vec3 specular = specularStrength * spec * specularColor;

    vec3 result = ambient + diffuse + specular;

    // --- LINTERNAS (spot) ---
    if(linterna) {
        vec3 lightDirSpot = normalize(spotLightPos - FragPos);
        float theta = dot(lightDirSpot, normalize(-spotLightDir));
        float epsilon = spotCutOff - spotOuterCutOff;
        float intensity = clamp((theta - spotOuterCutOff) / epsilon, 0.0, 1.0);

        // Difusa y especular spot
        float diffSpot = max(dot(norm, -lightDirSpot), 0.0);
        vec3 diffuseSpot = diffSpot * vec3(0.8,0.8,1.0) * intensity;

        float specSpot = pow(max(dot(viewDir, reflect(lightDirSpot, norm)), 0.0), 64);
        vec3 specularSpot = vec3(0.9,0.9,1.0) * specSpot * intensity;

        result += diffuseSpot + specularSpot;
    }

    FragColor = vec4(result * texColor, 1.0);
}
