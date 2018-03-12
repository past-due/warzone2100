#version 150 core

uniform vec4 color;
uniform sampler2D theTexture;

in vec2 uv;

out vec4 FragColor;

void main()
{
	FragColor = texture(theTexture, uv) * color;
}
