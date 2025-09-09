#pragma once

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	undef WIN32_LEAN_AND_MEAN
#endif

#include <GL/gl.h>
#include <GL/glew.h>

#include "spk_debug_macro.hpp"
#include "structure/container/spk_data_buffer_layout.hpp"
#include "structure/container/spk_json_object.hpp"
#include "structure/design_pattern/spk_contract_provider.hpp"
#include "structure/graphics/opengl/spk_vertex_buffer_object.hpp"
#include "structure/system/spk_exception.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

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

			DynamicArray(
				const std::wstring &p_name, spk::DataBuffer *p_buffer, size_t p_fixedReservedSpace, size_t p_elementSize, size_t p_elementPadding);

			DynamicArray(const DynamicArray &) = delete;
			DynamicArray &operator=(const DynamicArray &) = delete;

			DynamicArray(DynamicArray &&p_other);
			DynamicArray &operator=(DynamicArray &&p_other);

			bool contains(const std::wstring &p_name);
			Element &addElement(const std::wstring &p_name, size_t p_offset, size_t p_size);
			Element &addElement(const std::wstring &p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding);
			void removeElement(const std::wstring &p_name);
			void clear();
			void resize(size_t p_nbElement);

			template <typename TType>
			void pushBack(const TType &p_value)
			{
				resize(_elements.size() + 1);
				_elements.back() = p_value;
			}

			Element &operator[](size_t p_index);
			const Element &operator[](size_t p_index) const;

			size_t nbElement() const;
			size_t elementSize() const;

			template <typename T>
			DynamicArray &operator=(std::initializer_list<T> p_list)
			{
				resize(p_list.size());
				size_t index = 0;
				for (auto &value : p_list)
				{
					_elements[index] = value;
					++index;
				}
				return *this;
			}

		private:
			spk::TContractProvider<size_t> _resizeContractProvider;
			spk::DataBuffer *_buffer;
			std::vector<Element> _elements;
			Element _defaultElement;
			size_t _fixedReservedSpace;
			size_t _elementSize;
			size_t _elementPadding;

			void _redoArray();
			DynamicArray _duplicate() const;
		};

	private:
		std::wstring _blockName;
		BindingPoint _bindingPoint;
		GLint _blockIndex;

		spk::DataBufferLayout::Element _fixedData;
		spk::TContractProvider<size_t>::Contract _onResizeContract;
		DynamicArray _dynamicArray;

		void _loadElement(spk::DataBufferLayout::Element &p_parent, const spk::JSON::Object &p_elemDesc);
		void _loadElement(DynamicArray &p_array, const spk::JSON::Object &p_elemDesc);

	public:
		ShaderStorageBufferObject();
		ShaderStorageBufferObject(
			const std::wstring &p_blockName,
			BindingPoint p_bindingPoint,
			size_t p_fixedSize,
			size_t p_paddingFixedToDynamic,
			size_t p_dynamicElementSize,
			size_t p_dynamicElementPadding);
		ShaderStorageBufferObject(const spk::JSON::Object &p_desc);
		ShaderStorageBufferObject(const ShaderStorageBufferObject &p_other);
		ShaderStorageBufferObject &operator=(const ShaderStorageBufferObject &p_other);
		ShaderStorageBufferObject(ShaderStorageBufferObject &&p_other);
		ShaderStorageBufferObject &operator=(ShaderStorageBufferObject &&p_other);

		const std::wstring &blockName() const;
		void setBlockName(const std::wstring &p_blockName);

		BindingPoint bindingPoint() const;
		void setBindingPoint(BindingPoint p_bindingPoint);

		spk::DataBufferLayout::Element &fixedData();
		const spk::DataBufferLayout::Element &fixedData() const;

		DynamicArray &dynamicArray();
		const DynamicArray &dynamicArray() const;

		void activate() override;
	};
}