$input v_texcoord0

#include "../../bgfx/examples/common/common.sh"

uniform vec4 u_params[4];
#define u_mtx0          u_params[0]
#define u_mtx1          u_params[1]
#define u_mtx2          u_params[2]
#define u_mtx3          u_params[3]

SAMPLERCUBE(s_texLod, 3);

void main()
{
	float fov = radians(45.0);
	float height = tan(fov*0.5);
	float aspect = height*(u_viewRect.z / u_viewRect.w);

	vec3 dir = vec3((v_texcoord0*2.0 - 1.0)*vec2(aspect, height), 1.0);
	mat4 mtx;
	mtx[0] = u_mtx0;
	mtx[1] = u_mtx1;
	mtx[2] = u_mtx2;
	mtx[3] = u_mtx3;
	dir = mul(mtx, vec4(dir, 0.0) ).xyz;
	dir = normalize(dir);
	gl_FragColor.xyz = toFilmic(toLinear(textureCube(s_texLod, dir))).xyz;
	//gl_FragColor.xyz = vec3_splat(1);
	gl_FragColor.w = 1;
}