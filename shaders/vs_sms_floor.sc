$input a_position, a_normal
$output v_view, v_normal, v_shadowcoord, v_wpos

#include "../../bgfx/examples/common/common.sh"

uniform mat4 u_lightMtx;
uniform vec4 u_lightPos;
void main()
{
	// 获取顶点在世界空间中的坐标表示
	vec3 wpos = mul(u_model[0], vec4(a_position, 1.0) ).xyz;
	v_wpos = wpos;

	// 获取顶点在齐次裁剪空间中的坐标表示
	gl_Position = mul(u_viewProj, vec4(wpos, 1.0) );
	
	vec4 normal = a_normal * 2.0 - 1.0;

	
	// 将法线和切线变换到世界空间中
	vec3 wnormal = mul(u_model[0], vec4(normal.xyz, 0.0) ).xyz;

	v_normal = normalize(wnormal);


	// eye position in world space
	vec3 weyepos = mul(vec4(0.0, 0.0, 0.0, 1.0), u_view).xyz;
	
	v_view = normalize(mul(u_invView, vec4(0,0,0,1)).xyz - mul(u_model[0], vec4(a_position, 1.0)).xyz);

	const float shadowMapOffset = 0.001;
	vec3 posOffset = a_position + shadowMapOffset * normal.xyz;
	// the shadowcoord is in the range of 0 to 1 after mul by u_lightMtx
	v_shadowcoord = mul(u_lightMtx, vec4(posOffset, 1.0) );
	
}
