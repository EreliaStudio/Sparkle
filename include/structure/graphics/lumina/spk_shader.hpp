#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <regex>

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
		using SafeUnit = std::variant<spk::SafePointer<OpenGL::SamplerObject>, spk::SafePointer<OpenGL::UniformBufferObject>,
									  spk::SafePointer<OpenGL::ShaderStorageBufferObject>>;

	public:
		enum class Mode
		{
			Constant,
			Attribute
		};

		template <typename Var>
		static void visitVariant(Var &variant, auto &&function)
		{
			std::visit([&](auto &object) { function(object); }, std::forward<Var>(variant));
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
				_bufferSet(p_originator->_bufferSet),
				_attributes(p_originator->_attributes)
			{
			}

			void activate()
			{
				_bufferSet.activate();

				for (const auto &[name, object] : _attributes)
				{
					visitVariant(object, [&](auto &object) { object.activate(); });
				}
			}

			void deactivate()
			{
				_bufferSet.deactivate();

				for (const auto &[name, object] : _attributes)
				{
					visitVariant(object, [&](auto &object) { object.deactivate(); });
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

				_originator->activate();
				activate();
				_originator->render(_bufferSet.indexes().size(), _nbInstance);
				deactivate();
				_originator->deactivate();
			}

			OpenGL::BufferSet &bufferSet()
			{
				return _bufferSet;
			}

			bool containSampler(const std::wstring &p_name) const
			{
				return (_originator->containSampler(p_name) ||
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
						throw std::runtime_error("Sampler with name '" + spk::StringUtils::wstringToString(p_name) + "' not found.");
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
				return (_originator->containUBO(p_name) || (_attributes.contains(p_name) == true &&
															std::holds_alternative<OpenGL::UniformBufferObject>(_attributes.at(p_name)) == true));
			}

			OpenGL::UniformBufferObject &UBO(const std::wstring &p_name)
			{
				if (_attributes.contains(p_name) == false)
				{
					try
					{
						return (_originator->UBO(p_name));
					} catch (...)
					{
						throw std::runtime_error("UBO with name '" + spk::StringUtils::wstringToString(p_name) + "' not found.");
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
				return (_originator->containSSBO(p_name) ||
						(_attributes.contains(p_name) == true &&
						 std::holds_alternative<OpenGL::ShaderStorageBufferObject>(_attributes.at(p_name)) == true));
			}

			OpenGL::ShaderStorageBufferObject &SSBO(const std::wstring &p_name)
			{
				if (_attributes.contains(p_name) == false)
				{
					try
					{
						return (_originator->SSBO(p_name));
					} catch (...)
					{
						throw std::runtime_error("SSBO with name '" + spk::StringUtils::wstringToString(p_name) + "' not found.");
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

		// Map holding all the constants for all the shaders
		static inline std::unordered_map<std::wstring, Unit> _objects;
		// Map holding the constant of this specific shader instance
		std::unordered_map<std::wstring, SafeUnit> _availableObjects;

		// Buffer set for this shader instance
		OpenGL::BufferSet _bufferSet;
		// Map holding the attributes of this shader instance
		std::unordered_map<std::wstring, Unit> _attributes;

		void activate()
		{
			_program.activate();

			for (const auto &[name, object] : _availableObjects)
			{
				visitVariant(object, [&](auto &object) { object->activate(); });
			}
		}

		void render(const size_t &nbIndexes, const size_t &nbInstances)
		{
			_program.render(nbIndexes, nbInstances);
		}

		void deactivate()
		{
			_program.deactivate();

			for (const auto &[name, object] : _availableObjects)
			{
				visitVariant(object, [&](auto &object) { object->deactivate(); });
			}
		}

		void addSamplerConstant(const std::wstring &p_name, OpenGL::SamplerObject &&p_object)
		{
			if (_objects.contains(p_name) == false)
			{
				_objects[p_name] = std::move(p_object);
			}
			else
			{
				if (std::holds_alternative<OpenGL::SamplerObject>(_objects[p_name]) == false)
				{
					throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) +
											 "' is already set but is not a sampler object.");
				}

				spk::SafePointer<OpenGL::SamplerObject> sampler = &(std::get<OpenGL::SamplerObject>(_objects[p_name]));

				// No size verification to compute on samplers
			}

			_availableObjects[p_name] = &(std::get<OpenGL::SamplerObject>(_objects[p_name]));
		}

		void addSamplerAttribute(const std::wstring &p_name, OpenGL::SamplerObject &&p_object)
		{
			if (_attributes.contains(p_name) == true)
			{
				if (std::holds_alternative<OpenGL::SamplerObject>(_attributes[p_name]) == false)
				{
					throw std::runtime_error("Attribute with name '" + spk::StringUtils::wstringToString(p_name) +
											 "' is already set but is not a sampler object.");
				}

				throw std::runtime_error("Sampler attribute with name '" + spk::StringUtils::wstringToString(p_name) + "' is already set.");
			}

			_attributes[p_name] = std::move(p_object);
		}

		void addUBOConstant(const std::wstring &p_name, OpenGL::UniformBufferObject &&p_object)
		{
			if (_objects.contains(p_name) == false)
			{
				_objects[p_name] = std::move(p_object);
			}
			else
			{
				if (std::holds_alternative<OpenGL::UniformBufferObject>(_objects[p_name]) == false)
				{
					throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) +
											 "' is already set but is not an UBO.");
				}

				spk::SafePointer<OpenGL::UniformBufferObject> ubo = &(std::get<OpenGL::UniformBufferObject>(_objects[p_name]));

				if (ubo->size() != p_object.size())
				{
					throw std::runtime_error("UBO size mismatch for '" + spk::StringUtils::wstringToString(p_name) +
											 "'. Expected: " + std::to_string(ubo->size()) + ", got: " + std::to_string(p_object.size()));
				}

				if (ubo->bindingPoint() != p_object.bindingPoint())
				{
					throw std::runtime_error("UBO binding point mismatch for '" + spk::StringUtils::wstringToString(p_name) + "'. Expected: " +
											 std::to_string(ubo->bindingPoint()) + ", got: " + std::to_string(p_object.bindingPoint()));
				}
			}

			_availableObjects[p_name] = &(std::get<OpenGL::UniformBufferObject>(_objects[p_name]));
		}

		void addUBOAttribute(const std::wstring &p_name, OpenGL::UniformBufferObject &&p_object)
		{
			if (_attributes.contains(p_name) == true)
			{
				if (std::holds_alternative<OpenGL::UniformBufferObject>(_attributes[p_name]) == false)
				{
					throw std::runtime_error("Attribute with name '" + spk::StringUtils::wstringToString(p_name) +
											 "' is already set but is not an UBO object.");
				}

				throw std::runtime_error("UBO attribute with name '" + spk::StringUtils::wstringToString(p_name) + "' is already set.");
			}

			_attributes[p_name] = std::move(p_object);
		}

		void addSSBOConstant(const std::wstring &p_name, OpenGL::ShaderStorageBufferObject &&p_object)
		{
			if (_objects.contains(p_name) == false)
			{
				_objects[p_name] = std::move(p_object);
			}
			else
			{
				if (std::holds_alternative<OpenGL::ShaderStorageBufferObject>(_objects[p_name]) == false)
				{
					throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) +
											 "' is already set but is not an SSBO.");
				}

				spk::SafePointer<OpenGL::ShaderStorageBufferObject> ssbo = &(std::get<OpenGL::ShaderStorageBufferObject>(_objects[p_name]));

				if (ssbo->fixedData().size() != p_object.fixedData().size())
				{
					throw std::runtime_error("SSBO fixed data size mismatch for '" + spk::StringUtils::wstringToString(p_name) + "'. Expected: " +
											 std::to_string(ssbo->fixedData().size()) + ", got: " + std::to_string(p_object.fixedData().size()));
				}

				if (ssbo->dynamicArray().elementSize() != p_object.dynamicArray().elementSize())
				{
					throw std::runtime_error("SSBO dynamic array element size mismatch for '" + spk::StringUtils::wstringToString(p_name) +
											 "'. Expected: " + std::to_string(ssbo->dynamicArray().elementSize()) +
											 ", got: " + std::to_string(p_object.dynamicArray().elementSize()));
				}

				if (ssbo->bindingPoint() != p_object.bindingPoint())
				{
					throw std::runtime_error("SSBO binding point mismatch for '" + spk::StringUtils::wstringToString(p_name) + "'. Expected: " +
											 std::to_string(ssbo->bindingPoint()) + ", got: " + std::to_string(p_object.bindingPoint()));
				}
			}

			_availableObjects[p_name] = &(std::get<OpenGL::ShaderStorageBufferObject>(_objects[p_name]));
		}

		void addSSBOAttribute(const std::wstring &p_name, OpenGL::ShaderStorageBufferObject &&p_object)
		{
			if (_attributes.contains(p_name) == true)
			{
				if (std::holds_alternative<OpenGL::ShaderStorageBufferObject>(_attributes[p_name]) == false)
				{
					throw std::runtime_error("Attribute with name '" + spk::StringUtils::wstringToString(p_name) +
											 "' is already set but is not an SSBO.");
				}

				throw std::runtime_error("SSBO attribute with name '" + spk::StringUtils::wstringToString(p_name) + "' is already set.");
			}

			_attributes[p_name] = std::move(p_object);
		}

		void addAttribute(const OpenGL::LayoutBufferObject::Attribute &p_attribute)
		{
			_bufferSet.layout().addAttribute(p_attribute);
		}

		void addAttribute(OpenGL::LayoutBufferObject::Attribute::Index p_index, OpenGL::LayoutBufferObject::Attribute::Type p_type)
		{
			_bufferSet.layout().addAttribute(p_index, p_type);
		}

		void addSampler(const std::wstring &p_name, OpenGL::SamplerObject &&p_object, Mode p_mode)
		{
			if (p_mode == Mode::Constant)
			{
				addSamplerConstant(p_name, std::move(p_object));
			}
			else
			{
				addSamplerAttribute(p_name, std::move(p_object));
			}
		}

		void addUBO(const std::wstring &p_name, OpenGL::UniformBufferObject &&p_object, Mode p_mode)
		{
			if (p_mode == Mode::Constant)
			{
				addUBOConstant(p_name, std::move(p_object));
			}
			else
			{
				addUBOAttribute(p_name, std::move(p_object));
			}
		}

		void addSSBO(const std::wstring &p_name, OpenGL::ShaderStorageBufferObject &&p_object, Mode p_mode)
		{
			if (p_mode == Mode::Constant)
			{
				addSSBOConstant(p_name, std::move(p_object));
			}
			else
			{
				addSSBOAttribute(p_name, std::move(p_object));
			}
		}

		void parseBufferSet(const std::string &p_code)
		{
			/*
				Objective of the function : search for all layout(location = X) in etc etc, than construct the LayoutBufferObject::Attribute
				corresponding, saving it inside the shader
			*/
		}

		void parseBufferObjectConstant(std::string& p_code)
		{
			/*
				Objective of the function : search all the instance of "layout(... constant ...) [uniform or buffer] TypeName {} name;" and swap "constant" with
				"binding = XXX", with X equal to either _objects.size() or equal to _objects[name].bindingPoint().

				The idea is to remplace the "constant" custom keyword by a valid binding point, equal to a previously set UBO or SSBO with the same name,
				or with a newly allocated binding point.
				Once swapped, i need the function to parse it, and save the constant inside the Shader, parsing its values etc

				This way, the user wont have to define its binding point itself, and therefore just describe the constant in layout UBO and SSBO
			*/
		}

		void parseBufferObjectAttribute(std::string& p_code)
		{
			/*
				Objective of the function : search all the instance of "layout(... attribute ...) [uniform or buffer] TypeName {} name;" and swap "attribute" with
				"binding = XXX", with X equal to either _objects.size() or equal to _objects[name].bindingPoint().

				The idea is to remplace the "attribute" custom keyword by a valid binding point, equal to a previously set UBO or SSBO with the same name,
				or with a newly allocated binding point.
				Once swapped, i need the function to parse it, and save the attribute inside the Shader, parsing its values etc

				This way, the user wont have to define its binding point itself, and therefore just describe the attribute in layout UBO and SSBO.
			*/
		}

		void parseSampler(std::string &p_code)
		{
			/*
				Objective of the function : search all the instance of "sampler2D name as constant;" and save the texture as a SamplerObject in the shader as constant.
				After parsing the constant, it need to search for instance of "sampler2D name as attribute;" and save them as SamplerObject attribute this time
			*/
		}

	public:
		Shader() = default;

		Shader(std::string p_vertexCode, std::string p_fragmentCode)
		{
			parseBufferSet(p_vertexCode);

			parseBufferObjectConstant(p_vertexCode);
			parseBufferObjectConstant(p_fragmentCode);

			parseBufferObjectAttribute(p_vertexCode);
			parseBufferObjectAttribute(p_fragmentCode);

			parseSampler(p_vertexCode);
			parseSampler(p_fragmentCode);

			_program = spk::OpenGL::Program(p_vertexCode, p_fragmentCode);
		}

		bool containSampler(const std::wstring &p_name) const
		{
			return (_availableObjects.contains(p_name) == true &&
					std::holds_alternative<spk::SafePointer<OpenGL::SamplerObject>>(_availableObjects.at(p_name)) == true);
		}

		OpenGL::SamplerObject &sampler(const std::wstring &p_name)
		{
			if (_availableObjects.contains(p_name) == false)
			{
				throw std::runtime_error("Sampler with name '" + spk::StringUtils::wstringToString(p_name) + "' not found.");
			}
			if (std::holds_alternative<spk::SafePointer<OpenGL::SamplerObject>>(_availableObjects.at(p_name)) == false)
			{
				throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is not a sampler object.");
			}
			return *(std::get<spk::SafePointer<OpenGL::SamplerObject>>(_availableObjects.at(p_name)));
		}

		bool containUBO(const std::wstring &p_name) const
		{
			return (_availableObjects.contains(p_name) == true &&
					std::holds_alternative<spk::SafePointer<OpenGL::UniformBufferObject>>(_availableObjects.at(p_name)) == true);
		}

		OpenGL::UniformBufferObject &UBO(const std::wstring &p_name)
		{
			if (_availableObjects.contains(p_name) == false)
			{
				throw std::runtime_error("UBO with name '" + spk::StringUtils::wstringToString(p_name) + "' not found.");
			}
			if (std::holds_alternative<spk::SafePointer<OpenGL::UniformBufferObject>>(_availableObjects.at(p_name)) == false)
			{
				throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is not a UBO.");
			}
			return *(std::get<spk::SafePointer<OpenGL::UniformBufferObject>>(_availableObjects.at(p_name)));
		}

		bool containSSBO(const std::wstring &p_name) const
		{
			return (_availableObjects.contains(p_name) == true &&
					std::holds_alternative<spk::SafePointer<OpenGL::ShaderStorageBufferObject>>(_availableObjects.at(p_name)) == true);
		}

		OpenGL::ShaderStorageBufferObject &SSBO(const std::wstring &p_name)
		{
			if (_availableObjects.contains(p_name) == false)
			{
				throw std::runtime_error("SSBO with name '" + spk::StringUtils::wstringToString(p_name) + "' not found.");
			}
			if (std::holds_alternative<spk::SafePointer<OpenGL::ShaderStorageBufferObject>>(_availableObjects.at(p_name)) == false)
			{
				throw std::runtime_error("Object with name '" + spk::StringUtils::wstringToString(p_name) + "' is not an SSBO.");
			}
			return *(std::get<spk::SafePointer<OpenGL::ShaderStorageBufferObject>>(_availableObjects.at(p_name)));
		}

		Object createObject()
		{
			return (Object(this));
		}
	};
}