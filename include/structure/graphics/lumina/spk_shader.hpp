#pragma once

#include <regex>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"
#include "structure/graphics/opengl/spk_shader_storage_buffer_object.hpp"
#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"

namespace spk::Lumina
{
	class Shader
	{
	private:
		using Unit = std::variant<OpenGL::SamplerObject, OpenGL::UniformBufferObject, OpenGL::ShaderStorageBufferObject>;

	public:
		enum class Mode
		{
			Constant,
			Attribute
		};

		template <typename Var, typename F>
		static void visitVariant(Var &p_variant, F &&p_function)
		{
			std::visit([&](auto &&p_object) { p_function(p_object); }, p_variant);
		}

		class Object
		{
			friend class Shader;

		private:
			Shader *_originator = nullptr;

			OpenGL::BufferSet _bufferSet;
			size_t _nbInstance = 1;
			std::unordered_map<std::wstring, Shader::Unit> _attributes;

			Object(Shader *p_originator) :
				_originator(p_originator),
				_bufferSet(p_originator->_bufferSet)
			{
				for (const auto &[name, attribute] : p_originator->_attributes)
				{
					_attributes[name] = attribute;
				}
			}

			void _activate()
			{
				_bufferSet.activate();

				for (auto &[name, object] : _attributes)
				{
					visitVariant(object, [&](auto &p_object) { p_object.activate(); });
				}
			}

			void _deactivate()
			{
				_bufferSet.deactivate();

				for (auto &[name, object] : _attributes)
				{
					visitVariant(object, [&](auto &p_object) { p_object.deactivate(); });
				}
			}

		public:
			Object() = default;

			void setNbInstance(const size_t &p_nbInstance)
			{
				_nbInstance = p_nbInstance;
			}

			void render()
			{
				if (_originator == nullptr)
				{
					throw std::runtime_error("Can't render an Object not constructed by a Shader instance");
				}

				_originator->_activate();
				_activate();
				_originator->_render(_bufferSet.indexes().nbIndexes(), _nbInstance);
				_deactivate();
				_originator->_deactivate();
			}

			OpenGL::BufferSet &bufferSet()
			{
				return _bufferSet;
			}

			bool containSampler(const std::wstring &p_name) const
			{
				return (
					_originator->containSampler(p_name) ||
					(_attributes.contains(p_name) == true && std::holds_alternative<OpenGL::SamplerObject>(_attributes.at(p_name)) == true));
			}

			OpenGL::SamplerObject &sampler(const std::wstring &p_name)
			{
				if (_attributes.contains(p_name) == false)
				{
					try
					{
						return (_originator->sampler(p_name));
					} catch (...)
					{
						throw std::runtime_error(
							"Sampler with name '" + spk::StringUtils::wstringToString(p_name) +
							"' not found in spk::Lumina::Shader constants or object attributes.");
					}
				}
				if (std::holds_alternative<OpenGL::SamplerObject>(_attributes.at(p_name)) == false)
				{
					throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is not a sampler object.");
				}
				return std::get<OpenGL::SamplerObject>(_attributes.at(p_name));
			}

			bool containUBO(const std::wstring &p_name) const
			{
				return (
					_originator->containUBO(p_name) ||
					(_attributes.contains(p_name) == true && std::holds_alternative<OpenGL::UniformBufferObject>(_attributes.at(p_name)) == true));
			}

			OpenGL::UniformBufferObject &ubo(const std::wstring &p_name)
			{
				if (_attributes.contains(p_name) == false)
				{
					try
					{
						return (_originator->ubo(p_name));
					} catch (...)
					{
						throw std::runtime_error(
							"UBO with name '" + spk::StringUtils::wstringToString(p_name) +
							"' not found in spk::Lumina::Shader constants or object attributes.");
					}
				}
				if (std::holds_alternative<OpenGL::UniformBufferObject>(_attributes.at(p_name)) == false)
				{
					throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is not a UBO.");
				}
				return std::get<OpenGL::UniformBufferObject>(_attributes.at(p_name));
			}

			bool containSSBO(const std::wstring &p_name) const
			{
				return (
					_originator->containSSBO(p_name) || (_attributes.contains(p_name) == true &&
														 std::holds_alternative<OpenGL::ShaderStorageBufferObject>(_attributes.at(p_name)) == true));
			}

			OpenGL::ShaderStorageBufferObject &ssbo(const std::wstring &p_name)
			{
				if (_attributes.contains(p_name) == false)
				{
					try
					{
						return (_originator->ssbo(p_name));
					} catch (...)
					{
						throw std::runtime_error(
							"SSBO with name '" + spk::StringUtils::wstringToString(p_name) +
							"' not found in spk::Lumina::Shader constants or object attributes.");
					}
				}
				if (std::holds_alternative<OpenGL::ShaderStorageBufferObject>(_attributes.at(p_name)) == false)
				{
					throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is not an SSBO.");
				}
				return std::get<OpenGL::ShaderStorageBufferObject>(_attributes.at(p_name));
			}
		};

