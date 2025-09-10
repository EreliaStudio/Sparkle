#pragma once

#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "structure/container/spk_data_buffer.hpp"
#include "structure/spk_iostream.hpp"
#include "utils/spk_string_utils.hpp"

#include "structure/system/spk_exception.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	class DataBufferLayout
	{
	public:
		class Element
		{
			friend class DataBufferLayout;

		public:
			using Unit = std::monostate;
			using Array = std::vector<Element>;
			using Structure = std::unordered_map<std::wstring, Element>;

		private:
			std::wstring _name;
			DataBuffer *_buffer = nullptr;
			std::variant<Unit, Array, Structure> _content;
			size_t _offset = 0;
			size_t _size = 0;

		public:
			Element();
			Element(const std::wstring &p_name, DataBuffer *p_buffer, size_t p_offset, size_t p_size);

			DataBuffer *buffer() const;
			void setBuffer(DataBuffer *p_buffer);

			const std::wstring &name() const;
			void setName(const std::wstring &p_name);

			Element duplicate(size_t p_offset) const;
			bool isUnit() const;
			bool isArray() const;
			bool isStructure() const;
			bool contains(const std::wstring &p_name);
			size_t nbElement();
			void resize(size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding);
			Element &addElement(const std::wstring &p_name, size_t p_offset, size_t p_size);
			Element &addElement(const std::wstring &p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding);
			void removeElement(const std::wstring &p_name);
			Element &operator[](size_t p_index);
			const Element &operator[](size_t p_index) const;
			Element &operator[](const std::wstring &p_key);
			const Element &operator[](const std::wstring &p_key) const;

			template <typename TType>
			Element &operator=(const TType &p_value)
			{
				static_assert(std::is_standard_layout<TType>::value, "TType must be a standard layout type.");

				if (isArray())
				{
					GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - operator= called on an array element.");
				}

				if (_buffer == nullptr)
				{
					GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - No DataBuffer associated with this element.");
				}

				if (sizeof(TType) != _size)
				{
					GENERATE_ERROR(
						spk::StringUtils::wstringToString(_name) + " - Invalid TType : expected [" + std::to_string(_size) +
						"] bytes, but received [" + std::to_string(sizeof(TType)) + "].");
				}

				std::memcpy(_buffer->data() + _offset, &p_value, sizeof(TType));
				return *this;
			}

			template <typename TType>
			operator TType() const
			{
				return get<TType>();
			}

			template <typename TType>
			TType &get()
			{
				static_assert(std::is_standard_layout<TType>::value, "Element unit: TType must be a standard layout type.");

				if (isArray())
				{
					GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - operator TType() called on an array element.");
				}

				if (_buffer == nullptr)
				{
					GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - No DataBuffer associated with this element.");
				}

				if (sizeof(TType) != _size)
				{
					GENERATE_ERROR(
						spk::StringUtils::wstringToString(_name) + " - Invalid TType : expected [" + std::to_string(_size) +
						"] bytes, but received [" + std::to_string(sizeof(TType)) + "].");
				}

				return *reinterpret_cast<TType *>(_buffer->data() + _offset);
			}

			template <typename TType>
			const TType &get() const
			{
				static_assert(std::is_standard_layout<TType>::value, "Element unit: TType must be a standard layout type.");

				if (isArray())
				{
					GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - operator TType() called on an array element.");
				}

				if (_buffer == nullptr)
				{
					GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - No DataBuffer associated with this element.");
				}

				if (sizeof(TType) != _size)
				{
					GENERATE_ERROR(
						spk::StringUtils::wstringToString(_name) + " - Invalid TType : expected [" + std::to_string(_size) +
						"] bytes, but received [" + std::to_string(sizeof(TType)) + "].");
				}

				return *reinterpret_cast<const TType *>(_buffer->data() + _offset);
			}

			template <typename TType>
			std::vector<TType> getArray() const
			{
				static_assert(
					std::is_standard_layout<TType>::value, "DataBufferLayout::Element::getArray<TType>() - TType must be a standard layout type.");

				if (!isArray())
				{
					GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - getArray<TType>() called on a non-array element.");
				}

				if (_buffer == nullptr)
				{
					GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - No DataBuffer associated with this element.");
				}

				const auto &arr = std::get<Array>(_content);
				std::vector<TType> result;
				result.reserve(arr.size());

				for (const auto &child : arr)
				{
					if (child.size() != sizeof(TType))
					{
						GENERATE_ERROR(
							spk::StringUtils::wstringToString(_name) + " - Child element size [" + std::to_string(child.size()) +
							"] does not match TType size [" + std::to_string(sizeof(TType)) + "]");
					}
					result.push_back(child.get<TType>());
				}

				return result;
			}

			size_t offset() const;
			size_t size() const;

			template <typename TExportedType>
			TExportedType *as()
			{
				if (sizeof(TExportedType) == size())
				{
					return reinterpret_cast<TExportedType *>(buffer()->data() + offset());
				}
				else
				{
					throw std::runtime_error(
						"DataBufferLayout::as<TExportedType>() - Size mismatch: expected " + std::to_string(size()) + ", got " +
						std::to_string(sizeof(TExportedType)) + ".");
				}
			}

			template <typename TExportedType>
			const TExportedType *as() const
			{
				if (sizeof(TExportedType) == size())
				{
					return reinterpret_cast<TExportedType *>(buffer()->data() + offset());
				}
				else
				{
					throw std::runtime_error(
						"DataBufferLayout::as<TExportedType>() - Size mismatch: expected " + std::to_string(size()) + ", got " +
						std::to_string(sizeof(TExportedType)) + ".");
				}
			}
		};

	private:
		Element _root;

	public:
		DataBufferLayout(DataBuffer *p_buffer = nullptr);
		DataBufferLayout(const std::wstring &p_name, DataBuffer *p_buffer = nullptr);
		DataBufferLayout(const DataBufferLayout &p_other);
		DataBufferLayout(DataBufferLayout &&p_other);

		DataBufferLayout &operator=(const DataBufferLayout &p_other);
		DataBufferLayout &operator=(DataBufferLayout &&p_other);

		void setBuffer(DataBuffer *p_buffer);
		void updateRootSize();
		Element &root();
		const Element &root() const;
		bool contains(const std::wstring &p_name);
		Element &addElement(const std::wstring &p_name, size_t p_offset, size_t p_size);
		Element &addElement(const std::wstring &p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding);
		void removeElement(const std::wstring &p_name);

		Element &operator[](size_t p_index);
		Element &operator[](const std::wstring &p_key);

		const Element &operator[](size_t p_index) const;
		const Element &operator[](const std::wstring &p_key) const;

		template <typename TExportedType>
		TExportedType *as()
		{
			if (sizeof(TExportedType) == _root.size())
			{
				return reinterpret_cast<TExportedType *>(_root.buffer()->data() + _root.offset());
			}
			else
			{
				throw std::runtime_error(
					"DataBufferLayout::as<TExportedType>() - Size mismatch: expected " + std::to_string(_root.size()) + ", got " +
					std::to_string(sizeof(TExportedType)) + ".");
			}
		}

		template <typename TExportedType>
		const TExportedType *as() const
		{
			if (sizeof(TExportedType) == _root.size())
			{
				return reinterpret_cast<TExportedType *>(_root.buffer()->data() + _root.offset());
			}
			else
			{
				throw std::runtime_error(
					"DataBufferLayout::as<TExportedType>() - Size mismatch: expected " + std::to_string(_root.size()) + ", got " +
					std::to_string(sizeof(TExportedType)) + ".");
			}
		}
	};
}
