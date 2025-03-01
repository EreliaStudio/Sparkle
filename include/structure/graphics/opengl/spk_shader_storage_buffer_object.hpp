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
#include "structure/graphics/opengl/spk_binded_buffer_object.hpp"

#include "spk_debug_macro.hpp"
#include "utils/spk_string_utils.hpp"

namespace spk::OpenGL
{
    class ShaderStorageBufferObject : public BindedBufferObject
    {
    public:
		using FixedData = Element;

        class DynamicArray
        {
            friend class ShaderStorageBufferObject;
        private:
			ShaderStorageBufferObject* _owner = nullptr;
			size_t _bufferOffset = 0;
			size_t _elementSize = 0;
			size_t _paddingBetweenElements = 0;
			std::vector<Element> _elements;

			DynamicArray() = default;
			DynamicArray(ShaderStorageBufferObject* p_owner, size_t p_bufferOffset, size_t p_elementSize, size_t p_paddingBetweenElements) :
				_owner(p_owner),
				_bufferOffset(p_bufferOffset),
				_elementSize(p_elementSize),
				_paddingBetweenElements(p_paddingBetweenElements)
			{

			}

        public:
			size_t nbElements() const {return (_elements.size());}

			void clear()
			{
				_elements.clear();
			}

			void resize(size_t p_newSize)
			{
				_owner->resize(p_newSize * (_elementSize + _paddingBetweenElements));

				_elements.resize(p_newSize);

				for (size_t i = 0; i < p_newSize; i++)
				{
					_elements[i] = Element(_owner->data() + _bufferOffset + (_elementSize + _paddingBetweenElements) * i, _elementSize);
				}
			}

			template<typename TElementType>
			void push_back(const TElementType& p_data)
			{
				_elements.push_back(Element());

				_owner->resize(_elements.size() * (_elementSize + _paddingBetweenElements));

				for (size_t i = 0; i < _elements.size(); i++)
				{
					_elements[i] = Element(_owner->data() + _bufferOffset + (_elementSize + _paddingBetweenElements) * i, _elementSize);
				}
			}

			Element& operator[](const size_t& p_index)
			{
				if (_elements.size() <= p_index)
					throw std::out_of_range("Invalid access to element in SSBO dynamic array");

				return (_elements.at(p_index));
			}

			const Element& operator[](const size_t& p_index) const
			{
				if (_elements.size() <= p_index)
					throw std::out_of_range("Invalid access to element in SSBO dynamic array");

				return (_elements.at(p_index));
			}
            
			void validate()
			{
				_owner->validate();
			}
        };

    private:
        FixedData _fixedData;
        size_t _paddingBetweenFixedAndDynamic;
		DynamicArray _dynamicArray;

    public:
        // Constructors
        ShaderStorageBufferObject() = default;

        ShaderStorageBufferObject(const std::string& p_name,
                                  BindingPoint p_bindingPoint,
                                  size_t p_fixedSize,
                                  size_t p_fixedPadding,
                                  size_t p_dynamicElementSize,
                                  size_t p_dynamicPadding) :
			BindedBufferObject(VertexBufferObject::Type::ShaderStorage, VertexBufferObject::Usage::Dynamic, p_name, p_bindingPoint, p_fixedSize + p_fixedPadding),
			_fixedData(data(), p_fixedSize),
			_dynamicArray(this, p_fixedSize + p_fixedPadding, p_dynamicElementSize, p_dynamicPadding)
		{
		
		}

        void activate() override
		{
			BindedBufferObject::activate();

			GLint prog = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
			if (prog == 0)
				throw std::runtime_error("No shader program is currently bound.");

			GLint blockIndex = glGetProgramResourceIndex(prog, GL_SHADER_STORAGE_BLOCK, _name.c_str());
			if (blockIndex == GL_INVALID_INDEX)
				throw std::runtime_error("Shader storage block '" + _name + "' not found in the shader program.");

			glShaderStorageBlockBinding(prog, blockIndex, _bindingPoint);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _bindingPoint, _id);
		}

        FixedData& fixedData() { return _fixedData; }
        const FixedData& fixedData() const { return _fixedData; }

        DynamicArray& dynamicArray() {return _dynamicArray;}
        const DynamicArray& dynamicArray() const { return _dynamicArray; }
    };
}
