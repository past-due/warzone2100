#version 150 core

uniform sampler2D tex1;
uniform sampler2D tex2;

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
	vec4 fragColor = texture(tex1, uv1) * texture(tex2, uv2);
	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(vec4(1.), fragColor, fogFactor);
	}
	FragColor = fragColor;
}
