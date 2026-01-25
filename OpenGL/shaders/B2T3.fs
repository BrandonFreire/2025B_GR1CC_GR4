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

// ===== LUCES DE ANTORCHA (cubos coleccionables) =====
#define MAX_TORCH_LIGHTS 10
uniform int numTorchLights;
uniform vec3 torchPositions[MAX_TORCH_LIGHTS];
uniform vec3 torchColor;
uniform float torchIntensity;

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
        float intensity = clamp((theta - spotOuterCutOff) / epsilon,0.0,1.0);

        float dist = length(FragPos - spotLightPos);
        // Atenuación más suave para mayor alcance
        // increased constants to reduce effective range (darker further away)
        float attenuation =1.0 / (1.0 +0.044 * dist +0.0038 * dist * dist);

        // diffuse from spot: use direction from fragment to light for normal dot
        vec3 fragToLight = normalize(spotLightPos - FragPos);

        // Usar el valor absoluto del dot product para iluminar ambos lados de la superficie
        // Esto evita que las paredes se vean negras cuando la normal apunta en dirección opuesta
        float diffSpot = abs(dot(norm, fragToLight));
        // Añadir un mínimo de iluminación para evitar zonas completamente negras (reduced)
        diffSpot = max(diffSpot,0.08);

        // Reduced intensity multiplier for the spot diffuse
        vec3 diffuseSpot = diffSpot * vec3(1.0,0.95,0.85) * intensity * attenuation *1.4;

        // specular for spot (reduced)
        vec3 reflectSpot = reflect(-fragToLight, norm);
        float specSpot = pow(max(dot(viewDir, reflectSpot), 0.0),32.0);
        vec3 specularSpot = vec3(1.0,0.98,0.9) * specSpot * intensity * attenuation *0.8;

        result += diffuseSpot + specularSpot;
    }

    // ===== LUCES DE ANTORCHA (cubos coleccionables) =====
    for(int i = 0; i < numTorchLights && i < MAX_TORCH_LIGHTS; i++)
    {
        vec3 torchPos = torchPositions[i];
        vec3 fragToTorch = normalize(torchPos - FragPos);
        float dist = length(torchPos - FragPos);

        // Atenuación cuadrática para luz de antorcha (alcance ~8-10 unidades)
        float attenuation = 1.0 / (1.0 + 0.14 * dist + 0.07 * dist * dist);

        // Difusa bilateral (ilumina ambos lados)
        float diffTorch = abs(dot(norm, fragToTorch));
        diffTorch = max(diffTorch, 0.1);

        // Color de fuego con intensidad variable
        vec3 torchDiffuse = diffTorch * torchColor * attenuation * torchIntensity;

        // Especular suave para el brillo
        vec3 reflectTorch = reflect(-fragToTorch, norm);
        float specTorch = pow(max(dot(viewDir, reflectTorch), 0.0), 16.0);
        vec3 torchSpecular = torchColor * specTorch * attenuation * torchIntensity * 0.5;

        result += torchDiffuse + torchSpecular;
    }

    // --- SALIDA FINAL ---
    FragColor = vec4(result * color,1.0);
}