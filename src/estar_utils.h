#pragma once
#include <bx/uint32_t.h>
#include "common.h"
#include "bgfx_utils.h"
#include "bgfx_logo.h"
#include "imgui/imgui.h"
#include "entry/input.h"
#include <iostream>
#include <vector>
namespace estar {

	struct Texture
	{
		uint32_t            m_flags;
		bgfx::UniformHandle m_sampler;
		bgfx::TextureHandle m_texture;
		uint8_t             m_stage;
		Texture(uint32_t _m_flags, bgfx::UniformHandle _m_sampler, bgfx::TextureHandle _m_texture, uint8_t _m_stage) :
			m_flags(_m_flags), m_sampler(_m_sampler), m_texture(_m_texture), m_stage(_m_stage) {};
	};

	class MeshState {
	public:
		MeshState() : m_state(0), m_program(BGFX_INVALID_HANDLE), m_viewId(0), m_maxNumTextures(8) {};
		MeshState(uint8_t _m_maxNumTextures) : m_state(0), m_program(BGFX_INVALID_HANDLE), m_viewId(0), m_maxNumTextures(_m_maxNumTextures) {};
		bool addTextures(uint32_t _m_flags, bgfx::UniformHandle _m_sampler, bgfx::TextureHandle _m_texture, uint8_t _m_stage) {
			if (textures.size() >= m_maxNumTextures) return false;
			textures.emplace_back(_m_flags, _m_sampler, _m_texture, _m_stage);
			return true;
		}
		bool addTextures(Texture&& _texture) {
			if (textures.size() >= m_maxNumTextures) return false;
			textures.emplace_back(std::forward<Texture>(_texture));
			return true;
		}
		Texture& getTextures(uint8_t _index) {
			//if (_index >= (uint8_t)textures.size()) return {0, BGFX_INVALID_HANDLE , BGFX_INVALID_HANDLE, 0};
			return textures[_index];
		}
		uint64_t            m_state;
		bgfx::ProgramHandle m_program;
		bgfx::ViewId        m_viewId;
		std::vector<Texture>     textures;
	private:
		uint8_t             m_maxNumTextures;
		
	};


} // namespace estar