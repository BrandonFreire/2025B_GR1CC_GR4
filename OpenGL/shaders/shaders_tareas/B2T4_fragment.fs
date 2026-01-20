#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;    // Usamos la textura como color difuso
    sampler2D specular;   // (Opcional) Mapa de brillo
    float shininess;
}; 

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    // Obtener el color base de la textura
    vec3 texColor = texture(material.diffuse, TexCoords).rgb;

    // 1. Ambient
    vec3 ambient = light.ambient * texColor;
  	
    // 2. Diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * texColor;  
    
    // 3. Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * vec3(0.5); // Brillo blanco est�ndar

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}