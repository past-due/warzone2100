#version 150 core

uniform sampler2D lightmap_tex;
out vec2 uv2;
out vec4 FragColor;

void main()
{
	FragColor = texture(lightmap_tex, uv2);
}
