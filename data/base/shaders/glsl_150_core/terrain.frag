#version 150 core

uniform sampler2D tex;
uniform sampler2D lightmap_tex;

uniform int fogEnabled; // whether fog is enabled
uniform float fogEnd;
uniform float fogStart;
uniform vec4 fogColor;

in vec4 color;
in vec2 uv1;
in vec2 uv2;
in float vertexDistance;

out vec4 FragColor;

void main()
{
	vec4 fragColor = color * texture(tex, uv1) * texture(lightmap_tex, uv2);
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fogColor, fragColor, fogFactor);
	}
	FragColor = fragColor;
}
