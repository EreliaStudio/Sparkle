#pragma once

#include <set>
#include <mutex>
#include <variant>
#include <unordered_map>
#include <stdexcept>
#include <string>

#include "structure/spk_safe_pointer.hpp"
#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_uniform_object.hpp"
#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"
#include "structure/graphics/opengl/spk_shader_storage_buffer_object.hpp"

namespace spk
{
	class Pipeline
	{
	public:
		class Object
		{
			friend class Pipeline;

		private:
			spk::SafePointer<Pipeline> _owner;
			size_t _nbInstance = 0;

			spk::OpenGL::BufferSet _bufferSet;

			std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject> _ubos;
			std::unordered_map<std::wstring, spk::OpenGL::ShaderStorageBufferObject> _ssbos;
			std::unordered_map<std::wstring, spk::OpenGL::SamplerObject> _samplers;
			std::unordered_map<std::wstring, spk::OpenGL::UniformObject> _uniforms;

			Object(spk::SafePointer<Pipeline> p_owner);

		public:
			Object();

			void setNbInstance(size_t p_nbInstance);
			spk::OpenGL::BufferSet& bufferSet();

			spk::OpenGL::UniformBufferObject& ubo(const std::wstring &p_name);
			spk::OpenGL::ShaderStorageBufferObject& ssbo(const std::wstring &p_name);
			spk::OpenGL::SamplerObject& sampler(const std::wstring &p_name);
			spk::OpenGL::UniformObject& uniform(const std::wstring &p_name);

			const spk::OpenGL::UniformBufferObject& ubo(const std::wstring &p_name) const;
			const spk::OpenGL::ShaderStorageBufferObject& ssbo(const std::wstring &p_name) const;
			const spk::OpenGL::SamplerObject& sampler(const std::wstring &p_name) const;
			const spk::OpenGL::UniformObject& uniform(const std::wstring &p_name) const;

			void render();
		};

	private:
		spk::OpenGL::Program _program;

		std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject> _ubos;
		std::unordered_map<std::wstring, spk::OpenGL::ShaderStorageBufferObject> _ssbos;
		std::unordered_map<std::wstring, spk::OpenGL::SamplerObject> _samplers;
		std::unordered_map<std::wstring, spk::OpenGL::UniformObject> _uniforms;

		Object _defaultObject;

		void beginRender();
		void render(size_t p_nbIndexes, size_t p_nbInstance);
		void endRender();

	public:
		Pipeline();
		Pipeline(const std::string &p_vertexShaderCode, const std::string &p_fragmentShaderCode);

		void load(const std::string &p_vertexShaderCode, const std::string &p_fragmentShaderCode);

		void addUniformBufferObject(const std::wstring &p_name, spk::OpenGL::UniformBufferObject &&p_ubo);
		void addShaderStorageBufferObject(const std::wstring &p_name, spk::OpenGL::ShaderStorageBufferObject &&p_ssbo);
		void addSamplerObject(const std::wstring &p_name, spk::OpenGL::SamplerObject &&p_sampler);
		void addUniformObject(const std::wstring &p_name, spk::OpenGL::UniformObject &&p_uniform);

		spk::OpenGL::UniformBufferObject& ubo(const std::wstring &p_name);
		spk::OpenGL::ShaderStorageBufferObject& ssbo(const std::wstring &p_name);
		spk::OpenGL::SamplerObject& sampler(const std::wstring &p_name);
		spk::OpenGL::UniformObject& uniform(const std::wstring &p_name);

		const spk::OpenGL::UniformBufferObject& ubo(const std::wstring &p_name) const;
		const spk::OpenGL::ShaderStorageBufferObject& ssbo(const std::wstring &p_name) const;
		const spk::OpenGL::SamplerObject& sampler(const std::wstring &p_name) const;
		const spk::OpenGL::UniformObject& uniform(const std::wstring &p_name) const;

		bool containsUbo(const std::wstring& p_name) const;
		bool containsSsbo(const std::wstring& p_name) const;
		bool containsSampler(const std::wstring& p_name) const;
		bool containsUniform(const std::wstring& p_name) const;

		void addObjectLayoutAttribute(spk::OpenGL::LayoutBufferObject::Attribute::Index p_index,
									  spk::OpenGL::LayoutBufferObject::Attribute::Type p_type);

		void addObjectUniformBufferObject(const std::wstring &p_name, spk::OpenGL::UniformBufferObject &&p_ubo);
		void addObjectShaderStorageBufferObject(const std::wstring &p_name, spk::OpenGL::ShaderStorageBufferObject &&p_ssbo);
		void addObjectSamplerObject(const std::wstring &p_name, spk::OpenGL::SamplerObject &&p_sampler);
		void addObjectUniformObject(const std::wstring &p_name, spk::OpenGL::UniformObject &&p_uniform);

		Object createObject();
	};
}
