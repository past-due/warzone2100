#version 150 core

uniform mat4 ModelViewProjectionMatrix;

uniform vec4 paramxlight;
uniform vec4 paramylight;
uniform mat4 lightTextureMatrix;

in vec4 vertex;
in vec2 vertexTexCoord;

out vec2 uv_tex;
out vec2 uv_lightmap;
out float vertexDistance;

void main()
{
	vec4 position = ModelViewProjectionMatrix * vertex;
	gl_Position = position;
	uv_tex = vertexTexCoord;
	vec4 uv_lightmap_tmp = lightTextureMatrix * vec4(dot(paramxlight, vertex), dot(paramylight, vertex), 0.0, 1.0);
	uv_lightmap = uv_lightmap_tmp.xy / uv_lightmap_tmp.w;
	vertexDistance = position.z;
}
