$input a_position, a_normal, a_tangent, a_texcoord0
$output v_wpos, v_view, v_normal, v_tangent, v_bitangent, v_texcoord0

/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "../../bgfx/examples/common/common.sh"
void main()
{
	// 获取顶点在世界空间中的坐标表示
	vec3 wpos = mul(u_model[0], vec4(a_position, 1.0) ).xyz;
	v_wpos = wpos;

	// 获取顶点在齐次裁剪空间中的坐标表示
	gl_Position = mul(u_viewProj, vec4(wpos, 1.0) );
	
	// 将法线和切线的取值范围变换到-1到1
	vec4 normal = a_normal * 2.0 - 1.0;
	vec4 tangent = a_tangent * 2.0 - 1.0;
	
	// 将法线和切线变换到世界空间中
	vec3 wnormal = mul(u_model[0], vec4(normal.xyz, 0.0) ).xyz;
	vec3 wtangent = mul(u_model[0], vec4(tangent.xyz, 0.0) ).xyz;

	v_normal = normalize(wnormal);
	v_tangent = normalize(wtangent);
	// 计算副切线
	v_bitangent = cross(v_normal, v_tangent) * tangent.w;

	mat3 tbn = mtxFromCols(v_tangent, v_bitangent, v_normal);

	// eye position in world space
	v_view = mul(u_invView, vec4(0,0,0,1)).xyz - mul(u_model[0], vec4(a_position, 1.0)).xyz;
	a_texcoord0.y = 1.0 - a_texcoord0.y;
	v_texcoord0 = a_texcoord0;
}