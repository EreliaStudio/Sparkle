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
            uint8_t* _buffer; // Pointer to the UBO's data buffer
            size_t _size;     // Size of a single element
            std::variant<std::vector<Element>, std::unordered_map<std::wstring, Element>, std::monostate> _content; // Arrays or no additional content

        public:
            Element(uint8_t* buffer = nullptr, size_t size = 0);

            // Templated apply for non-container types
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

			// Templated apply for container types
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

            uint8_t* data();          // Access the buffer pointer
            size_t size() const;      // Get the size of the element

            Element& operator[](size_t index);               // Access by array index
            const Element& operator[](size_t index) const;

            void resizeArray(size_t arraySize, size_t elementSize); // Resize the array

            Element& addElement(const std::wstring& name, uint8_t* buffer, size_t elementSize, size_t arraySize = 0); // Add a sub-element
            Element& operator[](const std::wstring& name);    // Access by name
        };

    private:
        std::string _typeName;                               // Name of the uniform block in GLSL
        BindingPoint _bindingPoint;                          // Binding point for the UBO
        size_t _blockSize;                                   // Total size of the UBO's buffer
        std::unordered_map<std::wstring, Element> _elements; // Map of top-level elements

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
