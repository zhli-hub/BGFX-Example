/*
 * Copyright 2011-2022 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include <bx/uint32_t.h>
#include "common.h"
#include "bgfx_utils.h"
#include "bgfx_logo.h"
#include "imgui/imgui.h"
#include "entry/input.h"
#include <iostream>
#include <vector>

using namespace std;
namespace
{
#define RENDER_SKYBOX_PASS_ID 0
#define RENDER_SHADOW_PASS_ID 1
#define RENDER_SCENE_PASS_ID  2


class ShaderParams
{
public:
	enum { NumVec4 = 4 };
	void init() {
		u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, NumVec4);
		bx::mtxIdentity(view_mtx);
		//bx::mtxIdentity(light_mtx);
		//bx::memSet(light_pos, 0, 4 * sizeof(float));
	}
	void submit() {
		bgfx::setUniform(u_params, m_params, NumVec4);
	}
	void destroy()
	{
		bgfx::destroy(u_params);
	}
	union {
		struct {
			union {
				struct { float mtx1[4]; };
				struct { float mtx2[4]; };
				struct { float mtx3[4]; };
				struct { float mtx4[4]; };
				float view_mtx[16];
			};
		};
		float m_params[4 * NumVec4];
	};
private:
	
	bgfx::UniformHandle u_params;
};

struct PosNormalVertex
{
	float    m_x;
	float    m_y;
	float    m_z;
	uint32_t m_normal;

	static void init()
	{
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 4, bgfx::AttribType::Uint8, true, true)
			.end();
	};

	static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosNormalVertex::ms_layout;

static PosNormalVertex s_hplaneVertices[] =
{
	{ -1.0f, 0.0f,  1.0f, encodeNormalRgba8(0.0f, 1.0f, 0.0f) },
	{  1.0f, 0.0f,  1.0f, encodeNormalRgba8(0.0f, 1.0f, 0.0f) },
	{ -1.0f, 0.0f, -1.0f, encodeNormalRgba8(0.0f, 1.0f, 0.0f) },
	{  1.0f, 0.0f, -1.0f, encodeNormalRgba8(0.0f, 1.0f, 0.0f) },
};

static const uint16_t s_planeIndices[] =
{
	0, 1, 2,
	1, 3, 2,
};

struct PosColorTexCoord0Vertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_rgba;
	float m_u;
	float m_v;

	static void init()
	{
		ms_layout
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	}

	static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosColorTexCoord0Vertex::ms_layout;

void screenSpaceQuad(float _textureWidth, float _textureHeight, bool _originBottomLeft = false, float _width = 1.0f, float _height = 1.0f)
{
	if (3 == bgfx::getAvailTransientVertexBuffer(3, PosColorTexCoord0Vertex::ms_layout))
	{
		bgfx::TransientVertexBuffer vb;
		bgfx::allocTransientVertexBuffer(&vb, 3, PosColorTexCoord0Vertex::ms_layout);
		PosColorTexCoord0Vertex* vertex = (PosColorTexCoord0Vertex*)vb.data;

		const float zz = 1.0f;

		const float minx = -_width;
		const float maxx = _width;
		const float miny = 0.0f;
		const float maxy = _height * 2.0f;

		const float minu = -1.0f + 0.0f;
		const float maxu = 1.0f + 0.0f;

		float minv = 0.0f;
		float maxv = 2.0f + 0.0f;

		if (_originBottomLeft)
		{
			std::swap(minv, maxv);
			minv -= 1.0f;
			maxv -= 1.0f;
		}

		vertex[0].m_x = maxx;
		vertex[0].m_y = miny;
		vertex[0].m_z = zz;
		vertex[0].m_rgba = 0x00000000;
		vertex[0].m_u = maxu;
		vertex[0].m_v = minv;

		vertex[1].m_x = minx;
		vertex[1].m_y = miny;
		vertex[1].m_z = zz;
		vertex[1].m_rgba = 0x00000000;
		vertex[1].m_u = minu;
		vertex[1].m_v = minv;

		vertex[2].m_x = maxx;
		vertex[2].m_y = maxy;
		vertex[2].m_z = zz;
		vertex[2].m_rgba = 0x00000000;
		vertex[2].m_u = maxu;
		vertex[2].m_v = maxv;

		bgfx::setVertexBuffer(0, &vb);
	}
}

struct Background
{
	enum Enum {
		Bolonga = 0,
		Kyoto = 1,
	};
	void Init() {
		// load cube texture
		m_textureLod[Bolonga] = loadTexture("textures/bolonga_lod.dds", BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP);
		// load irr texture
		m_textureIrr[Bolonga] = loadTexture("textures/bolonga_irr.dds", BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP);

		// load cube texture
		m_textureLod[Kyoto] = loadTexture("textures/kyoto_lod.dds", BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP);
		// load irr texture
		m_textureIrr[Kyoto] = loadTexture("textures/kyoto_irr.dds", BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP);
	}
	bgfx::TextureHandle m_textureLod[2];
	bgfx::TextureHandle m_textureIrr[2];
	
};
struct Setting 
{
	Setting() {
		m_pbrUseShadowSampler = true;
		m_blinnUseShadowSampler = true;
		m_doDiffuse = true;
		m_doSpecular = true;
		m_doDiffuseIBL = true;
		m_doSpecularIBL = true;
		m_useAORMTexture = false;
		m_bunnyUseAlbedo = false;
		m_blinnUseNormalMap = true;
		m_blinnUseAlbedoMap = true;
		m_shadowType = 0;
		m_lightColor[0] = 1.0f;
		m_lightColor[1] = 1.0f;
		m_lightColor[2] = 1.0f;
		m_glossiness = 0.2;
		m_metalness = 0.85;
		m_backward = Background::Enum::Bolonga;
	}
 	bool m_pbrUseShadowSampler;
	bool m_blinnUseShadowSampler;

	bool m_doDiffuse;
	bool m_doSpecular;
	bool m_doDiffuseIBL;
	bool m_doSpecularIBL;
	bool m_useAORMTexture;
	bool m_bunnyUseAlbedo;
	bool m_blinnUseNormalMap;
	bool m_blinnUseAlbedoMap;
	int m_shadowType;
	float m_lightColor[3];
	float m_glossiness;
	float m_metalness;
	Background::Enum m_backward;
};

class EStarHomework : public entry::AppI
{
public:
	EStarHomework(const char* _name, const char* _description, const char* _url)
		: entry::AppI(_name, _description, _url)
	{
		m_width = 0;
		m_height = 0;
		m_debug = BGFX_DEBUG_NONE;
		m_reset = BGFX_RESET_NONE;
		entry::setCurrentDir("../runtime/");
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);

		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_NONE;
		m_reset  = BGFX_RESET_VSYNC;

		bgfx::Init init;
		init.type     = bgfx::RendererType::Direct3D11;
		init.vendorId = args.m_pciId;
		init.resolution.width  = m_width;
		init.resolution.height = m_height;
		init.resolution.reset  = m_reset;
		bgfx::init(init);

		PosColorTexCoord0Vertex::init();
		shaderParams.init();
		m_background.Init();
		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(RENDER_SKYBOX_PASS_ID
			, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
			, 0x303030ff
			, 1.0f
			, 0
			);

		u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);

		m_programShadow = loadProgram("vs_sms_shadow", "fs_sms_shadow");
		m_programMeshPBR = loadProgram("vs_sms_mesh", "fs_sms_mesh");
		m_programMeshBlinn = loadProgram("vs_blinn", "fs_blinn");
		m_programFloor = loadProgram("vs_sms_floor", "fs_sms_floor");
		m_programLod = loadProgram("vs_ibl_skybox", "fs_ibl_skybox");
		
		// load model mesh
		m_meshPBR = meshLoad("meshes/bunny.bin");
		m_meshBlinn = meshLoad("meshes/stone.bin");
		m_meshPlatform = meshLoad("meshes/platform.bin");
		PosNormalVertex::init();
		m_vbh = bgfx::createVertexBuffer(
			bgfx::makeRef(s_hplaneVertices, sizeof(s_hplaneVertices))
			, PosNormalVertex::ms_layout
		);

		m_ibh = bgfx::createIndexBuffer(
			bgfx::makeRef(s_planeIndices, sizeof(s_planeIndices))
		);

		// Render targets.
		m_shadowMapSize = 1024;
		m_shadowMapFB = BGFX_INVALID_HANDLE;

		// Create texture sampler uniforms
		s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
		s_texNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
		s_texAorm = bgfx::createUniform("s_texAorm", bgfx::UniformType::Sampler);
		s_texLod = bgfx::createUniform("s_texLod", bgfx::UniformType::Sampler);
		s_texIrr = bgfx::createUniform("s_texIrr", bgfx::UniformType::Sampler);
		s_texLUT = bgfx::createUniform("s_texLUT", bgfx::UniformType::Sampler);
		s_texShadowMap = bgfx::createUniform("s_texShadowMap", bgfx::UniformType::Sampler);

		// Create lights and the colors of them
		m_numLights = 1;
		u_lightPos = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4, m_numLights);
		u_lightColor = bgfx::createUniform("u_lightColor", bgfx::UniformType::Vec4, m_numLights);
		u_camPos = bgfx::createUniform("u_camPos", bgfx::UniformType::Vec4, 4);
		u_lightMtx = bgfx::createUniform("u_lightMtx", bgfx::UniformType::Mat4);
		u_lightSetting = bgfx::createUniform("u_lightSetting", bgfx::UniformType::Vec4, 1);
		u_pbrMaterial = bgfx::createUniform("u_pbrMaterial", bgfx::UniformType::Vec4, 1);
		u_blinnMaterial = bgfx::createUniform("u_blinnMaterial", bgfx::UniformType::Vec4, 1);
		u_shadowType = bgfx::createUniform("u_shadowType", bgfx::UniformType::Vec4, 1);
		// load diffuse texture.
		m_textureColor = loadTexture("textures/pbr_stone_base_color.dds");
		m_textureBunny = loadTexture("textures/bunny_base_color.dds");
		// load normal texture.
		m_textureNormal = loadTexture("textures/pbr_stone_normal.dds");
		// load ambient occlusion, roughness and metallic texture
		m_textureAorm = loadTexture("textures/pbr_stone_aorm.dds");
		m_textureLUT = loadTexture("textures/brdf_lut.dds");

		m_timeOffset = bx::getHPCounter();

		imguiCreate();
	}

	virtual int shutdown() override
	{
		imguiDestroy();

		meshUnload(m_meshPBR);

		// Cleanup.
		bgfx::destroy(m_programMeshPBR);
		bgfx::destroy(m_programLod);
		bgfx::destroy(m_textureColor);
		bgfx::destroy(m_textureNormal);
		bgfx::destroy(m_textureAorm);
		bgfx::destroy(m_textureLUT);
		bgfx::destroy(s_texColor);
		bgfx::destroy(s_texNormal);
		bgfx::destroy(s_texAorm);
		bgfx::destroy(s_texLod);
		bgfx::destroy(s_texIrr);
		bgfx::destroy(s_texLUT);
		bgfx::destroy(u_lightPos);
		bgfx::destroy(u_lightColor);
		bgfx::destroy(u_camPos);
		bgfx::destroy(u_time);
		bgfx::destroy(u_lightSetting);
		bgfx::destroy(u_pbrMaterial);
		bgfx::destroy(u_blinnMaterial);
		bgfx::destroy(u_shadowType);
		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}
	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState) )
		{
 			imguiBeginFrame(m_mouseState.m_mx
				,  m_mouseState.m_my
				, (m_mouseState.m_buttons[entry::MouseButton::Left  ] ? IMGUI_MBUT_LEFT   : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Right ] ? IMGUI_MBUT_RIGHT  : 0)
				| (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
				,  m_mouseState.m_mz
				, uint16_t(m_width)
				, uint16_t(m_height)
				);

			showExampleDialog(this);

			// build UI
			{
				ImGui::SetNextWindowPos(
					ImVec2(m_width - m_width / 5.0f - 10.0f, 10.0f)
					, ImGuiCond_FirstUseEver
				);
				ImGui::SetNextWindowSize(
					ImVec2(m_width / 5.0f, m_height - 20.0f)
					, ImGuiCond_FirstUseEver
				);
				ImGui::Begin("Settings"
					, NULL
					, 0
				);
				ImGui::PushItemWidth(200.0f);
				ImGui::Text("Skybox:");
				if (ImGui::BeginTabBar("Cubemap", ImGuiTabBarFlags_None))
				{
					if (ImGui::BeginTabItem("Bolonga"))
					{
						m_currBackground = Background::Enum::Bolonga;
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Kyoto"))
					{
						m_currBackground = Background::Enum::Kyoto;
						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
				
				ImGui::Text("Light Color:");
				ImGui::ColorWheel("", m_setting.m_lightColor, 0.6f);
				ImGui::Separator();
				
				ImGui::Text("Bunny : PBR + IBL");
				ImGui::Indent();
				ImGui::Text("Environment Light:");
				ImGui::Indent();
				ImGui::Checkbox("IBL Diffuse", &m_setting.m_doDiffuseIBL);
				ImGui::Checkbox("IBL Specular", &m_setting.m_doSpecularIBL);
				ImGui::Unindent();
				ImGui::Text("Directional Light:");
				ImGui::Indent();
				ImGui::Checkbox("Diffuse", &m_setting.m_doDiffuse);
				ImGui::Checkbox("Specular", &m_setting.m_doSpecular);
				ImGui::Unindent();
				ImGui::Text("Material:");
				ImGui::Indent();
				ImGui::Checkbox("Use Albedo Texture", &m_setting.m_bunnyUseAlbedo);
				ImGui::Checkbox("Use AORM Texture", &m_setting.m_useAORMTexture);
				if (!m_setting.m_useAORMTexture)
				{
					ImGui::PushItemWidth(130.0f);
					ImGui::SliderFloat("Glossiness", &m_setting.m_glossiness, 0.0f, 1.0f);
					ImGui::SliderFloat("Metalness", &m_setting.m_metalness, 0.0f, 1.0f);
					ImGui::PopItemWidth();
				}
				ImGui::Unindent();
				ImGui::Unindent();

				ImGui::Separator();
				ImGui::Text("Stone : BlinnPhong");
				ImGui::Indent();
				ImGui::Text("Material:");
				ImGui::Indent();
				ImGui::Checkbox("Use Normal Map", &m_setting.m_blinnUseNormalMap);	
				ImGui::Checkbox("Use Albedo Map", &m_setting.m_blinnUseAlbedoMap);

				ImGui::Unindent();
				ImGui::Unindent();

				ImGui::Separator();
				ImGui::Text("Shadow");
				ImGui::Indent();
				ImGui::RadioButton("Hard Shadow", &m_setting.m_shadowType, 0); ImGui::SameLine();
				ImGui::RadioButton("PCF 3x3", &m_setting.m_shadowType, 1);
			}
			ImGui::End();
			imguiEndFrame();

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			bgfx::touch(0);

			float time = (float)((bx::getHPCounter() - m_timeOffset) / double(bx::getHPFrequency()));
			bgfx::setUniform(u_time, &time);
			float lightSetting[4] = {m_setting.m_doDiffuse, m_setting.m_doSpecular, m_setting.m_doDiffuseIBL, m_setting.m_doSpecularIBL};
			float pbrMaterial[4] = { m_setting.m_useAORMTexture, m_setting.m_glossiness, m_setting.m_metalness, m_setting.m_bunnyUseAlbedo };
			bgfx::setUniform(u_lightSetting, &lightSetting);
			bgfx::setUniform(u_pbrMaterial, &pbrMaterial);
			float blinnMaterial[4] = { m_setting.m_blinnUseNormalMap, m_setting.m_blinnUseAlbedoMap, 0.0f, 0.0f };
			bgfx::setUniform(u_blinnMaterial, &blinnMaterial);
			float shadowType[4] = { m_setting.m_shadowType, 0.0f, 0.0f, 0.0f };
			bgfx::setUniform(u_shadowType, &shadowType);

			// set controller
			if (ImGui::GetIO().MouseDown[0] && !ImGui::MouseOverArea()) {
				m_horizontalAngle -= m_mouse_speed * ImGui::GetIO().MouseDelta.x;
				m_verticalAngle += m_mouse_speed * ImGui::GetIO().MouseDelta.y;
				m_verticalAngle = bx::clamp(m_verticalAngle, 0.1f, bx::kPi - 0.1f);
			}
			/*cout << "m_horizontalAngle" << m_horizontalAngle << endl;
			cout << "m_verticalAngle" << m_verticalAngle << endl;*/
			eye.x = (m_mouseState.m_mz - 30) * bx::sin(m_verticalAngle) * bx::cos(m_horizontalAngle);
			eye.z = (m_mouseState.m_mz - 30) * bx::sin(m_verticalAngle) * bx::sin(m_horizontalAngle);
			eye.y = (m_mouseState.m_mz - 30) * bx::cos(m_verticalAngle);
			if (inputGetKeyState(entry::Key::Enum::KeyW)) {
				m_verticalOffset -= 0.1;
			}
			if (inputGetKeyState(entry::Key::Enum::KeyS)) {
				m_verticalOffset += 0.1;
			}
			if (inputGetKeyState(entry::Key::Enum::KeyA)) {
				m_horizontalOffset += 0.1;
			}
			if (inputGetKeyState(entry::Key::Enum::KeyD)) {
				m_horizontalOffset -= 0.1;
			}

			// set shadowmap frame buffer
			if (!bgfx::isValid(m_shadowMapFB)) {
				bgfx::TextureHandle shadowMapTexture = BGFX_INVALID_HANDLE;
				bgfx::TextureHandle fbtextures[] =
				{
					bgfx::createTexture2D(
						  m_shadowMapSize
						, m_shadowMapSize
						, false
						, 1
						, bgfx::TextureFormat::D16
						, BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LEQUAL
						),
				};
				shadowMapTexture = fbtextures[0];
				m_textureShadowMap = shadowMapTexture;
				m_shadowMapFB = bgfx::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
			}

			// set light
			float lightPos[4] = {bx::cos(time)*10, 7.0f, bx::sin(time)*10, 0.0f};
			float lightColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			bx::memCopy(lightColor, m_setting.m_lightColor, 3 * sizeof(float));
			// model mtx
			float mtxFloor[16];
			float mtxMeshPBR[16];
			float mtxMeshBlinn[16];
			bx::mtxSRT(mtxFloor
				, 30.0f, 30.0f, 30.0f
				, 0.0f, 0.0f, 0.0f
				, 0.0f, 0.0f, 0.0f
			);

			bx::mtxSRT(mtxMeshPBR
				, 5.0f, 5.0f, 5.0f
				, 0.0f, 0.0f, 0.0f
				, -10.0f, 0.0f, 0.0f
			);

			bx::mtxSRT(mtxMeshBlinn
				, 1.0f, 1.0f, 1.0f
				, 0.0f, 0.0f, 0.0f
				, 10.0f, 0.0f, 0.0f
			);
			
			float lightView[16];
			float lightProj[16];
			// set shadow map
			{
				bgfx::setState(0
					| 0
					| BGFX_STATE_WRITE_Z
					| BGFX_STATE_DEPTH_TEST_LESS
					| BGFX_STATE_CULL_CCW
					| BGFX_STATE_MSAA
					);
				
				bx::mtxLookAt(lightView, eye, at);
				auto caps = bgfx::getCaps();
				bx::Vec3 at = { 0.0f,  0.0f,   0.0f };
				bx::Vec3 eye = { lightPos[0], lightPos[1], lightPos[2] };

				bx::mtxLookAt(lightView, eye, at);
				bx::mtxOrtho(lightProj, -30.0f, 30.0f, -30.0f, 30.0f, -100.0f, 100.0f, 0.0f, caps->homogeneousDepth);
				bgfx::setViewRect(RENDER_SHADOW_PASS_ID, 0, 0, m_shadowMapSize, m_shadowMapSize);
				bgfx::setViewTransform(RENDER_SHADOW_PASS_ID, lightView, lightProj);
				bgfx::setViewFrameBuffer(RENDER_SHADOW_PASS_ID, m_shadowMapFB);

				bgfx::setViewClear(RENDER_SHADOW_PASS_ID
					, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
					, 0x303030ff, 1.0f, 0
				);

				bgfx::setIndexBuffer(m_ibh);
				bgfx::setVertexBuffer(0, m_vbh);
				bgfx::setTransform(mtxFloor);
				bgfx::submit(RENDER_SHADOW_PASS_ID, m_programShadow);	
				//meshSubmit(m_meshPlatform, RENDER_SHADOW_PASS_ID, m_programShadow, mtxFloor);
				if (m_setting.m_pbrUseShadowSampler)
					meshSubmit(m_meshPBR, RENDER_SHADOW_PASS_ID, m_programShadow, mtxMeshPBR);
				if (m_setting.m_blinnUseShadowSampler)
					meshSubmit(m_meshBlinn, RENDER_SHADOW_PASS_ID, m_programShadow, mtxMeshBlinn);
			}

			// draw mesh
			{
				bgfx::setState(0
					| BGFX_STATE_WRITE_RGB
					| BGFX_STATE_WRITE_A
					| BGFX_STATE_WRITE_Z
					| BGFX_STATE_DEPTH_TEST_LESS
					| BGFX_STATE_CULL_CCW
					| BGFX_STATE_MSAA);
				float view[16];
				float proj[16];
				bx::mtxLookAt(view, eye, at);
				view[12] = m_horizontalOffset;
				view[13] = m_verticalOffset;
				bx::mtxProj(proj, 60.0f, float(m_width) / float(m_height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
				bgfx::setViewRect(RENDER_SCENE_PASS_ID, 0, 0, uint16_t(m_width), uint16_t(m_height));
				bgfx::setViewTransform(RENDER_SCENE_PASS_ID, view, proj);

				bgfx::setUniform(u_lightColor, lightColor, 1);
				bgfx::setUniform(u_lightPos, lightPos, 1);

				float mtxToLightMtx[16]; // transpose different models to light space
				float mtxShadow[16];  // the vp matrix for light space

				const float sy = bgfx::getCaps()->originBottomLeft ? 0.5f : -0.5f;
				const float sz = bgfx::getCaps()->homogeneousDepth ? 0.5f : 1.0f;
				const float tz = bgfx::getCaps()->homogeneousDepth ? 0.5f : 0.0f;
				const float mtxCrop[16] =
				{
					0.5f, 0.0f, 0.0f, 0.0f,
					0.0f,   sy, 0.0f, 0.0f,
					0.0f, 0.0f, sz,   0.0f,
					0.5f, 0.5f, tz,   1.0f,
				};
				float mtxTmp[16];
				bx::mtxMul(mtxTmp, lightProj, mtxCrop);
				bx::mtxMul(mtxShadow, lightView, mtxTmp);

				// floor
				bx::mtxMul(mtxToLightMtx, mtxFloor, mtxShadow);
				bgfx::setUniform(u_lightMtx, mtxToLightMtx);
				bgfx::setIndexBuffer(m_ibh);
				bgfx::setVertexBuffer(0, m_vbh);
				bgfx::setTexture(0, s_texColor, m_textureColor);
				bgfx::setTexture(1, s_texNormal, m_textureNormal);
				bgfx::setTexture(2, s_texAorm, m_textureAorm);
				bgfx::setTexture(3, s_texLUT, m_textureLUT);
				bgfx::setTexture(4, s_texLod, m_background.m_textureLod[m_currBackground]);
				bgfx::setTexture(5, s_texIrr, m_background.m_textureIrr[m_currBackground]);
				bgfx::setTexture(6, s_texShadowMap, m_textureShadowMap, UINT32_MAX);
				bgfx::setTransform(mtxFloor);
				bgfx::submit(RENDER_SCENE_PASS_ID, m_programFloor);
				//meshSubmit(m_meshPlatform, RENDER_SCENE_PASS_ID, m_programFloor, mtxFloor);

				// bunny 
				bgfx::setTexture(0, s_texColor, m_textureBunny);
				bgfx::setTexture(1, s_texNormal, m_textureNormal);
				bgfx::setTexture(2, s_texAorm, m_textureAorm);
				bgfx::setTexture(3, s_texLUT, m_textureLUT);
				bgfx::setTexture(4, s_texLod, m_background.m_textureLod[m_currBackground]);
				bgfx::setTexture(5, s_texIrr, m_background.m_textureIrr[m_currBackground]);
				meshSubmit(m_meshPBR, RENDER_SCENE_PASS_ID, m_programMeshPBR, mtxMeshPBR);

				// stone
				bgfx::setTexture(0, s_texColor, m_textureColor);
				bgfx::setTexture(1, s_texNormal, m_textureNormal);
				meshSubmit(m_meshBlinn, RENDER_SCENE_PASS_ID, m_programMeshBlinn, mtxMeshBlinn);
			}
			
			// set skybox
			{
				float view[16];
				float proj[16];
				// in pass0, the position of camera is at the origin, and look at the positive direction of the z-axis 
				bx::mtxIdentity(view);
				bx::mtxOrtho(proj, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 100.0f, 0.0, bgfx::getCaps()->homogeneousDepth);
				bgfx::setViewTransform(0, view, proj);

				bgfx::setTexture(3, s_texLod, m_background.m_textureLod[m_currBackground]);
				bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
				screenSpaceQuad((float)m_width, (float)m_height, false, 1.0f, 1.0f);
				bx::mtxLookAt(shaderParams.view_mtx, eye, at);
				shaderParams.submit();
				bgfx::setViewRect(RENDER_SKYBOX_PASS_ID, 0, 0, uint16_t(m_width), uint16_t(m_height));
				bgfx::submit(0, m_programLod);
			}	
			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();
			return true;
		}

		return false;
	}

	entry::MouseState m_mouseState;

	bgfx::UniformHandle u_camPos;
	bgfx::UniformHandle s_texColor;
	bgfx::UniformHandle s_texNormal;
	bgfx::UniformHandle s_texAorm;
	bgfx::UniformHandle s_texLod;
	bgfx::UniformHandle s_texIrr;
	bgfx::UniformHandle s_texLUT;
	bgfx::UniformHandle s_texShadowMap;
	bgfx::UniformHandle u_lightPos;
	bgfx::UniformHandle u_lightColor;
	bgfx::UniformHandle u_lightMtx;
	bgfx::UniformHandle u_viewMtx;
	bgfx::UniformHandle u_lightSetting;
	bgfx::UniformHandle u_pbrMaterial;
	bgfx::UniformHandle u_blinnMaterial;
	bgfx::UniformHandle u_useAlbedoTexture;
	bgfx::UniformHandle u_useNormalTexture;
	bgfx::UniformHandle u_shadowType;

	bgfx::TextureHandle m_textureColor;
	bgfx::TextureHandle m_textureNormal;
	bgfx::TextureHandle m_textureAorm;
	bgfx::TextureHandle m_textureBunny;
	
	bgfx::TextureHandle m_textureLUT;
	bgfx::TextureHandle m_textureShadowMap;
	uint16_t m_numLights;

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
	
	int64_t m_timeOffset;
	Mesh* m_meshPBR;
	Mesh* m_meshBlinn;
	Mesh* m_meshPlatform;
	bgfx::ProgramHandle m_programShadow;
	bgfx::ProgramHandle m_programMeshPBR;
	bgfx::ProgramHandle m_programMeshBlinn;
	bgfx::ProgramHandle m_programLod;
	bgfx::ProgramHandle m_programFloor;
	bgfx::UniformHandle u_time;

	float m_horizontalAngle = -1.5;
	float m_verticalAngle = 2;

	const float m_mouse_speed = 0.0037;

	float m_verticalOffset = 0;
	float m_horizontalOffset = 0;

	bx::Vec3 at = { 0.0f, 0.0f,  0.0f };
	bx::Vec3 eye = { 0.0f, 0.0f, 0.0f };

	ShaderParams shaderParams;
	Setting m_setting;
	Background m_background;
	Background::Enum m_currBackground;

	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh;

	uint16_t m_shadowMapSize;

	bgfx::FrameBufferHandle m_shadowMapFB;

};	

} // namespdoSpecularIBL;ace

int _main_(int _argc, char** _argv)
{
	EStarHomework app("e-star-homework", "", "");
	return entry::runApp(&app, _argc, _argv);
}

