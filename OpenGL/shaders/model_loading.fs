#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

// La clase Model de LearnOpenGL asigna automáticamente el texture_diffuse1
// a la primera textura que encuentra (tu baseColor.png)
uniform sampler2D texture_diffuse1;

// Variables de luz (las pasaremos desde el main)
uniform vec3 viewPos;
uniform vec3 lightPos; // Usaremos la misma posición de luz de tu laberinto

// ===== LINTERNA (Spotlight) =====
uniform bool linterna;
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform float spotCutOff;
uniform float spotOuterCutOff;

void main()
{    
    // 1. Obtener el color del pixel desde la textura del Alien
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    
    // Si la textura tiene partes transparentes, no las dibujamos
    if(texColor.a < 0.1)
        discard;

    // --- CÁLCULO DE LUZ BÁSICO (Blinn-Phong simplificado) ---
    
    // Ambiente (Luz base mínima para que no se vea negro total en las sombras)
    float ambientStrength = 0.15; // Reducido para que se note más la linterna
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
  
    // Difusa (Luz direccional desde tu lightPos)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(0.3, 0.3, 0.3); // Reducido para ambiente oscuro

    // ===== LINTERNA (Spotlight) =====
    vec3 spotLight = vec3(0.0);
    if (linterna) {
        vec3 spotDir = normalize(spotLightPos - FragPos);
        float theta = dot(spotDir, normalize(-spotLightDir));
        
        if (theta > spotOuterCutOff) {
            // Dentro del cono de luz
            float epsilon = spotCutOff - spotOuterCutOff;
            float intensity = clamp((theta - spotOuterCutOff) / epsilon, 0.0, 1.0);
            
            // Atenuación por distancia
            float distance = length(spotLightPos - FragPos);
            float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
            
            // Difusa de la linterna
            float spotDiff = max(dot(norm, spotDir), 0.0);
            
            // Color de la linterna (blanco brillante)
            vec3 spotColor = vec3(1.0, 0.95, 0.9) * 2.5; // Intensidad alta
            
            spotLight = spotColor * spotDiff * intensity * attenuation;
        }
    }

    // Resultado final: (Ambiente + Difusa + Linterna) * ColorDeLaTextura
    vec3 result = (ambient + diffuse + spotLight) * texColor.rgb;
    
    FragColor = vec4(result, texColor.a);
}