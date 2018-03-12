#version 150 core
//#pragma debug(on)

uniform sampler2D Texture;
uniform vec4 colour;
uniform bool alphaTest;
uniform float graphicsCycle; // a periodically cycling value for special effects

in vec2 texCoord;

out vec4 FragColor;

void main()
{
	vec4 texColour = texture(Texture, texCoord);

	vec4 fragColour = texColour * colour;

	if (alphaTest && (fragColour.a <= 0.001))
	{
		discard;
	}

	FragColor = fragColour;
}
