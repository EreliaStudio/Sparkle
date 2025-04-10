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

			Object(spk::SafePointer<Pipeline> p_owner) :
				_owner(p_owner)
			{
			}

		public:
			Object() :
				_owner(nullptr)
			{
			}

			void setNbInstance(size_t p_nbInstance)
			{
				_nbInstance = p_nbInstance;
			}

			spk::OpenGL::BufferSet& bufferSet()
			{
				return _bufferSet;
			}

			spk::OpenGL::UniformBufferObject& ubo(const std::wstring &p_name)
			{
				if (_ubos.find(p_name) == _ubos.end())
				{
					throw std::runtime_error("Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Object.");
				}
				return _ubos[p_name];
			}

			spk::OpenGL::ShaderStorageBufferObject& ssbo(const std::wstring &p_name)
			{
				if (_ssbos.find(p_name) == _ssbos.end())
				{
					throw std::runtime_error("Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Object.");
				}
				return _ssbos[p_name];
			}

			spk::OpenGL::SamplerObject& sampler(const std::wstring &p_name)
			{
				if (_samplers.find(p_name) == _samplers.end())
				{
					throw std::runtime_error("Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Object.");
				}
				return _samplers[p_name];
			}

			spk::OpenGL::UniformObject& uniform(const std::wstring &p_name)
			{
				if (_uniforms.find(p_name) == _uniforms.end())
				{
					throw std::runtime_error("Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Object.");
				}
				return _uniforms[p_name];
			}

			void render()
			{
				if (_owner == nullptr)
				{
					throw std::runtime_error("Pipeline owner is null.");
				}

				std::lock_guard<std::mutex> lock(_owner->_mutex);

				_owner->beginRender();

				_bufferSet.activate();

				for (auto& [name, ubo] : _ubos)
				{
					ubo.activate();
				}

				for (auto& [name, ssbo] : _ssbos)
				{
					ssbo.activate();
				}

				for (auto& [name, sampler] : _samplers)
				{
					sampler.activate();
				}

				for (auto& [name, uniform] : _uniforms)
				{
					uniform.activate();
				}
				
				_owner->render(_bufferSet.indexes().nbIndexes(), _nbInstance);

				_bufferSet.deactivate();
				
				for (auto& [name, ubo] : _ubos)
				{
					ubo.deactivate();
				}

				for (auto& [name, ssbo] : _ssbos)
				{
					ssbo.deactivate();
				}

				for (auto& [name, sampler] : _samplers)
				{
					sampler.deactivate();
				}

				_owner->endRender();
			}
		};

	private:
		mutable std::mutex _mutex;

		spk::OpenGL::Program _program;

		std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject> _ubos;
		std::unordered_map<std::wstring, spk::OpenGL::ShaderStorageBufferObject> _ssbos;
		std::unordered_map<std::wstring, spk::OpenGL::SamplerObject> _samplers;
		std::unordered_map<std::wstring, spk::OpenGL::UniformObject> _uniforms;

		Object _defaultObject;

		void beginRender()
		{
			_program.activate();

			for (auto& [name, ubo] : _ubos)
			{
				ubo.activate();
			}

			for (auto& [name, ssbo] : _ssbos)
			{
				ssbo.activate();
			}

			for (auto& [name, sampler] : _samplers)
			{
				sampler.activate();
			}

			for (auto& [name, uniform] : _uniforms)
			{
				uniform.activate();
			}
		}

		void render(size_t p_nbIndexes, size_t p_nbInstance)
		{
			if (p_nbIndexes == 0)
				return;

			if (p_nbInstance > 0)
			{
				glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>(p_nbIndexes), static_cast<GLsizei>(p_nbInstance));
			}
			else
			{
				glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(p_nbIndexes));
			}
		}

		void endRender()
		{
			for (auto& [name, ubo] : _ubos)
			{
				ubo.deactivate();
			}

			for (auto& [name, ssbo] : _ssbos)
			{
				ssbo.deactivate();
			}

			for (auto& [name, sampler] : _samplers)
			{
				sampler.deactivate();
			}

			_program.deactivate();
		}

	public:
		Pipeline(const std::string &p_vertexShaderCode, const std::string &p_fragmentShaderCode) :
			_program(p_vertexShaderCode, p_fragmentShaderCode)
		{
		}

		void addUniformBufferObject(const std::wstring &p_name, const spk::OpenGL::UniformBufferObject &p_ubo)
		{
			if (_ubos.find(p_name) != _ubos.end())
			{
				throw std::runtime_error("Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
			}
			_ubos[p_name] = p_ubo;
		}

		void addShaderStorageBufferObject(const std::wstring &p_name, const spk::OpenGL::ShaderStorageBufferObject &p_ssbo)
		{
			if (_ssbos.find(p_name) != _ssbos.end())
			{
				throw std::runtime_error("Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
			}
			_ssbos[p_name] = p_ssbo;
		}

		void addSamplerObject(const std::wstring &p_name, const spk::OpenGL::SamplerObject &p_sampler)
		{
			if (_samplers.find(p_name) != _samplers.end())
			{
				throw std::runtime_error("Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
			}
			_samplers[p_name] = p_sampler;
		}

		void addUniformObject(const std::wstring &p_name, const spk::OpenGL::UniformObject &p_uniform)
		{
			if (_uniforms.find(p_name) != _uniforms.end())
			{
				throw std::runtime_error("Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
			}
			_uniforms[p_name] = p_uniform;
		}

		spk::OpenGL::UniformBufferObject& ubo(const std::wstring &p_name)
		{
			if (_ubos.find(p_name) == _ubos.end())
			{
				throw std::runtime_error("Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
			}
			return _ubos[p_name];
		}

		spk::OpenGL::ShaderStorageBufferObject& ssbo(const std::wstring &p_name)
		{
			if (_ssbos.find(p_name) == _ssbos.end())
			{
				throw std::runtime_error("Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
			}
			return _ssbos[p_name];
		}

		spk::OpenGL::SamplerObject& sampler(const std::wstring &p_name)
		{
			if (_samplers.find(p_name) == _samplers.end())
			{
				throw std::runtime_error("Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
			}
			return _samplers[p_name];
		}

		spk::OpenGL::UniformObject& uniform(const std::wstring &p_name)
		{
			if (_uniforms.find(p_name) == _uniforms.end())
			{
				throw std::runtime_error("Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
			}
			return _uniforms[p_name];
		}

		void addObjectLayoutAttribute(spk::OpenGL::LayoutBufferObject::Attribute::Index p_index,
									spk::OpenGL::LayoutBufferObject::Attribute::Type p_type)
		{
			if (_defaultObject._bufferSet.layout().hasAttribute(p_index))
			{
				throw std::runtime_error("Attribute location " + std::to_string(p_index) + " has already been defined.");
			}
			_defaultObject._bufferSet.layout().addAttribute(p_index, p_type);
		}

		void addObjectUniformBufferObject(const std::wstring &p_name, const spk::OpenGL::UniformBufferObject &p_ubo)
		{
			if (_defaultObject._ubos.find(p_name) != _defaultObject._ubos.end())
			{
				throw std::runtime_error("Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
			}
			_defaultObject._ubos[p_name] = p_ubo;
		}

		void addObjectShaderStorageBufferObject(const std::wstring &p_name, const spk::OpenGL::ShaderStorageBufferObject &p_ssbo)
		{
			if (_defaultObject._ssbos.find(p_name) != _defaultObject._ssbos.end())
			{
				throw std::runtime_error("Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
			}
			_defaultObject._ssbos[p_name] = p_ssbo;
		}

		void addObjectSamplerObject(const std::wstring &p_name, const spk::OpenGL::SamplerObject &p_sampler)
		{
			if (_defaultObject._samplers.find(p_name) != _defaultObject._samplers.end())
			{
				throw std::runtime_error("Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
			}
			_defaultObject._samplers[p_name] = p_sampler;
		}

		void addObjectUniformObject(const std::wstring &p_name, const spk::OpenGL::UniformObject &p_uniform)
		{
			if (_defaultObject._uniforms.find(p_name) != _defaultObject._uniforms.end())
			{
				throw std::runtime_error("Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
			}
			_defaultObject._uniforms[p_name] = p_uniform;
		}

		Object createObject()
		{
			std::lock_guard<std::mutex> lock(_mutex);

			return _defaultObject;
		}
	};
}
