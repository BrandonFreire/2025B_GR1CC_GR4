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

void main()
{    
    // 1. Obtener el color del pixel desde la textura del Alien
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    
    // Si la textura tiene partes transparentes, no las dibujamos
    if(texColor.a < 0.1)
        discard;

    // --- CÁLCULO DE LUZ BÁSICO (Blinn-Phong simplificado) ---
    
    // Ambiente (Luz base mínima para que no se vea negro total en las sombras)
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
  
    // Difusa (Luz direccional desde tu lightPos)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);

    // Resultado final: (Ambiente + Difusa) * ColorDeLaTextura
    vec3 result = (ambient + diffuse) * texColor.rgb;
    
    FragColor = vec4(result, texColor.a);
}