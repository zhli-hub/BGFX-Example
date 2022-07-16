$input v_view, v_normal, v_shadowcoord, v_wpos

#include "../../bgfx/examples/common/common.sh"

uniform vec4 u_lightPos;
uniform vec4 u_lightColor;
uniform vec4 u_shadowType;

SAMPLER2D(s_texShadowMap, 6);
#define Sampler sampler2D

vec3 BlinnPhong(vec3 _lightDir, vec3 _normal, vec3 _viewDir)
{
	vec3 specularColor = vec3_splat(0.1);
	vec3 reflectDir = _lightDir - 2.0 * dot(_normal, _lightDir) * _normal;
	vec3 halfDir = normalize(_viewDir + _lightDir);
	
	vec3 diffuse = u_lightColor.xyz * vec3(1,1,1) * saturate(dot(_normal, _lightDir));
	vec3 specular = u_lightColor.xyz * specularColor.xyz * pow(saturate(mul(halfDir, _normal)), vec3_splat(20.f) );
	vec3 color = specular + diffuse;
	return color;
}

float hardShadow(Sampler _sampler, vec4 _shadowCoord, float _bias)
{
	vec3 texCoord = _shadowCoord.xyz;
	return step(texCoord.z-_bias, unpackRgbaToFloat(texture2D(_sampler, texCoord.xy) ) );

}
float Lerp(float x, float y, float s)
{
	return (1 - s) * x + s * y;
}
float PCF1x1(Sampler _sampler, vec4 _shadowCoord, float _bias, float _texelSize)
{
	vec2 texCoord = _shadowCoord.xy;

	bool outside = any(greaterThan(texCoord, vec2_splat(1.0)))
				|| any(lessThan   (texCoord, vec2_splat(0.0)));

	if (outside)
	{
		return 1.0;
	}
	float s0 = hardShadow(_sampler, _shadowCoord + vec4(0.0, 0.0, 0.0, 0.0), _bias);
	float s1 = hardShadow(_sampler, _shadowCoord + vec4(_texelSize, 0.0, 0.0, 0.0), _bias);
	float s2 = hardShadow(_sampler, _shadowCoord + vec4(0.0, _texelSize, 0.0, 0.0), _bias);
	float s3 = hardShadow(_sampler, _shadowCoord + vec4(_texelSize, _texelSize, 0.0, 0.0), _bias);
	
	vec2 texelPos = 1024 * _shadowCoord.xy;
	vec2 t = fract(texelPos);
	return Lerp(Lerp(s0, s1, t.x), Lerp(s2, s3, t.x), t.y);
}
float SampleShadowPCF3x3_4Tap_Fast(Sampler _sampler, vec4 _shadowCoord, float _bias, float _texelSize) 
{
	float res = 0;
	vec2 offsets[9] = 
	{
		vec2(-_texelSize, -_texelSize), vec2(0.0f, -_texelSize), vec2(_texelSize, -_texelSize),
		vec2(-_texelSize, 0.0f),        vec2(0.0f, 0.0f),        vec2(_texelSize, 0.0f),
		vec2(-_texelSize, _texelSize),  vec2(0.0f, _texelSize),  vec2(_texelSize, _texelSize)
	};
	for (int i = 0; i < 9; ++i)
	{
		res += PCF1x1(_sampler, vec4(_shadowCoord.xy + offsets[i], _shadowCoord.zw), _bias, _texelSize);
	}
	return res / 9.0f;
}

void main()
{
	float shadowMapBias = 0.005;
	vec3 albedo = vec3_splat(1.0);
	vec3 ambinet = vec3_splat(0.3);
	vec3 lightDir = normalize(u_lightPos.xyz - v_wpos);
	vec3 color =  BlinnPhong(lightDir, v_normal, v_view);

	float texelSize =  1.0 / 1024.0;
	float visibility = 0;
	if (u_shadowType[0] == 1) {
		visibility = SampleShadowPCF3x3_4Tap_Fast(s_texShadowMap, v_shadowcoord, shadowMapBias, texelSize);
	} else {
		visibility = hardShadow(s_texShadowMap, v_shadowcoord, shadowMapBias);
	}
	
	vec3 brdf = color * visibility;

	vec3 final = toGamma(abs(brdf + ambinet * albedo.xyz) );
	float ndotv = max(dot(v_normal, v_view), 0.0);
	gl_FragColor = vec4(final, 1.0);
}
