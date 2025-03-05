#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <unordered_map>
#include <variant>
#include "spk_debug_macro.hpp"
#include "structure/graphics/opengl/spk_binded_buffer_object.hpp"

namespace spk::OpenGL
{

	class ShaderStorageBufferObject : public BindedBufferObject
	{
	public:
		using BindingPoint = int;

		class DynamicArray
		{
		private:
			Element* _dynamicElement;

		public:
			DynamicArray()
			{

			}

			void resize()
			{

			}

			Element& operator[](const size_t& p_index)
			{
				return (_dynamicElement->operator[](p_index));
			}

			template<TType>
			void push_back(const TType& p_value)
			{

			}
		};

	private:
		Element _fixedData;
		DynamicArray _dynamicArray;

	public:
		Element &fixedData() { return _fixedData; }
		const Element &fixedData() const { return _fixedData; }

		DynamicArray &dynamicArray() { return _dynamicArray; }
		const DynamicArray &dynamicArray() const { return _dynamicArray; }

		ShaderStorageBufferObject() :
			BindedBufferObject(
				VertexBufferObject::Type::ShaderStorage,
				VertexBufferObject::Usage::Dynamic,
				"", // empty name
				0,	 // binding point 0
				0), // initial buffer size is 0
			_fixedSize(0),
			_fixedPadding(0),
			_dynamicElementSize(0),
			_dynamicPadding(0),
			_fixedData(data(), 0),
			_dynamicArray(static_cast<uint8_t *>(data()), 0, 0)
		{

		}

		ShaderStorageBufferObject( const std::string &p_name, BindingPoint p_bindingPoint, size_t p_fixedSize, size_t p_fixedPadding, size_t p_dynamicElementSize, size_t p_dynamicPadding) :
			BindedBufferObject(
				VertexBufferObject::Type::ShaderStorage,
				VertexBufferObject::Usage::Dynamic,
				p_name,
				p_bindingPoint,
				p_fixedSize + p_fixedPadding),
			_fixedSize(p_fixedSize),
			_fixedPadding(p_fixedPadding),
			_dynamicElementSize(p_dynamicElementSize),
			_dynamicPadding(p_dynamicPadding),
			_fixedData(data(), p_fixedSize),
			_dynamicArray(
				static_cast<uint8_t *>(data()) + p_fixedSize + p_fixedPadding,
				p_dynamicElementSize, p_dynamicPadding)
		{
			_dynamicArray.contractProvider.subscribe([this]()
			{
				this->onDynamicArrayChange();
			});
		}

		ShaderStorageBufferObject(const ShaderStorageBufferObject &p_other) : ShaderStorageBufferObject(p_other._name, p_other._bindingPoint, p_other._fixedSize, p_other._fixedPadding, p_other._dynamicElementSize, p_other._dynamicPadding)
		{
		}

		ShaderStorageBufferObject(ShaderStorageBufferObject &&other) noexcept : BindedBufferObject(std::move(other)),
																				_fixedSize(other._fixedSize),
																				_fixedPadding(other._fixedPadding),
																				_dynamicElementSize(other._dynamicElementSize),
																				_dynamicPadding(other._dynamicPadding),
																				_fixedData(data(), other._fixedSize),
																				_dynamicArray(static_cast<uint8_t *>(data()) + other._fixedSize + other._fixedPadding, other._dynamicElementSize, other._dynamicPadding)
		{
			_dynamicArray._elements = std::move(other._dynamicArray._elements);
			_dynamicArray.setBaseAddress(static_cast<uint8_t *>(data()) + _fixedSize + _fixedPadding);
			_dynamicArray.contractProvider.subscribe([this]()
			{
            	this->onDynamicArrayChange();
			});
		}

		ShaderStorageBufferObject &operator=(const ShaderStorageBufferObject &other)
		{
			if (this != &other)
			{
				BindedBufferObject::operator=(other);
				_fixedSize = other._fixedSize;
				_fixedPadding = other._fixedPadding;
				_dynamicElementSize = other._dynamicElementSize;
				_dynamicPadding = other._dynamicPadding;

				_fixedData.setBuffer(data());

				_dynamicArray.setBaseAddress(static_cast<uint8_t *>(data()) + _fixedSize + _fixedPadding);
				_dynamicArray.resize(other._dynamicArray.size());
			}
			return *this;
		}

		ShaderStorageBufferObject &operator=(ShaderStorageBufferObject &&other) noexcept
		{
			if (this != &other)
			{
				BindedBufferObject::operator=(std::move(other));
				_fixedSize = other._fixedSize;
				_fixedPadding = other._fixedPadding;
				_dynamicElementSize = other._dynamicElementSize;
				_dynamicPadding = other._dynamicPadding;

				_fixedData.setBuffer(data());
				_dynamicArray._elements = std::move(other._dynamicArray._elements);
				_dynamicArray.setBaseAddress(static_cast<uint8_t *>(data()) + _fixedSize + _fixedPadding);
			}
			return *this;
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
	};
}
