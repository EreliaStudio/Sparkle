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

namespace spk::OpenGL
{
    class BindedBufferObject : public VertexBufferObject
    {
    public:
        using BindingPoint = int;

        class Element
        {
            friend class BindedBufferObject;
            friend class ShaderStorageBufferObject;

        private:
            uint8_t* _buffer;
            size_t _size;
            std::variant<std::monostate, std::vector<Element>, std::unordered_map<std::string, Element>> _content;

        public:
            Element(uint8_t* buffer = nullptr, size_t size = 0);

            template <typename TType>
            requires (!spk::IsContainer<TType>::value)
            Element& operator=(const TType& value)
            {
                if (std::holds_alternative<std::vector<Element>>(_content))
				{
					if (sizeof(value) == _size * std::get<std::vector<Element>>(_content).size())
					{
                		std::memcpy(_buffer, &value, sizeof(TType));
						return *this;
					}
					else
					{
                    	throw std::runtime_error("Cannot apply scalar value to an array.");
					}
				}

                if (sizeof(TType) != _size)
                    throw std::runtime_error("Size mismatch: Expected " + std::to_string(_size) +
                                             " bytes, but got " + std::to_string(sizeof(TType)) + " bytes.");

                std::memcpy(_buffer, &value, sizeof(TType));
                return *this;
            }

            template <typename TContainer>
            requires (spk::IsContainer<TContainer>::value)
            Element& operator=(const TContainer& values)
            {
                if (!std::holds_alternative<std::vector<Element>>(_content))
                    throw std::runtime_error("Cannot apply container to a non-array element.");

                auto& elements = std::get<std::vector<Element>>(_content);

                if (values.size() > elements.size())
                    throw std::runtime_error("Container size exceeds array size.");

                auto it = values.begin();
                for (size_t i = 0; i < values.size(); ++i, ++it)
                {
                    elements[i] = *it;
                }

                return *this;
            }

            uint8_t* data();
            const uint8_t* data() const;
            size_t size() const;

            size_t nbElement() const;

            Element& operator[](size_t index);
            const Element& operator[](size_t index) const;

            Element& operator[](const std::string& name);

            const Element& operator[](const std::string& name) const;
        };

    protected:
		std::string _name;
        BindingPoint _bindingPoint;
		size_t _blockSize;
        std::unordered_map<std::string, Element> _elements;

    public:
		BindedBufferObject() = default;
        BindedBufferObject(Type p_type, Usage p_usage, const std::string& typeName, BindingPoint bindingPoint, size_t p_size);
        virtual ~BindedBufferObject() = default;

        virtual void activate();

        template<typename TType>
        BindedBufferObject& operator=(const TType& p_data)
        {
            if (sizeof(p_data) != _blockSize)
            {
                throw std::runtime_error("Invalid data size passed to ShaderStorageBufferObject.");
            }
            edit(&p_data, _blockSize, 0);
            return (*this);
        }

        Element& operator[](const std::string& name);
        const Element& operator[](const std::string& name) const;

        Element& addElement(const std::string& name, size_t offset, size_t elementSize);
        Element& addElement(const std::string& name, size_t offset, size_t elementSize, size_t arraySize, size_t arrayPadding);
    };
}
