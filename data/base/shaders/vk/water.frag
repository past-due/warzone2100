#version 450

layout(set = 1, binding = 0) uniform sampler2D tex1;
layout(set = 1, binding = 1) uniform sampler2D tex2;
layout(set = 1, binding = 2) uniform sampler2D tex1_nm;
layout(set = 1, binding = 3) uniform sampler2D tex2_nm;
layout(set = 1, binding = 4) uniform sampler2D tex1_sm;
layout(set = 1, binding = 5) uniform sampler2D tex2_sm;
layout(set = 1, binding = 6) uniform sampler2D tex1_hm;
layout(set = 1, binding = 7) uniform sampler2D tex2_hm;

layout(std140, set = 0, binding = 0) uniform cbuffer {
	mat4 ModelViewProjectionMatrix;
	mat4 ModelUV1Matrix;
	mat4 ModelUV2Matrix;
	vec4 cameraPos; // in modelSpace
	vec4 sunPos; // in modelSpace, normalized
	vec4 emissiveLight; // light colors/intensity
	vec4 ambientLight;
	vec4 diffuseLight;
	vec4 specularLight;
	vec4 fogColor;
	int fogEnabled; // whether fog is enabled
	float fogEnd;
	float fogStart;
	float timeSec;
	int quality;
};

layout(location = 1) in vec4 uv1_uv2;
layout(location = 2) in float vertexDistance;
layout(location = 3) in vec3 lightDir;
layout(location = 4) in vec3 halfVec;
layout(location = 5) in float depth;
layout(location = 6) in float depth2;

layout(location = 0) out vec4 FragColor;

vec4 main_medium()
{
	vec2 uv1 = uv1_uv2.xy;
	vec2 uv2 = uv1_uv2.zw;
	vec4 fragColor = texture(tex1, uv1);
	float specColor = texture(tex2, uv2).r;
	fragColor *= vec4(specColor, specColor, specColor, 1.0);
	return fragColor;
}

vec4 main_bumpMapping()
{
	vec2 uv1 = uv1_uv2.xy;
	vec2 uv2 = uv1_uv2.zw;

	vec3 N1 = texture(tex1_nm, uv2).xzy; // y is up in modelSpace
	vec3 N2 = texture(tex2_nm, uv1).xzy;
	vec3 N; //use overlay blending to mix normal maps properly
	N.x = N1.x < 0.5 ? (2 * N1.x * N2.x) : (1 - 2 * (1 - N1.x) * (1 - N2.x));
	N.z = N1.z < 0.5 ? (2 * N1.z * N2.z) : (1 - 2 * (1 - N1.z) * (1 - N2.z));
	N.y = N1.y < 0.5 ? (2 * N1.y * N2.y) : (1 - 2 * (1 - N1.y) * (1 - N2.y));
	if (N == vec3(0,0,0)) {
		N = vec3(0,1,0);
	} else {
		N = normalize(N * 2.0 - 1.0);
	}
	float lambertTerm = max(dot(N, lightDir), 0.0); // diffuse lighting

	// Gaussian specular term computation
	float gloss = texture(tex1_sm, uv1).r * texture(tex2_sm, uv2).r;
	vec3 H = normalize(halfVec);
	float exponent = acos(dot(H, N)) / (gloss + 0.05);
	float gaussianTerm = exp(-(exponent * exponent));

	vec4 fragColor = (texture(tex1, uv1)+texture(tex2, uv2)) * (gloss+vec4(0.08,0.13,0.15,1.0));
	return fragColor*(ambientLight+diffuseLight*lambertTerm) + specularLight*(1-gloss)*gaussianTerm*vec4(1.0,0.843,0.686,1.0);
}

void main()
{
	vec4 fragColor;
	if (quality == 2) {
		fragColor = main_bumpMapping();
		fragColor = mix(fragColor, fragColor-depth*0.0007, depth*0.0009);
		fragColor.a = mix(0.25, 1.0, depth2*0.005);
	} else {
		fragColor = main_medium();
	}

	if (fogEnabled > 0)
	{
		// Calculate linear fog
		float fogFactor = (fogEnd - vertexDistance) / (fogEnd - fogStart);
		fogFactor = clamp(fogFactor, 0.0, 1.0);

		// Return fragment color
		fragColor = mix(fragColor, fogColor, fogFactor);
	}

	FragColor = fragColor;
}
