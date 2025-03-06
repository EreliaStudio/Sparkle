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
#include "structure/container/spk_data_buffer_layout.hpp"
#include "structure/graphics/opengl/spk_vertex_buffer_object.hpp"
#include "structure/design_pattern/spk_contract_provider.hpp"

namespace spk::OpenGL
{
	class ShaderStorageBufferObject : public VertexBufferObject
	{
	public:
		using BindingPoint = int;

		class DynamicArray
		{
			friend class ShaderStorageBufferObject;

		public:
			using Element = spk::DataBufferLayout::Element;

		private:
			spk::TContractProvider<size_t> _resizeContractProvider;
			spk::DataBuffer* _buffer;
			std::vector<Element> _elements;
			Element _defaultElement;
			size_t _fixedReservedSpace;
			size_t _elementSize;
			size_t _elementPadding;

			void redoArray()
			{
				for (size_t i = 0; i < _elements.size(); i++)
				{
					auto& element = _elements[i];

					element = _defaultElement.duplicate(_fixedReservedSpace + (_elementSize + _elementPadding) * i);
				}
			}

		public:
			DynamicArray(spk::DataBuffer* p_buffer, size_t p_fixedReservedSpace, size_t p_elementSize, size_t p_elementPadding) :
				_defaultElement(p_buffer, 0, p_elementSize),
				_fixedReservedSpace(p_fixedReservedSpace),
				_elementSize(p_elementSize),
				_elementPadding(p_elementPadding)
			{

			}

			bool contains(const std::wstring& p_name)
			{
				return (_defaultElement.contains(p_name));
			}

			Element& addElement(const std::wstring& p_name, size_t p_offset, size_t p_size)
			{
				Element& result = _defaultElement.addElement(p_name, p_offset, p_size);

				redoArray();

				return (result);
			}

			Element& addElement(const std::wstring& p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding)
			{
				Element& result = _defaultElement.addElement(p_name, p_offset, p_nbElement, p_elementSize, p_elementPadding);

				redoArray();

				return (result);
			}

			void removeElement(const std::wstring& p_name)
			{
				_defaultElement.removeElement(p_name);

				redoArray();
			}

			void resize(size_t p_nbElement)
			{
				_resizeContractProvider.trigger(_fixedReservedSpace + (_elementSize + _elementPadding) * p_nbElement);

				_elements.resize(p_nbElement);

				redoArray();
			}

			template <typename TType>
			void push_back(const TType& p_value)
			{
				resize(_elements.size() + 1);

				_elements.back() = p_value;
			}

			Element& operator[](size_t p_index)
			{
				return (_elements[p_index]);
			}
			
			const Element& operator[](size_t p_index) const
			{
				return (_elements[p_index]);
			}

			size_t nbElement() const
			{
				return (_elements.size());
			}
		};

	private:
		std::wstring _blockName;
		BindingPoint _bindingPoint;
		GLint _blockIndex;

		spk::DataBufferLayout::Element _fixedData;
		spk::TContractProvider<size_t>::Contract _onResizeContract;
		DynamicArray _dynamicArray;

	public:
		ShaderStorageBufferObject() :
			VertexBufferObject(VertexBufferObject::Type::ShaderStorage, VertexBufferObject::Usage::Dynamic),
			_blockName(L"Unnamed SSBO"),
			_bindingPoint(-1),
			_blockIndex(-1),
			_fixedData(&(dataBuffer()), 0, 0),
			_dynamicArray(&dataBuffer(), 0, 0, 0)
		{
			_onResizeContract = _dynamicArray._resizeContractProvider.subscribe([&](size_t p_size){resize(p_size);});
		}

		ShaderStorageBufferObject(const std::wstring& p_blockName, BindingPoint p_bindingPoint, size_t p_fixedSize, size_t p_paddingFixedToDynamic, size_t p_dynamicElementSize, size_t p_dynamicElementPadding) :
			VertexBufferObject(VertexBufferObject::Type::ShaderStorage, VertexBufferObject::Usage::Dynamic),
			_blockName(p_blockName),
			_bindingPoint(p_bindingPoint),
			_blockIndex(-1),
			_fixedData(&(dataBuffer()), 0, p_fixedSize),
			_dynamicArray(&dataBuffer(), p_fixedSize + p_paddingFixedToDynamic, p_dynamicElementSize, p_dynamicElementPadding)
		{
			_onResizeContract = _dynamicArray._resizeContractProvider.subscribe([&](size_t p_size){resize(p_size);});
		}
		
		const std::wstring& blockName() const
		{
			return (_blockName);
		}
		void setBlockName(const std::wstring& p_blockName)
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

		spk::DataBufferLayout::Element& fixedData()
		{
			return (_fixedData);
		}

		const spk::DataBufferLayout::Element& fixedData() const
		{
			return (_fixedData);
		}

		DynamicArray& dynamicArray()
		{
			return (_dynamicArray);
		}

		const DynamicArray& dynamicArray() const
		{
			return (_dynamicArray);
		}

		void activate() override
		{
			VertexBufferObject::activate();
			GLint prog = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
			if (prog == 0)
				throw std::runtime_error("No shader program is currently bound.");

			if (_blockIndex == -1)
			{
				std::string str = spk::StringUtils::wstringToString(_blockName);
				_blockIndex = glGetProgramResourceIndex(prog, GL_SHADER_STORAGE_BLOCK, str.c_str());
				if (_blockIndex == GL_INVALID_INDEX)
					throw std::runtime_error("Shader storage block '" + str + "' not found in the shader program.");
			}

			glShaderStorageBlockBinding(prog, _blockIndex, _bindingPoint);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _bindingPoint, _id);
		}
	};
}
