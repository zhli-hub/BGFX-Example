$input v_wpos, v_view, v_normal, v_tangent, v_bitangent, v_texcoord0

/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "../../bgfx/examples/common/common.sh"

SAMPLER2D(s_texColor, 0);
SAMPLER2D(s_texNormal, 1);
SAMPLER2D(s_texAorm, 2);
SAMPLER2D(s_texLUT, 3);
SAMPLERCUBE(s_texLod, 4);
SAMPLERCUBE(s_texIrr, 5);

uniform vec4 u_lightPos;
uniform vec4 u_lightColor;

vec3 FresnelSchlick(float hdotl, vec3 F0) {
	return F0 + (1 - F0) * pow(1 - hdotl, 5);
}

float GeometrySmith2(float ndotv, float ndotl, float roughness) {
	
	float r = (roughness + 1.0);
    float k = (roughness * roughness) / 2.0;
	float ggx1 = (1 - k) * ndotl + k;
	float ggx2 = (1 - k) * ndotv + k;
	return 1.f / (ggx1 * ggx2 + 0.001f);
}

// 几何函数 GGX与Schlick-Beckmann近似的结合体
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float ndotv, float ndotl, float roughness)
{
    float ggx2 = GeometrySchlickGGX(ndotv, roughness);
    float ggx1 = GeometrySchlickGGX(ndotl, roughness);

    return ggx1 * ggx2;
}

float DistributionGGX(float ndoth, float roughness) {
	//float a = roughness * roughness;
	//return (a * a) / (3.14f * pow((a * a - 1.f) * ndoth * ndoth + 1.f, 2));

	float a = roughness*roughness;
    float a2 = a*a;

    float nom   = a2;
    float denom = (ndoth * ndoth * (a2 - 1.0f) + 1.0f);
    denom = 3.14159265359f * denom * denom;

    return nom / max(denom, 0.0000001);
}
vec3 calcFresnel(vec3 _cspec, float _dot, float _strength)
{
	return _cspec + (1.0 - _cspec)*pow(1.0 - _dot, 5.0) * _strength;
}

void main()
{	
	//vec3 inAlbedo = vec3_splat(1);
	// texture sample
	vec3 normal, aorm, albedo;
	//albedo = toLinear(texture2D(s_texColor, v_texcoord0));
	albedo = vec3_splat(1);
	aorm = texture2D(s_texAorm, v_texcoord0);
	normal.xy = texture2D(s_texNormal, v_texcoord0).xy * 2.0 - 1;
	normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
	normal = v_normal;
	aorm[1] = 0.1;
	aorm[2] = 0.85;
	// light direction
	vec3 ld = normalize(u_lightPos - v_wpos);
	vec3 clight = u_lightColor;

	vec3 nd = v_normal;
	vec3 vd = normalize(v_view);
	vec3 hd = normalize(ld + vd);
	float ndotv = max(dot(nd, vd), 0.0);
	float ndotl = clamp(dot(nd, ld), 0.0, 1.0);
	float ndoth = clamp(dot(nd, hd), 0.0, 1.0);
	float hdotv = clamp(dot(hd, vd), 0.0, 1.0);
	float hdotl = clamp(dot(hd, ld), 0.0, 1.0);

	vec3 ambient = vec3_splat(0.03) * albedo;

	// direct light diffuse
	float FD90 = 0.5f + 2 * aorm[1] * hdotl * hdotl;
	float dirFDiffuse = albedo / 3.14f * (1 + (FD90 - 1) * pow(1 - ndotl, 5)) * (1 + (FD90 - 1) * pow(1 - ndotv, 5));

	//direct light specular
	vec3 F0 = vec3_splat(0.01);
	F0 = mix(F0, albedo, aorm[2]);
	float N = DistributionGGX(ndoth, aorm[1]);
	float D = GeometrySmith(ndotv, ndotl, aorm[1]);
	vec3 F = FresnelSchlick(ndotv, F0);
	//vec3 dirFresnel = calcFresnel(F0, hdotv, 1 - aorm[1]);
	//F = dirFresnel;
	vec3 dirFSpecular =  (F * N * D) / max(4 * ndotv * ndotl, 0.001);
	vec3 ks = F;
	vec3 kd = 1.0 - ks;
	// for metallic flow
	kd = kd * (1 - aorm[2]);

	vec3 dirColor = (kd * dirFDiffuse + dirFSpecular) * ndotl * clight;
	//dirColor = dirColor / (dirColor + vec3(1.f, 1.f, 1.f));

	// environment light diffuse
	float mip = 1.0 + 5.0 * aorm[1];
	vec3 rd = 2.0 * ndotv * nd - vd; // reflect direction
	vec3 cubeR = normalize(rd);
	vec3 cubeN = nd;
	cubeR = fixCubeLookup(cubeR, mip, 256.0);

	vec3 radiance = toLinear(textureCubeLod(s_texLod, cubeR, mip).xyz);
	vec3 irradiance  = toLinear(textureCube(s_texIrr, cubeN).xyz);
	vec3 envDiffuse  = kd * albedo * irradiance;
	vec2 envBRDF  = texture2D(s_texLUT, vec2(ndotv, aorm[1])).xy;
	vec3 envSpecular = radiance * (ks * envBRDF.x + envBRDF.y);
	envSpecular = radiance * ks;
	vec3 indirectColor = 1 * envDiffuse + envSpecular;

	//gl_FragColor.xyz = radiance;
	gl_FragColor.xyz = 1 * dirColor + 1 * indirectColor;
	gl_FragColor.w = 1.0;
	gl_FragColor = toGamma(gl_FragColor);
}