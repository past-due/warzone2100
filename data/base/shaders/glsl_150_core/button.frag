#version 150 core
//#pragma debug(on)

uniform sampler2D Texture;
uniform sampler2D TextureTcmask;
uniform vec4 colour;
uniform vec4 teamcolour;
uniform int tcmask;
uniform bool alphaTest;

in vec2 texCoord;

out vec4 FragColor;

void main()
{
	// Get color from texture unit 0
	vec4 texColour = texture(Texture, texCoord);

	vec4 fragColour;
	if (tcmask == 1)
	{
		// Get tcmask information from texture unit 1
		vec4 mask = texture(TextureTcmask, texCoord);

		// Apply colour using grain merge with tcmask
		fragColour = (texColour + (teamcolour - 0.5) * mask.a) * colour;
	}
	else
	{
		fragColour = texColour * colour;
	}

	if (alphaTest && fragColour.a <= 0.001)
	{
		discard;
	}

	FragColor = fragColour;
}