	private:
		OpenGL::Program _program;

		static inline std::unordered_map<std::wstring, Unit> _objects;
		std::unordered_map<std::wstring, Unit> _availableObjects;

		OpenGL::BufferSet _bufferSet;
		std::unordered_map<std::wstring, Unit> _attributes;

		void _activate()
		{
			_program.activate();

			for (auto &[name, object] : _availableObjects)
			{
				visitVariant(object, [&](auto &p_object) { p_object.activate(); });
			}
		}

		void _render(const size_t &p_nbIndexes, const size_t &p_nbInstances)
		{
			_program.render(p_nbIndexes, p_nbInstances);
		}

		void _renderIndirect(const size_t &p_commandOffset, const size_t &p_drawCount, const size_t &p_stride)
		{
			_program.renderIndirect(p_commandOffset, p_drawCount, p_stride);
		}

		void _deactivate()
		{
			_program.deactivate();

			for (auto &[name, object] : _availableObjects)
			{
				visitVariant(object, [&](auto &p_object) { p_object.deactivate(); });
			}
		}

		void _addSamplerConstant(const std::wstring &p_name, const OpenGL::SamplerObject &p_object)
		{
			if (_objects.contains(p_name) == false)
			{
				_objects[p_name] = p_object;
			}
			else
			{
				if (std::holds_alternative<OpenGL::SamplerObject>(_objects[p_name]) == false)
				{
					throw std::runtime_error(
						"Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is already set but is not a sampler object.");
				}

				spk::SafePointer<OpenGL::SamplerObject> sampler = &(std::get<OpenGL::SamplerObject>(_objects[p_name]));

				// No size verification to compute on samplers
			}

			_availableObjects[p_name] = std::get<OpenGL::SamplerObject>(_objects[p_name]);
		}

		void _addSamplerAttribute(const std::wstring &p_name, const OpenGL::SamplerObject &p_object)
		{
			if (_attributes.contains(p_name) == true)
			{
				if (std::holds_alternative<OpenGL::SamplerObject>(_attributes[p_name]) == false)
				{
					throw std::runtime_error(
						"Attribute with name '" + spk::StringUtils::wstringToString(p_name) + "' is already set but is not a sampler object.");
				}

				throw std::runtime_error("Sampler attribute with name '" + spk::StringUtils::wstringToString(p_name) + "' is already set.");
			}

			_attributes[p_name] = p_object;
		}

		void _addUboConstant(const std::wstring &p_name, const OpenGL::UniformBufferObject &p_object)
		{
			if (_objects.contains(p_name) == false)
			{
				_objects[p_name] = p_object;
			}
			else
			{
				if (std::holds_alternative<OpenGL::UniformBufferObject>(_objects[p_name]) == false)
				{
					throw std::runtime_error(
						"Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is already set but is not an UBO.");
				}

				OpenGL::UniformBufferObject &ubo = std::get<OpenGL::UniformBufferObject>(_objects[p_name]);

				if (ubo.size() != p_object.size())
				{
					throw std::runtime_error(
						"UBO size mismatch for '" + spk::StringUtils::wstringToString(p_name) + "'. Expected: " + std::to_string(ubo.size()) +
						", got: " + std::to_string(p_object.size()));
				}

				if (ubo.bindingPoint() != p_object.bindingPoint())
				{
					throw std::runtime_error(
						"UBO binding point mismatch for '" + spk::StringUtils::wstringToString(p_name) +
						"'. Expected: " + std::to_string(ubo.bindingPoint()) + ", got: " + std::to_string(p_object.bindingPoint()));
				}
			}

			_availableObjects[p_name] = std::get<OpenGL::UniformBufferObject>(_objects[p_name]);
		}

		void _addUboAttribute(const std::wstring &p_name, const OpenGL::UniformBufferObject &p_object)
		{
			if (_attributes.contains(p_name) == true)
			{
				if (std::holds_alternative<OpenGL::UniformBufferObject>(_attributes[p_name]) == false)
				{
					throw std::runtime_error(
						"Attribute with name '" + spk::StringUtils::wstringToString(p_name) + "' is already set but is not an UBO object.");
				}

				throw std::runtime_error("UBO attribute with name '" + spk::StringUtils::wstringToString(p_name) + "' is already set.");
			}

			_attributes[p_name] = p_object;
		}

