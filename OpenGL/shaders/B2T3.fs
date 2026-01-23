#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords; // no se usa, pero se mantiene por compatibilidad con tu VS

uniform vec3 lightPos;   // luz del cubo
uniform vec3 viewPos;

// ===== COLOR BASE (SIN TEXTURAS) =====
uniform vec3 baseColor;

// ===== LINTERNAS (spot) =====
uniform bool linterna;
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform float spotCutOff;       // corte interno (cos)
uniform float spotOuterCutOff;  // corte externo (cos)

void main()
{
    // --- AMBIENTE OSCURO ---
    float ambientStrength = 0.45;
    vec3 ambientColor = vec3(2.2, 2.2, 2.2);
    vec3 ambient = ambientStrength * ambientColor;

    // --- DIFUSA (luz principal) ---
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuseColor = vec3(0.45, 0.45, 0.55);
    vec3 diffuse = diff * diffuseColor;

    // --- ESPECULAR (luz principal) ---
    float specularStrength = 0.55;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
    vec3 specularColor = vec3(0.85, 0.85, 0.95);
    vec3 specular = specularStrength * spec * specularColor;

    vec3 result = ambient + diffuse + specular;

    // --- LINTERNAS (spot) ---
    if (linterna) {
        vec3 dirToFrag = normalize(spotLightPos - FragPos);
        float theta = dot(dirToFrag, normalize(-spotLightDir));
        float epsilon = spotCutOff - spotOuterCutOff;
        float intensity = clamp((theta - spotOuterCutOff) / epsilon, 0.0, 1.0);

        // Atenuación simple (opcional, para que no ilumine infinito)
        float dist = length(spotLightPos - FragPos);
        float att = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);

        float diffSpot = max(dot(norm, -dirToFrag), 0.0);
        vec3 diffuseSpot = diffSpot * vec3(0.85, 0.85, 1.00) * intensity * att;

        vec3 reflectSpot = reflect(-dirToFrag, norm);
        float specSpot = pow(max(dot(viewDir, reflectSpot), 0.0), 64.0);
        vec3 specularSpot = vec3(0.95, 0.95, 1.00) * specSpot * intensity * att;

        result += diffuseSpot + specularSpot;
    }

    FragColor = vec4(result * baseColor, 1.0);
}
