#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTex;
uniform float alpha;

void main()
{
    vec4 texColor = texture(screenTex, TexCoords);
    FragColor = vec4(texColor.rgb, alpha);
}