		void _addSsboConstant(const std::wstring &p_name, const OpenGL::ShaderStorageBufferObject &p_object)
		{
			if (_objects.contains(p_name) == false)
			{
				_objects[p_name] = p_object;
			}
			else
			{
				if (std::holds_alternative<OpenGL::ShaderStorageBufferObject>(_objects[p_name]) == false)
				{
					throw std::runtime_error(
						"Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is already set but is not an SSBO.");
				}

				OpenGL::ShaderStorageBufferObject &ssbo = std::get<OpenGL::ShaderStorageBufferObject>(_objects[p_name]);

				if (ssbo.fixedData().size() != p_object.fixedData().size())
				{
					throw std::runtime_error(
						"SSBO fixed data size mismatch for '" + spk::StringUtils::wstringToString(p_name) +
						"'. Expected: " + std::to_string(ssbo.fixedData().size()) + ", got: " + std::to_string(p_object.fixedData().size()));
				}

				if (ssbo.dynamicArray().elementSize() != p_object.dynamicArray().elementSize())
				{
					throw std::runtime_error(
						"SSBO dynamic array element size mismatch for '" + spk::StringUtils::wstringToString(p_name) + "'. Expected: " +
						std::to_string(ssbo.dynamicArray().elementSize()) + ", got: " + std::to_string(p_object.dynamicArray().elementSize()));
				}

				if (ssbo.bindingPoint() != p_object.bindingPoint())
				{
					throw std::runtime_error(
						"SSBO binding point mismatch for '" + spk::StringUtils::wstringToString(p_name) +
						"'. Expected: " + std::to_string(ssbo.bindingPoint()) + ", got: " + std::to_string(p_object.bindingPoint()));
				}
			}

			_availableObjects[p_name] = std::get<OpenGL::ShaderStorageBufferObject>(_objects[p_name]);
		}

		void _addSsboAttribute(const std::wstring &p_name, const OpenGL::ShaderStorageBufferObject &p_object)
		{
			if (_attributes.contains(p_name) == true)
			{
				if (std::holds_alternative<OpenGL::ShaderStorageBufferObject>(_attributes[p_name]) == false)
				{
					throw std::runtime_error(
						"Attribute with name '" + spk::StringUtils::wstringToString(p_name) + "' is already set but is not an SSBO.");
				}

				throw std::runtime_error("SSBO attribute with name '" + spk::StringUtils::wstringToString(p_name) + "' is already set.");
			}

			_attributes[p_name] = p_object;
		}

	public:
		Shader() = default;

		Shader(std::string p_vertexCode, std::string p_fragmentCode) :
			_program(p_vertexCode, p_fragmentCode)
		{
		}

		void setVerboseMode(bool p_verbose = true)
		{
			_program.setVerboseMode(p_verbose);
		}

		void addAttribute(const OpenGL::LayoutBufferObject::Attribute &p_attribute)
		{
			_bufferSet.layout().addAttribute(p_attribute);
		}

		void addAttribute(OpenGL::LayoutBufferObject::Attribute::Index p_index, OpenGL::LayoutBufferObject::Attribute::Type p_type)
		{
			_bufferSet.layout().addAttribute(p_index, p_type);
		}

		void addSampler(const std::wstring &p_name, const OpenGL::SamplerObject &p_object, Mode p_mode)
		{
			if (p_mode == Mode::Constant)
			{
				_addSamplerConstant(p_name, p_object);
			}
			else
			{
				_addSamplerAttribute(p_name, p_object);
			}
		}

		void addUBO(const std::wstring &p_name, const OpenGL::UniformBufferObject &p_object, Mode p_mode)
		{
			if (p_mode == Mode::Constant)
			{
				_addUboConstant(p_name, p_object);
			}
			else
			{
				_addUboAttribute(p_name, p_object);
			}
		}

		void addSSBO(const std::wstring &p_name, const OpenGL::ShaderStorageBufferObject &p_object, Mode p_mode)
		{
			if (p_mode == Mode::Constant)
			{
				_addSsboConstant(p_name, p_object);
			}
			else
			{
				_addSsboAttribute(p_name, p_object);
			}
		}

		bool containSampler(const std::wstring &p_name) const
		{
			return (
				_availableObjects.contains(p_name) == true && std::holds_alternative<OpenGL::SamplerObject>(_availableObjects.at(p_name)) == true);
		}

		OpenGL::SamplerObject &sampler(const std::wstring &p_name)
		{
			if (_availableObjects.contains(p_name) == false)
			{
				throw std::runtime_error(
					"Sampler with name '" + spk::StringUtils::wstringToString(p_name) + "' not found in spk::Lumina::Shader constants.");
			}
			if (std::holds_alternative<OpenGL::SamplerObject>(_availableObjects.at(p_name)) == false)
			{
				throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is not a sampler object.");
			}
			return std::get<OpenGL::SamplerObject>(_availableObjects.at(p_name));
		}

