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
				if (_owner != nullptr)
				{
					std::lock_guard<std::mutex> lock(_owner->_mutex);
					_nbInstance = p_nbInstance;
				}
				else
				{
					_nbInstance = p_nbInstance;
				}
			}

			void render()
			{
				if (_owner == nullptr)
					return;

				std::lock_guard<std::mutex> lock(_owner->_mutex);

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

			spk::OpenGL::BufferSet& bufferSet()
			{
				return _bufferSet;
			}

			spk::OpenGL::UniformBufferObject& ubo(const std::wstring& p_name)
			{
				if (_owner != nullptr)
				{
					std::lock_guard<std::mutex> lock(_owner->_mutex);

					if (_attributes.find(p_name) == _attributes.end())
					{
						if (_owner->contains(p_name))
						{
							return _owner->ubo(p_name);
						}
						throw std::runtime_error("Attribute [" + spk::StringUtils::wstringToString(p_name) + "] not found");
					}
					if (!std::holds_alternative<spk::OpenGL::UniformBufferObject>(_attributes[p_name]))
					{
						throw std::runtime_error("Attribute [" + spk::StringUtils::wstringToString(p_name) + "] is not a UBO");
					}
					return std::get<spk::OpenGL::UniformBufferObject>(_attributes[p_name]);
				}
				else
				{
					throw std::runtime_error("Object has no valid Pipeline owner.");
				}
			}

			spk::OpenGL::ShaderStorageBufferObject& ssbo(const std::wstring& p_name)
			{
				if (_owner != nullptr)
				{
					std::lock_guard<std::mutex> lock(_owner->_mutex);
					if (_attributes.find(p_name) == _attributes.end())
					{
						if (_owner->contains(p_name))
						{
							return _owner->ssbo(p_name);
						}
						throw std::runtime_error("Attribute [" + spk::StringUtils::wstringToString(p_name) + "] not found");
					}
					if (!std::holds_alternative<spk::OpenGL::ShaderStorageBufferObject>(_attributes[p_name]))
					{
						throw std::runtime_error("Attribute [" + spk::StringUtils::wstringToString(p_name) + "] is not a SSBO");
					}
					return std::get<spk::OpenGL::ShaderStorageBufferObject>(_attributes[p_name]);
				}
				else
				{
					throw std::runtime_error("Object has no valid Pipeline owner.");
				}
			}
		};

	private:
		spk::OpenGL::Program _program;

		std::unordered_map<std::wstring, Constant> _constants;

		std::unordered_map<std::wstring, Object::Attribute> _objectUbos;
		std::unordered_map<spk::OpenGL::LayoutBufferObject::Attribute::Index, spk::OpenGL::LayoutBufferObject::Attribute::Type> _objectLayoutAttributes;

		mutable std::mutex _mutex;

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

		spk::OpenGL::UniformBufferObject& addConstant(const std::wstring& p_name, spk::OpenGL::UniformBufferObject&& p_ubo)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			if (_constants.find(p_name) != _constants.end())
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] already defined");
			}
			_constants[p_name] = std::move(p_ubo);
			return std::get<spk::OpenGL::UniformBufferObject>(_constants[p_name]);
		}

		spk::OpenGL::UniformBufferObject& addConstant(const std::wstring& p_name,
			spk::OpenGL::UniformBufferObject::BindingPoint p_bindingPoint, size_t p_size)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			return addConstant(
				p_name, 
				spk::OpenGL::UniformBufferObject(p_name, p_bindingPoint, p_size)
			);
		}

		spk::OpenGL::ShaderStorageBufferObject& addConstant(const std::wstring& p_name, spk::OpenGL::ShaderStorageBufferObject&& p_ssbo)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			if (_constants.find(p_name) != _constants.end())
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] already defined");
			}
			_constants[p_name] = std::move(p_ssbo);
			return std::get<spk::OpenGL::ShaderStorageBufferObject>(_constants[p_name]);
		}

		spk::OpenGL::ShaderStorageBufferObject& addConstant(const std::wstring& p_name,
			spk::OpenGL::ShaderStorageBufferObject::BindingPoint p_bindingPoint, size_t p_fixedSize,
			size_t p_paddingFixedToDynamic, size_t p_dynamicElementSize, size_t p_dynamicElementPadding)
		{
			std::lock_guard<std::mutex> lock(_mutex);
			return addConstant(
				p_name,
				spk::OpenGL::ShaderStorageBufferObject(
					p_name, p_bindingPoint,
					p_fixedSize, p_paddingFixedToDynamic,
					p_dynamicElementSize, p_dynamicElementPadding
				)
			);
		}

		spk::OpenGL::UniformBufferObject& addAttribute(const std::wstring& p_name, spk::OpenGL::UniformBufferObject&& p_ubo)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			if (_objectUbos.find(p_name) != _objectUbos.end())
			{
				throw std::runtime_error("Attribute [" + spk::StringUtils::wstringToString(p_name) + "] already used in Pipeline");
			}

			_objectUbos[p_name] = std::move(p_ubo);
			return std::get<spk::OpenGL::UniformBufferObject>(_objectUbos[p_name]);
		}

		spk::OpenGL::UniformBufferObject& addAttribute(const std::wstring& p_name,
			spk::OpenGL::UniformBufferObject::BindingPoint p_bindingPoint, size_t p_size)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			return addAttribute(
				p_name,
				spk::OpenGL::UniformBufferObject(p_name, p_bindingPoint, p_size)
			);
		}

		spk::OpenGL::ShaderStorageBufferObject& addAttribute(const std::wstring& p_name, spk::OpenGL::ShaderStorageBufferObject&& p_ssbo)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			if (_objectUbos.find(p_name) != _objectUbos.end())
			{
				throw std::runtime_error("Attribute [" + spk::StringUtils::wstringToString(p_name) + "] already used in Pipeline");
			}

			_objectUbos[p_name] = std::move(p_ssbo);
			return std::get<spk::OpenGL::ShaderStorageBufferObject>(_objectUbos[p_name]);
		}

		spk::OpenGL::ShaderStorageBufferObject& addAttribute(const std::wstring& p_name,
			spk::OpenGL::ShaderStorageBufferObject::BindingPoint p_bindingPoint, size_t p_fixedSize,
			size_t p_paddingFixedToDynamic, size_t p_dynamicElementSize, size_t p_dynamicElementPadding)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			return addAttribute(
				p_name,
				spk::OpenGL::ShaderStorageBufferObject(
					p_name, p_bindingPoint,
					p_fixedSize, p_paddingFixedToDynamic,
					p_dynamicElementSize, p_dynamicElementPadding
				)
			);
		}

		void addLayoutAttribute(const spk::OpenGL::LayoutBufferObject::Attribute& p_attribute)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			addLayoutAttribute(p_attribute.index, p_attribute.type);
		}

		void addLayoutAttribute(spk::OpenGL::LayoutBufferObject::Attribute::Index p_index,
			spk::OpenGL::LayoutBufferObject::Attribute::Type p_type)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			if (_objectLayoutAttributes.find(p_index) != _objectLayoutAttributes.end())
			{
				throw std::runtime_error("Layout index [" + std::to_string(p_index) + "] already used in Pipeline");
			}
			_objectLayoutAttributes[p_index] = p_type;
		}

		Object createObject()
		{
			std::lock_guard<std::mutex> lock(_mutex);

			Object result(this);

			for (auto& objectUBO : _objectUbos)
			{
				if (std::holds_alternative<spk::OpenGL::UniformBufferObject>(objectUBO.second))
				{
					result._attributes[objectUBO.first] = std::get<spk::OpenGL::UniformBufferObject>(objectUBO.second);
				}
				else if (std::holds_alternative<spk::OpenGL::ShaderStorageBufferObject>(objectUBO.second))
				{
					result._attributes[objectUBO.first] = std::get<spk::OpenGL::ShaderStorageBufferObject>(objectUBO.second);
				}
			}

			for (auto& [index, type] : _objectLayoutAttributes)
			{
				result._bufferSet.layout().addAttribute(index, type);
			}
			return result;
		}

		bool contains(const std::wstring& p_name) const
		{
			std::lock_guard<std::mutex> lock(_mutex);

			return _constants.contains(p_name);
		}

		spk::OpenGL::UniformBufferObject& ubo(const std::wstring& p_name)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			if (_constants.find(p_name) == _constants.end())
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] not found");
			}
			if (!std::holds_alternative<spk::OpenGL::UniformBufferObject>(_constants[p_name]))
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] is not a UBO");
			}
			return std::get<spk::OpenGL::UniformBufferObject>(_constants[p_name]);
		}

		spk::OpenGL::ShaderStorageBufferObject& ssbo(const std::wstring& p_name)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			if (_constants.find(p_name) == _constants.end())
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] not found");
			}
			if (!std::holds_alternative<spk::OpenGL::ShaderStorageBufferObject>(_constants[p_name]))
			{
				throw std::runtime_error("Constant [" + spk::StringUtils::wstringToString(p_name) + "] is not a SSBO");
			}
			return std::get<spk::OpenGL::ShaderStorageBufferObject>(_constants[p_name]);
		}
	};
}
