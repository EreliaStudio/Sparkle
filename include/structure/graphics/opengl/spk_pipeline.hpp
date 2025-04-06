#pragma once

#include <set>

#include "structure/spk_safe_pointer.hpp"

#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_uniform_object.hpp"
#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"
#include "structure/graphics/opengl/spk_shader_storage_buffer_object.hpp"

namespace spk
{
	class Pipeline
	{
	public:
		using Constant = std::variant<spk::OpenGL::UniformBufferObject, spk::OpenGL::ShaderStorageBufferObject>;

		class Object
		{
			friend class Pipeline;

		public:
			using Attribute = Pipeline::Constant;
			
		private:
			spk::SafePointer<Pipeline> _owner;
			size_t _nbInstance = 0;
			spk::OpenGL::BufferSet _bufferSet;
			std::unordered_map<std::wstring, Attribute> _attributes;

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

			void render()
			{
				_owner->_preRender();

				_bufferSet.activate();
				for (auto& attribute : _attributes)
				{
					if (std::holds_alternative<spk::OpenGL::UniformBufferObject>(attribute.second))
					{
						std::get<spk::OpenGL::UniformBufferObject>(attribute.second).activate();
					}
					else if (std::holds_alternative<spk::OpenGL::ShaderStorageBufferObject>(attribute.second))
					{
						std::get<spk::OpenGL::ShaderStorageBufferObject>(attribute.second).activate();
					}
				}

				_owner->_render(_bufferSet.indexes().nbIndexes(), _nbInstance);
				
				_bufferSet.deactivate();
				for (auto& attribute : _attributes)
				{
					if (std::holds_alternative<spk::OpenGL::UniformBufferObject>(attribute.second))
					{
						std::get<spk::OpenGL::UniformBufferObject>(attribute.second).deactivate();
					}
					else if (std::holds_alternative<spk::OpenGL::ShaderStorageBufferObject>(attribute.second))
					{
						std::get<spk::OpenGL::ShaderStorageBufferObject>(attribute.second).deactivate();
					}
				}				
				_owner->_postRender();
			}

			spk::OpenGL::UniformBufferObject& ubo(const std::wstring& p_name)
			{
				if (_attributes.find(p_name) == _attributes.end())
				{
					throw std::runtime_error("Attribute [" + spk::StringUtils::wstringToString(p_name) + "] not found");
				}
				if (std::holds_alternative<spk::OpenGL::UniformBufferObject>(_attributes[p_name]) == false)
				{
					throw std::runtime_error("Attribute [" + spk::StringUtils::wstringToString(p_name) + "] is not a UBO");
				}
				return std::get<spk::OpenGL::UniformBufferObject>(_attributes[p_name]);
			}

			spk::OpenGL::ShaderStorageBufferObject& ssbo(const std::wstring& p_name)
			{
				if (_attributes.find(p_name) == _attributes.end())
				{
					throw std::runtime_error("Attribute [" + spk::StringUtils::wstringToString(p_name) + "] not found");
				}
				if (std::holds_alternative<spk::OpenGL::ShaderStorageBufferObject>(_attributes[p_name]) == false)
				{
					throw std::runtime_error("Attribute [" + spk::StringUtils::wstringToString(p_name) + "] is not a SSBO");
				}
				return std::get<spk::OpenGL::ShaderStorageBufferObject>(_attributes[p_name]);
			}
		};

	private:
		spk::OpenGL::Program _program;

		static inline std::unordered_map<std::wstring, Constant> _constants;
		std::set<std::wstring> _usedConstants;

		std::unordered_map<std::wstring, Object::Attribute> _objectUbos;
		std::vector<spk::OpenGL::LayoutBufferObject::Attribute> _objectLayoutAttributes;