		bool containUBO(const std::wstring &p_name) const
		{
			return (
				_availableObjects.contains(p_name) == true &&
				std::holds_alternative<OpenGL::UniformBufferObject>(_availableObjects.at(p_name)) == true);
		}

		OpenGL::UniformBufferObject &ubo(const std::wstring &p_name)
		{
			if (_availableObjects.contains(p_name) == false)
			{
				throw std::runtime_error(
					"UBO with name '" + spk::StringUtils::wstringToString(p_name) + "' not found in spk::Lumina::Shader constants.");
			}
			if (std::holds_alternative<OpenGL::UniformBufferObject>(_availableObjects.at(p_name)) == false)
			{
				throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is not a UBO.");
			}
			return std::get<OpenGL::UniformBufferObject>(_availableObjects.at(p_name));
		}

		bool containSSBO(const std::wstring &p_name) const
		{
			return (
				_availableObjects.contains(p_name) == true &&
				std::holds_alternative<OpenGL::ShaderStorageBufferObject>(_availableObjects.at(p_name)) == true);
		}

		OpenGL::ShaderStorageBufferObject &ssbo(const std::wstring &p_name)
		{
			if (_availableObjects.contains(p_name) == false)
			{
				throw std::runtime_error(
					"SSBO with name '" + spk::StringUtils::wstringToString(p_name) + "' not found in spk::Lumina::Shader constants.");
			}
			if (std::holds_alternative<OpenGL::ShaderStorageBufferObject>(_availableObjects.at(p_name)) == false)
			{
				throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is not an SSBO.");
			}
			return std::get<OpenGL::ShaderStorageBufferObject>(_availableObjects.at(p_name));
		}

		Object createObject()
		{
			return (Object(this));
		}

		struct Constants
		{
			static void addSampler(const std::wstring &p_name, const OpenGL::SamplerObject &p_object)
			{
				if (_objects.contains(p_name) == false)
				{
					_objects[p_name] = p_object;
				}
			}

			static void addUBO(const std::wstring &p_name, const OpenGL::UniformBufferObject &p_object)
			{
				if (_objects.contains(p_name) == false)
				{
					_objects[p_name] = p_object;
				}
			}

			static void addSSBO(const std::wstring &p_name, const OpenGL::ShaderStorageBufferObject &p_object)
			{
				if (_objects.contains(p_name) == false)
				{
					_objects[p_name] = p_object;
				}
			}

			static bool containsSampler(const std::wstring &p_name)
			{
				return (
					Shader::_objects.contains(p_name) == true && std::holds_alternative<OpenGL::SamplerObject>(Shader::_objects.at(p_name)) == true);
			}

			static bool containsUBO(const std::wstring &p_name)
			{
				return (
					Shader::_objects.contains(p_name) == true &&
					std::holds_alternative<OpenGL::UniformBufferObject>(Shader::_objects.at(p_name)) == true);
			}

			static bool containsSSBO(const std::wstring &p_name)
			{
				return (
					Shader::_objects.contains(p_name) == true &&
					std::holds_alternative<OpenGL::ShaderStorageBufferObject>(Shader::_objects.at(p_name)) == true);
			}

			static OpenGL::SamplerObject &sampler(const std::wstring &p_name)
			{
				if (Shader::_objects.contains(p_name) == false)
				{
					throw std::runtime_error("Sampler with name '" + spk::StringUtils::wstringToString(p_name) + "' not found.");
				}
				if (std::holds_alternative<OpenGL::SamplerObject>(Shader::_objects[p_name]) == false)
				{
					throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is not an Sampler.");
				}
				return (std::get<OpenGL::SamplerObject>(Shader::_objects[p_name]));
			}

			static OpenGL::UniformBufferObject &ubo(const std::wstring &p_name)
			{
				if (Shader::_objects.contains(p_name) == false)
				{
					throw std::runtime_error("UBO with name '" + spk::StringUtils::wstringToString(p_name) + "' not found.");
				}
				if (std::holds_alternative<OpenGL::UniformBufferObject>(Shader::_objects[p_name]) == false)
				{
					throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is not an UBO.");
				}
				return (std::get<OpenGL::UniformBufferObject>(Shader::_objects[p_name]));
			}

			static OpenGL::ShaderStorageBufferObject &ssbo(const std::wstring &p_name)
			{
				if (Shader::_objects.contains(p_name) == false)
				{
					throw std::runtime_error("SSBO with name '" + spk::StringUtils::wstringToString(p_name) + "' not found.");
				}
				if (std::holds_alternative<OpenGL::ShaderStorageBufferObject>(Shader::_objects[p_name]) == false)
				{
					throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is not an SSBO.");
				}
				return (std::get<OpenGL::ShaderStorageBufferObject>(Shader::_objects[p_name]));
			}
		};
	};
}