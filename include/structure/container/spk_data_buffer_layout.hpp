#pragma once

#include <variant>
#include <unordered_map>

#include "structure/container/spk_data_buffer.hpp"

#include "utils/spk_string_utils.hpp"

#include "structure/spk_iostream.hpp"

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

			std::variant<Unit, Array, Structure> _content;

		private:
			DataBuffer* _buffer = nullptr;

			size_t _offset = 0;
			size_t _size   = 0;

			void setBuffer(DataBuffer* p_buffer)
			{
				_buffer = p_buffer;

				if (isArray() == true)
				{
					Array& tmpArray = std::get<Array>(_content);

					for (auto& element : tmpArray)
					{
						element.setBuffer(p_buffer);
					}
				}

				if (isStructure() == true)
				{
					Structure& tmpStructure = std::get<Structure>(_content);

					for (auto& [key, element] : tmpStructure)
					{
						element.setBuffer(p_buffer);
					}
				}
			}

		public:
			Element() = default;

			Element(DataBuffer* p_buffer, size_t p_offset, size_t p_size) :
				_buffer(p_buffer),
				_offset(p_offset),
				_size(p_size),
				_content(Unit())
			{

			}

			bool isUnit() const
			{
				return std::holds_alternative<Unit>(_content);
			}

			bool isArray() const
			{
				return std::holds_alternative<Array>(_content);
			}

			bool isStructure() const
			{
				return std::holds_alternative<Structure>(_content);
			}
	
			bool contains(const std::wstring& p_name)
			{
				if (isArray() == true)
				{
					throw std::runtime_error("Can't check for presence of element by name in array");
				}

				if (isUnit() == true)
				{
					throw std::runtime_error("Can't check for presence of element by name in unit");
				}

				Structure& structure = std::get<Structure>(_content);

				return structure.contains(p_name);
			}

			Element& addElement(const std::wstring& p_name, size_t p_offset, size_t p_size)
			{
				if (isArray() == true)
				{
					throw std::runtime_error("Can't assign a field to an array");
				}

				if (isUnit() == true)
				{
					_content = Structure();
				}

				if (contains(p_name) == true)
				{
					throw std::runtime_error("Element [" + spk::StringUtils::wstringToString(p_name) + "] already contained in structure");
				}

				if ((p_offset + p_size) > (_offset + _size))
				{
					throw std::runtime_error("Can't assign a field larger than the current element");
				}

				if ((p_offset + p_size) > _buffer->size())
				{
					throw std::runtime_error("Can't create a new element overflowing the buffer");
				}

				Structure& structure = std::get<Structure>(_content);

				structure[p_name] = Element(_buffer, p_offset, p_size);

				return structure[p_name];
			}

			Element& addElement(const std::wstring& p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding)
			{
				if (isArray() == true)
				{
					throw std::runtime_error("Can't assign a field to an array");
				}

				Element& newElement = addElement(p_name, p_offset, p_nbElement * (p_elementSize + p_elementPadding));

				Array newArray = Array();

				newArray.resize(p_nbElement);

				for (size_t i = 0; i < p_nbElement; i++)
				{
					size_t childOffset = p_offset + i * (p_elementSize + p_elementPadding);

					newArray[i] = Element(_buffer, childOffset, p_elementSize);
				}

				newElement._content = std::move(newArray);

				return (newElement);
			}

			void removeElement(const std::wstring& p_name)
			{
				if (isArray() == true)
				{
					throw std::runtime_error("Can't remove a field from an array");
				}

				if (contains(p_name) == false)
				{
					throw std::runtime_error("Element [" + spk::StringUtils::wstringToString(p_name) + "] not contained in structure");
				}

				Structure& structure = std::get<Structure>(_content);
			
				structure.erase(p_name);
			}

			Element& operator[](size_t index)
			{
				if (isArray() == false)
				{
					throw std::runtime_error("This element is not an array.");
				}

				auto& vec = std::get<Array>(_content);

				if (index >= vec.size())
				{
					throw std::runtime_error("Array index out of range.");
				}

				return vec[index];
			}

			const Element& operator[](size_t index) const
			{
				if (isArray() == false)
				{
					throw std::runtime_error("This element is not an array.");
				}

				auto& vec = std::get<Array>(_content);

				if (index >= vec.size())
				{
					throw std::runtime_error("Array index out of range.");
				}
				
				return vec[index];
			}

			Element& operator[](const std::wstring& key)
			{
				if (isStructure() == false)
				{
					throw std::runtime_error("This element is not a map/struct.");
				}
				
				auto& map = std::get<Structure>(_content);
				auto it = map.find(key);
				
				if (it == map.end())
				{
					throw std::runtime_error("Key not found in map: " + spk::StringUtils::wstringToString(key));
				}
				
				return it->second;
			}

			const Element& operator[](const std::wstring& key) const
			{
				if (isStructure() == false)
				{
					throw std::runtime_error("This element is not a structure.");
				}
				
				auto& map = std::get<Structure>(_content);
				auto it = map.find(key);
				
				if (it == map.end())
				{
					throw std::runtime_error("Key not found in structure: " + spk::StringUtils::wstringToString(key));
				}
				
				return it->second;
			}

			template<typename TType>
			Element& operator=(const TType& value)
			{
				static_assert(std::is_standard_layout<TType>::value, "TType must be a standard layout type.");

				if (isArray() == true)
				{
					throw std::runtime_error("operator= called on an array element.");
				}

				if (_buffer == nullptr)
				{
					throw std::runtime_error("No DataBuffer associated with this element.");
				}

				if (sizeof(TType) != _size)
				{
					throw std::runtime_error("Invalid TType : expected [" + std::to_string(_size) + "] bytes, but received [" + std::to_string(sizeof(TType)) + "].");
				}

				spk::cout << "Writing data [" << value << "] at offset [" << _offset << "]" << std::endl;
				std::memcpy(_buffer->data() + _offset, &value, sizeof(TType));
				
				return *this;
			}

			template<typename TType>
			operator TType() const
			{
				return get<TType>();
			}

			template<typename TType>
			TType& get()
			{
				static_assert(std::is_standard_layout<TType>::value,
							"Element unit: TType must be a standard layout type.");

				if (isArray() == true)
				{
					throw std::runtime_error("operator TType() called on an array element.");
				}

				if (_buffer == nullptr)
				{
					throw std::runtime_error("No DataBuffer associated with this element.");
				}

				if (sizeof(TType) != _size)
				{
					throw std::runtime_error("Invalid TType : expected [" + std::to_string(_size) + "] bytes, but received [" + std::to_string(sizeof(TType)) + "].");
				}

				return (*(reinterpret_cast<TType*>((_buffer->data() + _offset))));
			}

			template<typename TType>
			const TType& get() const
			{
				static_assert(std::is_standard_layout<TType>::value,
							"Element unit: TType must be a standard layout type.");

				if (isArray() == true)
				{
					throw std::runtime_error("operator TType() called on an array element.");
				}

				if (_buffer == nullptr)
				{
					throw std::runtime_error("No DataBuffer associated with this element.");
				}

				if (sizeof(TType) != _size)
				{
					throw std::runtime_error("Invalid TType : expected [" + std::to_string(_size) + "] bytes, but received [" + std::to_string(sizeof(TType)) + "].");
				}

				return (*(reinterpret_cast<const TType*>((_buffer->data() + _offset))));
			}

			size_t offset() const { return _offset; }
			size_t size()   const { return _size; }
		};

    private:
        DataBuffer* _buffer;
        Element _root;

    public:
        DataBufferLayout(DataBuffer* buffer = nullptr) :
			_buffer(buffer)
        {
			if (buffer != nullptr)
			{
            	_root = Element(_buffer, 0, buffer->size());
			}
			else
			{
				_root = Element(nullptr, 0, 0);
			}
        }

		DataBufferLayout(const DataBufferLayout& p_other) :
			_buffer(p_other._buffer),
			_root(p_other._root)
		{

		}

		DataBufferLayout(DataBufferLayout&& p_other) :
			_buffer(std::move(p_other._buffer)),
			_root(std::move(p_other._root))
		{

		}

		DataBufferLayout& operator = (const DataBufferLayout& p_other)
		{
			if (this != &p_other)
			{
				_buffer = p_other._buffer;
				_root = p_other._root;
			}

			return (*this);
		}

		DataBufferLayout& operator = (DataBufferLayout&& p_other)
		{
			if (this != &p_other)
			{
				_buffer = std::move(p_other._buffer);
				_root = std::move(p_other._root);
			}

			return (*this);
		}

		void setBuffer(DataBuffer* buffer)
		{
			_root.setBuffer(buffer);
		}

        Element& root()
		{
			return _root;
		}

        const Element& root() const
		{
			return _root;
		}

		bool contains(const std::wstring& p_name)
		{
			return (_root.contains(p_name));
		}

		Element& addElement(const std::wstring& p_name, size_t p_offset, size_t p_size)
		{
			return (_root.addElement(p_name, p_offset, p_size));
		}

		Element& addElement(const std::wstring& p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding)
		{
			return (_root.addElement(p_name, p_offset, p_nbElement, p_elementSize, p_elementPadding));
		}

		void removeElement(const std::wstring& p_name)
		{
			_root.removeElement(p_name);
		}

        Element& operator[](size_t index)
        {
            return _root[index];
        }

        Element& operator[](const std::wstring& key)
        {
            return _root[key];
        }

        const Element& operator[](size_t index) const
        {
            return _root[index];
        }

        const Element& operator[](const std::wstring& key) const
        {
            return _root[key];
        }
    };
}