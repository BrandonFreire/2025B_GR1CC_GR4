#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform sampler2D texture1;
uniform vec3 baseColor;
uniform bool useTexture;

uniform bool linterna;
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform float spotCutOff;
uniform float spotOuterCutOff;

// scale applied to texture coordinates per-axis (legacy)
uniform vec2 texScale2; // (s,t)
uniform bool flipTexY; // flip vertical sampling

// use world-space UVs (use FragPos) instead of per-quad TexCoords
uniform bool useWorldUV;
uniform vec2 worldTexScale; // scaling applied to world uv (horiz, vert)
uniform vec2 worldTexOffset; // offset for world uv (horiz, vert)

void main()
{
    // --- COMPUTE UV ---
    vec2 uv;
    if(useWorldUV) {
        // choose horizontal axis depending on face orientation
        float horiz = (abs(Normal.z) > abs(Normal.x)) ? FragPos.x : FragPos.z;
        uv = vec2(horiz - worldTexOffset.x, FragPos.y - worldTexOffset.y) * worldTexScale;
        if(flipTexY) uv.y =1.0 - uv.y;
    } else {
        uv = TexCoords * texScale2;
        if(flipTexY) uv.y =1.0 - uv.y;
    }

    // --- COLOR BASE O TEXTURA ---
    vec3 color = useTexture ? texture(texture1, uv).rgb * baseColor : baseColor;

    // --- LUZ AMBIENTAL ---
    float ambientStrength =0.25;
    vec3 ambientColor = vec3(0.15,0.15,0.2);
    vec3 ambient = ambientStrength * ambientColor;

    // --- LUZ PUNTUAL DIFUSA ---
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir),0.0);
    vec3 diffuse = diff * vec3(0.4,0.4,0.5);

    // --- ESPECULAR ---
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir),0.0),64.0);
    vec3 specular =0.6 * spec * vec3(0.8,0.8,0.9);

    vec3 result = ambient + diffuse + specular;

    // --- LINTERNAS (spot) ---
    if(linterna)
    {
        // direction from light to fragment
        vec3 lightToFrag = normalize(FragPos - spotLightPos);
        // spotLightDir should point where the cone faces (e.g., camera.Front)
        float theta = dot(normalize(spotLightDir), lightToFrag);

        float epsilon = spotCutOff - spotOuterCutOff;
        float intensity = clamp((theta - spotOuterCutOff) / epsilon, 0.0, 1.0);

        float dist = length(FragPos - spotLightPos);
        // Atenuación más suave para mayor alcance
        float attenuation = 1.0 / (1.0 + 0.022 * dist + 0.0019 * dist * dist);

        // diffuse from spot: use direction from fragment to light for normal dot
        vec3 fragToLight = normalize(spotLightPos - FragPos);

        // Usar el valor absoluto del dot product para iluminar ambos lados de la superficie
        // Esto evita que las paredes se vean negras cuando la normal apunta en dirección opuesta
        float diffSpot = abs(dot(norm, fragToLight));
        // Añadir un mínimo de iluminación para evitar zonas completamente negras
        diffSpot = max(diffSpot, 0.15);

        // Aumentada la intensidad de la luz difusa
        vec3 diffuseSpot = diffSpot * vec3(1.0, 0.95, 0.85) * intensity * attenuation * 3.5;

        // specular for spot
        vec3 reflectSpot = reflect(-fragToLight, norm);
        float specSpot = pow(max(dot(viewDir, reflectSpot), 0.0), 32.0);
        // Aumentada la intensidad especular
        vec3 specularSpot = vec3(1.0, 0.98, 0.9) * specSpot * intensity * attenuation * 2.0;

        result += diffuseSpot + specularSpot;
    }

    // --- SALIDA FINAL ---
    FragColor = vec4(result * color,1.0);
}