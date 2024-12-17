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

namespace spk::OpenGL
{
    class UniformBufferObject : public VertexBufferObject
    {
    public:
        using BindingPoint = int;

        class Element
        {
			friend class UniformBufferObject;
			
        private:
            uint8_t* _buffer;
            size_t _size;
            std::variant<std::vector<Element>, std::unordered_map<std::wstring, Element>, std::monostate> _content;

        public:
            Element(uint8_t* buffer = nullptr, size_t size = 0);

			template <typename TType>
			requires (!spk::IsContainer<TType>::value)
			Element& operator = (const TType& value)
			{
				if (sizeof(TType) != _size)
					throw std::runtime_error("Size mismatch: Expected size " + std::to_string(_size) +
											" bytes, but received " + std::to_string(sizeof(TType)) + " bytes.");

				if (std::holds_alternative<std::vector<Element>>(_content))
					throw std::runtime_error("Cannot apply scalar value to an array.");

				std::memcpy(_buffer, &value, sizeof(TType));
				return *this;
			}

			template <typename TContainer>
			requires (spk::IsContainer<TContainer>::value)
			Element& operator = (const TContainer& values)
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
            size_t size() const;

            Element& operator[](size_t index);
            const Element& operator[](size_t index) const;

            void resizeArray(size_t arraySize, size_t elementSize);

            Element& addElement(const std::wstring& name, uint8_t* buffer, size_t elementSize, size_t arraySize = 0);
            Element& operator[](const std::wstring& name);
        };

    private:
        std::string _typeName;
        BindingPoint _bindingPoint;
        size_t _blockSize;
        std::unordered_map<std::wstring, Element> _elements;

    public:
        UniformBufferObject(const std::string& typeName, BindingPoint bindingPoint, size_t p_size);
        ~UniformBufferObject();

        void activate();

		template<typename TType>
		UniformBufferObject& operator = (const TType& p_data)
		{
			if (sizeof(p_data) != _blockSize)
			{
                throw std::runtime_error("Invalide data size passed to UniformBufferObject.");
			}
			edit(&p_data, _blockSize, 0);
			return (*this);
		}

        Element& operator[](const std::wstring& name);
        const Element& operator[](const std::wstring& name) const;

        Element& addElement(const std::wstring& name, size_t offset, size_t elementSize, size_t arraySize = 0); // Add a top-level element
    };
}