		void _preRender()
		{
			_program.activate();

			for (auto& constant : _constants)
			{
				if (std::holds_alternative<spk::OpenGL::UniformBufferObject>(constant.second))
				{
					std::get<spk::OpenGL::UniformBufferObject>(constant.second).activate();
				}
				else if (std::holds_alternative<spk::OpenGL::ShaderStorageBufferObject>(constant.second))
				{
					std::get<spk::OpenGL::ShaderStorageBufferObject>(constant.second).activate();
				}
			}
		}

		void _render(size_t nbIndex, size_t nbInstance)
		{
			_program.render(nbIndex, nbInstance);
		}

		void _postRender()
		{
			for (auto& constant : _constants)
			{
				if (std::holds_alternative<spk::OpenGL::UniformBufferObject>(constant.second))
				{
					std::get<spk::OpenGL::UniformBufferObject>(constant.second).deactivate();
				}
				else if (std::holds_alternative<spk::OpenGL::ShaderStorageBufferObject>(constant.second))
				{
					std::get<spk::OpenGL::ShaderStorageBufferObject>(constant.second).deactivate();
				}
			}

			_program.deactivate();
		}

	public:
		Pipeline(const std::string &p_vertexShaderCode, const std::string &p_fragmentShaderCode) :
			_program(p_vertexShaderCode, p_fragmentShaderCode)
		{

		}

		void addConstant(const std::wstring& p_name, spk::OpenGL::UniformBufferObject&& p_ubo)
		{
			if (_constants.find(p_name) != _constants.end())
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] already created in Pipeline");
			}
			if (_usedConstants.find(p_name) != _usedConstants.end())
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] already used in Pipeline");
			}
			if (std::holds_alternative<spk::OpenGL::UniformBufferObject>(_constants[p_name]))
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] is not a UBO");
			}
			_constants[p_name] = std::move(p_ubo);
		}

		void addConstant(const std::wstring& p_name, spk::OpenGL::ShaderStorageBufferObject&& p_ssbo)
		{
			_constants[p_name] = std::move(p_ssbo);
		}

		void addAttribute(const std::wstring& p_name, spk::OpenGL::UniformBufferObject&& p_ubo)
		{
			_objectUbos[p_name] = std::move(p_ubo);
		}

		void addAttribute(const std::wstring& p_name, spk::OpenGL::ShaderStorageBufferObject&& p_ssbo)
		{
			_objectUbos[p_name] = std::move(p_ssbo);
		}

		void addLayoutAttribute(const spk::OpenGL::LayoutBufferObject::Attribute& p_attribute)
		{
			_objectLayoutAttributes.push_back(p_attribute);
		}

		void addLayoutAttribute(spk::OpenGL::LayoutBufferObject::Attribute::Index p_index, spk::OpenGL::LayoutBufferObject::Attribute::Type p_type)
		{
			addLayoutAttribute(spk::OpenGL::LayoutBufferObject::Attribute(p_index, p_type));
		}

		Object createObject()
		{
			Object result(this);

			for (auto& objectUBO : _objectUbos)
			{
				result._attributes[objectUBO.first] = std::move(objectUBO.second);
			}

			for (auto& objectLayoutAttribute : _objectLayoutAttributes)
			{
				result._bufferSet.layout().addAttribute(objectLayoutAttribute);
			}

			return (result);
		}

		spk::OpenGL::UniformBufferObject& ubo(const std::wstring& p_name)
		{
			if (_constants.find(p_name) == _constants.end())
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] not found");
			}
			if (std::holds_alternative<spk::OpenGL::UniformBufferObject>(_constants[p_name]) == false)
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] is not a UBO");
			}
			return (std::get<spk::OpenGL::UniformBufferObject>(_constants[p_name]));
		}

		spk::OpenGL::ShaderStorageBufferObject& ssbo(const std::wstring& p_name)
		{
			if (_constants.find(p_name) == _constants.end())
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] not found");
			}
			if (std::holds_alternative<spk::OpenGL::ShaderStorageBufferObject>(_constants[p_name]) == false)
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] is not a SSBO");
			}
			return (std::get<spk::OpenGL::ShaderStorageBufferObject>(_constants[p_name]));
		}
	};
}