#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <cstring>

#include "spk_sfinae.hpp"
#include "structure/graphics/opengl/spk_vertex_buffer_object.hpp"

#include "iostream"

#include "spk_debug_macro.hpp"

namespace spk::OpenGL
{
    class BindedBufferObject : public VertexBufferObject
    {
    public:
        using BindingPoint = int;

		struct Element
		{
			uint8_t* buffer = nullptr;
			size_t size;

			using Offset = size_t;
			using Structure = std::unordered_map<std::string, std::pair<Offset, Element>>;
			using Array = std::vector<Element>;

			std::variant<std::monostate, Structure, Array> innerElements;

			Element() = default;
			Element(uint8_t* p_buffer, size_t p_size) : buffer(p_buffer), size(p_size) {}

			Element(const Element& p_other) : buffer(p_other.buffer), size(p_other.size) {}
			Element(Element&& p_other) : buffer(p_other.buffer), size(p_other.size)
			{
				p_other.buffer = nullptr;
				p_other.size = 0;
			}

			Element& operator = (const Element& p_other)
			{
				if (this != &p_other)
				{
					buffer = p_other.buffer;
					size = p_other.size;
				}
				return (*this);
			}

			Element& operator = (Element&& p_other)
			{
				if (this != &p_other)
				{
					buffer = p_other.buffer;
					size = p_other.size;

					p_other.buffer = nullptr;
					p_other.size = 0;
				}
				return (*this);
			}

			Element& addElement(const std::string& p_name, size_t p_offset, size_t p_size)
			{
				if (std::holds_alternative<std::monostate>(innerElements) == true)
				{
					innerElements = Structure();
				}
				if (std::holds_alternative<Array>(innerElements) == true)
				{
					throw std::runtime_error("Can't add a nested data inside an array element");
				}
				std::get<Structure>(innerElements)[p_name] = {p_offset, {buffer + p_offset, p_size}};
			}

			void assign(const uint8_t* p_rawElement, size_t p_size)
			{
				if (std::holds_alternative<Structure>(innerElements) == true)
				{
					if (p_size != size)
					{
						auto& structure = std::get<Structure>(innerElements);
						for (auto& [name, pair] : structure)
						{
							pair.second.assign(p_rawElement + pair.first, pair.second.size);
						}
					}
					else
					{
						std::memcpy(buffer, p_rawElement, size);
					}
				}
				else if (std::holds_alternative<Array>(innerElements) == true)
				{
					auto& array = std::get<Array>(innerElements);
					for (auto& element : array)
					{
						element.assign(p_rawElement + element.size, element.size);
					}
				}
				else
				{
					if (p_size != size)
					{
						throw std::runtime_error("Invalid size passed to Element: Expected [" +
												std::to_string(size) + "] bytes but received [" +
												std::to_string(p_size) + "]");
					}
					
					std::memcpy(buffer, p_rawElement, size);
				}
			}

			template<typename TType>
			Element& operator = (const TType& p_value)
			{
				if (std::holds_alternative<Array>(innerElements) == true)
				{
					throw std::runtime_error("Can't assign an array element");
				}

				assign(&p_value, sizeof(TType));
			}

			template<typename TType>
			TType get() const
			{
				TType result;

				/* i want to feed the result with the content of each elements buffer, iterating
				thought all the children to set the data inside result, treating it as a uint8_t
				to export data from the buffer to a "readable data" for the user */

				return (result);
			}

			Element& operator[](const std::string& p_innerElementsName)
			{
				if (std::holds_alternative<Structure>(innerElements) == false)
				{
					throw std::runtime_error("Can't access inner element of a non-structured data");
				}
				if (std::get<Structure>(innerElements).contains(p_innerElementsName) == false)
				{
					throw std::runtime_error("Inner data named [" + p_innerElementsName + "] does not exist");
				}
				return (std::get<Structure>(innerElements).at(p_innerElementsName).second);
			}

			const Element& operator[](const std::string& p_innerElementsName) const
			{
				if (std::holds_alternative<Structure>(innerElements) == false)
				{
					throw std::runtime_error("Can't access inner element of a non-structured data");
				}
				if (std::get<Structure>(innerElements).contains(p_innerElementsName) == false)
				{
					throw std::runtime_error("Inner data named [" + p_innerElementsName + "] does not exist");
				}
				return (std::get<Structure>(innerElements).at(p_innerElementsName).second);
			}

			Element& operator[](size_t p_index)
			{
				if (std::holds_alternative<Array>(innerElements) == false)
				{
					throw std::runtime_error("Can't access element index [" + std::to_string(p_index) + "] of a non-array data");
				}
				if (std::get<Array>(innerElements).size() <= p_index)
				{
					throw std::out_of_range("Out of range access to inner data array");
				}
				return (std::get<Array>(innerElements).at(p_index));
			}

			const Element& operator[](size_t p_index) const
			{
				if (std::holds_alternative<Array>(innerElements) == false)
				{
					throw std::runtime_error("Can't access element index [" + std::to_string(p_index) + "] of a non-array data");
				}
				if (std::get<Array>(innerElements).size() <= p_index)
				{
					throw std::out_of_range("Out of range access to inner data array");
				}
				return (std::get<Array>(innerElements).at(p_index));
			}
		};

    protected:
		std::string _blockName ="Unnamed binded buffer object";
        BindingPoint _bindingPoint = -1;

		Element::Structure _elements;

    public:
		BindedBufferObject() = default;
        
		BindedBufferObject(Type p_type, Usage p_usage, const std::string& p_blockName, BindingPoint p_bindingPoint, size_t p_size) :
			_blockName(p_blockName),
			_bindingPoint(p_bindingPoint)
		{
			resize(p_size);
		}

        virtual ~BindedBufferObject() = default;

		BindedBufferObject(const BindedBufferObject& p_other) :
			_blockName(p_other._blockName),
			_bindingPoint(p_other._bindingPoint)
		{

		}

		BindedBufferObject(BindedBufferObject&& p_other) :
			_blockName(p_other._blockName),
			_bindingPoint(p_other._bindingPoint)
		{

		}

		const std::string& blockName() const
		{
			return (_blockName);
		}
		void setBlockName(const std::string& p_blockName)
		{
			_blockName = p_blockName;
		}

		BindingPoint bindingPoint() const
		{
			return (_bindingPoint);
		}
		void setBindingPoint(BindingPoint p_bindingPoint)
		{
			_bindingPoint = p_bindingPoint;
		}

		Element& addElement(const std::string& p_name, size_t p_offset, size_t p_size)
		{
			_elements[p_name] = {p_offset, {data(), p_size}};
		}

		BindedBufferObject& operator = (const BindedBufferObject& p_other)
		{
			if (this != &p_other)
			{
				_blockName = p_other._blockName;
				_blockName = p_other._bindingPoint;
			}
			return (*this);
		}
		
		BindedBufferObject& operator = (BindedBufferObject&& p_other)
		{
			if (this != &p_other)
			{
				_blockName = p_other._blockName;
				_blockName = p_other._bindingPoint;
			}
			return (*this);
		}

		virtual void activate()
		{
			VertexBufferObject::activate();
		}
    };
}
