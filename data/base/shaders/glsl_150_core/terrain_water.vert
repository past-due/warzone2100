#version 150 core

uniform mat4 ModelViewProjectionMatrix;

uniform vec4 paramx1;
uniform vec4 paramy1;
uniform vec4 paramx2;
uniform vec4 paramy2;

uniform mat4 textureMatrix1;
uniform mat4 textureMatrix2;

in vec4 vertex;
in vec4 vertexColor;

out vec4 color;
out vec2 uv1;
out vec2 uv2;
out float vertexDistance;

void main()
{
	color = vertexColor;
	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
	vec4 uv1_tmp = textureMatrix1 * vec4(dot(paramx1, vertex), dot(paramy1, vertex), 0., 1.);
	uv1 = uv1_tmp.xy / uv1_tmp.w;
	vec4 uv2_tmp = textureMatrix2 * vec4(dot(paramx2, vertex), dot(paramy2, vertex), 0., 1.);
	uv2 = uv2_tmp.xy / uv2_tmp.w;
	vertexDistance = position.z;
}
