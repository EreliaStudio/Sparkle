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
#include "structure/design_pattern/spk_contract_provider.hpp"

namespace spk::OpenGL
{

	class ShaderStorageBufferObject : public BindedBufferObject
	{
	public:
		using BindingPoint = int;

		class DynamicArray
		{
		public:
			DynamicArray(uint8_t *baseAddress,
						 size_t elementSize,
						 size_t padding) : _baseAddress(baseAddress),
										   _elementSize(elementSize),
										   _padding(padding)
			{
			}

			template <typename TType>
			void push_back(const TType &value)
			{
				if (sizeof(TType) != _elementSize)
				{
					throw std::runtime_error("push_back: size mismatch - Expected size [" + std::to_string(_elementSize) + "] bytes but received [" + std::to_string(sizeof(TType)) + "]");
				}
				_elements.push_back(Element(nullptr, _elementSize));
				triggerChange();
				updatePointers();
				_elements.back() = value;
			}

			void resize(size_t newSize)
			{
				_elements.resize(newSize);
				triggerChange();
				updatePointers();
			}

			void clear()
			{
				_elements.clear();
				triggerChange();
			}

			size_t size() const
			{
				return _elements.size();
			}

			Element &operator[](size_t index)
			{
				return _elements.at(index);
			}
			const Element &operator[](size_t index) const
			{
				return _elements.at(index);
			}

			ContractProvider contractProvider;

			void setBaseAddress(uint8_t *base)
			{
				_baseAddress = base;
				updatePointers();
			}

		private:
			uint8_t *_baseAddress;
			size_t _elementSize;
			size_t _padding;
			std::vector<Element> _elements;

			void updatePointers()
			{
				for (size_t i = 0; i < _elements.size(); i++)
				{
					size_t offset = i * (_elementSize + _padding);
					_elements[i].setBuffer(_baseAddress + offset);
				}
			}

			void triggerChange()
			{
				contractProvider.trigger();
			}

			friend class ShaderStorageBufferObject;
		};

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
			DEBUG_LINE();
			_dynamicArray.contractProvider.subscribe([&]()
			{
				DEBUG_LINE();
            	onDynamicArrayChange();
				DEBUG_LINE();
			});
			DEBUG_LINE();
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
			DEBUG_LINE();
			_dynamicArray.contractProvider.subscribe([this]()
			{
				DEBUG_LINE();
				this->onDynamicArrayChange();
				DEBUG_LINE();
			});
			DEBUG_LINE();
		}

		ShaderStorageBufferObject(const ShaderStorageBufferObject &p_other) : ShaderStorageBufferObject(p_other._name, p_other._bindingPoint, p_other._fixedSize, p_other._fixedPadding, p_other._dynamicElementSize, p_other._dynamicPadding)
		{
			DEBUG_LINE();
		}

		ShaderStorageBufferObject(ShaderStorageBufferObject &&other) noexcept : BindedBufferObject(std::move(other)),
																				_fixedSize(other._fixedSize),
																				_fixedPadding(other._fixedPadding),
																				_dynamicElementSize(other._dynamicElementSize),
																				_dynamicPadding(other._dynamicPadding),
																				_fixedData(data(), other._fixedSize),
																				_dynamicArray(static_cast<uint8_t *>(data()) + other._fixedSize + other._fixedPadding, other._dynamicElementSize, other._dynamicPadding)
		{
			DEBUG_LINE();
			_dynamicArray._elements = std::move(other._dynamicArray._elements);
			DEBUG_LINE();
			_dynamicArray.setBaseAddress(static_cast<uint8_t *>(data()) + _fixedSize + _fixedPadding);
			DEBUG_LINE();
			_dynamicArray.contractProvider.subscribe([this]()
			{
				DEBUG_LINE();
            	this->onDynamicArrayChange();
				DEBUG_LINE();
			});
			DEBUG_LINE();
		}

		ShaderStorageBufferObject &operator=(const ShaderStorageBufferObject &other)
		{
			DEBUG_LINE();
			if (this != &other)
			{
				DEBUG_LINE();
				BindedBufferObject::operator=(other);
				DEBUG_LINE();
				_fixedSize = other._fixedSize;
				_fixedPadding = other._fixedPadding;
				_dynamicElementSize = other._dynamicElementSize;
				_dynamicPadding = other._dynamicPadding;
				DEBUG_LINE();

				_fixedData.setBuffer(data());
				DEBUG_LINE();

				_dynamicArray.setBaseAddress(static_cast<uint8_t *>(data()) + _fixedSize + _fixedPadding);
				DEBUG_LINE();
				_dynamicArray.resize(other._dynamicArray.size());
				DEBUG_LINE();
			}
			DEBUG_LINE();
			return *this;
		}

		ShaderStorageBufferObject &operator=(ShaderStorageBufferObject &&other) noexcept
		{
			DEBUG_LINE();
			if (this != &other)
			{
				DEBUG_LINE();
				BindedBufferObject::operator=(std::move(other));
				DEBUG_LINE();
				_fixedSize = other._fixedSize;
				_fixedPadding = other._fixedPadding;
				_dynamicElementSize = other._dynamicElementSize;
				_dynamicPadding = other._dynamicPadding;
				DEBUG_LINE();

				_fixedData.setBuffer(data());
				_dynamicArray._elements = std::move(other._dynamicArray._elements);
				_dynamicArray.setBaseAddress(static_cast<uint8_t *>(data()) + _fixedSize + _fixedPadding);
				DEBUG_LINE();
			}
			DEBUG_LINE();
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

	private:
		size_t _fixedSize = 0;
		size_t _fixedPadding = 0;
		size_t _dynamicElementSize = 0;
		size_t _dynamicPadding = 0;

		Element _fixedData;
		DynamicArray _dynamicArray;

		void onDynamicArrayChange()
		{
			size_t numElements = _dynamicArray.size();
			size_t dynamicBlockSize = (numElements > 0) ? (_dynamicElementSize + _dynamicPadding) * numElements - _dynamicPadding : 0;
			size_t newSize = _fixedSize + _fixedPadding + dynamicBlockSize;
			BindedBufferObject::resize(newSize);
			_fixedData.setBuffer(data());
			uint8_t *dynBase = static_cast<uint8_t *>(data()) + _fixedSize + _fixedPadding;
			_dynamicArray.setBaseAddress(dynBase);
		}
	};
}
