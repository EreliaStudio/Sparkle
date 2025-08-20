#include "structure/container/spk_data_buffer_layout.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	DataBufferLayout::Element::Element() :
		_name(),
		_buffer(nullptr),
		_offset(0),
		_size(0),
		_content(Unit())
	{
	}

	DataBufferLayout::Element::Element(const std::wstring &p_name, DataBuffer *p_buffer, size_t p_offset, size_t p_size) :
		_name(p_name),
		_buffer(p_buffer),
		_offset(p_offset),
		_size(p_size),
		_content(Unit())
	{
	}

	DataBuffer *DataBufferLayout::Element::buffer() const
	{
		return (_buffer);
	}

	const std::wstring &DataBufferLayout::Element::name() const
	{
		return (_name);
	}

	void DataBufferLayout::Element::setName(const std::wstring &p_name)
	{
		_name = p_name;
	}

	DataBufferLayout::Element DataBufferLayout::Element::duplicate(size_t p_offset)
	{
		Element result = *this;
		result._offset = p_offset;
		return result;
	}

	void DataBufferLayout::Element::setBuffer(DataBuffer *p_buffer)
	{
		_buffer = p_buffer;

		if (isArray())
		{
			auto &tmpArray = std::get<Array>(_content);

			for (auto &element : tmpArray)
			{
				element.setBuffer(p_buffer);
			}
		}
		else if (isStructure())
		{
			auto &tmpStructure = std::get<Structure>(_content);
			for (auto &[key, element] : tmpStructure)
			{
				element.setBuffer(p_buffer);
			}
		}
	}

	bool DataBufferLayout::Element::isUnit() const
	{
		return std::holds_alternative<Unit>(_content);
	}

	bool DataBufferLayout::Element::isArray() const
	{
		return std::holds_alternative<Array>(_content);
	}

	bool DataBufferLayout::Element::isStructure() const
	{
		return std::holds_alternative<Structure>(_content);
	}

	bool DataBufferLayout::Element::contains(const std::wstring &p_name)
	{
		if (isArray())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - Can't check for presence of element by name in array");
		}

		if (isUnit())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - Can't check for presence of element by name in unit");
		}

		auto &structure = std::get<Structure>(_content);
		return structure.contains(p_name);
	}

	size_t DataBufferLayout::Element::nbElement()
	{
		if (!isArray())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - Can't check for element array size on non-array element");
		}

		return std::get<Array>(_content).size();
	}

	void DataBufferLayout::Element::resize(size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding)
	{
		if (!isArray())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - Can't resize a non-array element");
		}

		if (_buffer == nullptr)
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - No DataBuffer associated with this element.");
		}

		size_t newSize = p_nbElement * (p_elementSize + p_elementPadding);

		if (_offset + newSize > _buffer->size())
		{
			GENERATE_ERROR(
				spk::StringUtils::wstringToString(_name) + " - Resizing would exceed the buffer's capacity. Requested " +
				std::to_string(_offset + newSize) + " but buffer size is " + std::to_string(_buffer->size()) + ".");
		}

		Array newArray;
		newArray.resize(p_nbElement);

		for (size_t i = 0; i < p_nbElement; ++i)
		{
			size_t childOffset = _offset + i * (p_elementSize + p_elementPadding);
			newArray[i] = Element(_name + L"[" + std::to_wstring(i) + L"]", _buffer, childOffset, p_elementSize);
		}

		_content = std::move(newArray);
		_size = newSize;
	}

	DataBufferLayout::Element &DataBufferLayout::Element::addElement(const std::wstring &p_name, size_t p_offset, size_t p_size)
	{
		if (isArray())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - Can't assign a field to an array");
		}

		if (isUnit())
		{
			_content = Structure();
		}

		if (contains(p_name))
		{
			GENERATE_ERROR(
				spk::StringUtils::wstringToString(_name) + " - Element [" + spk::StringUtils::wstringToString(p_name) +
				"] already contained in structure");
		}

		if ((p_offset + p_size) > (_offset + _size))
		{
			GENERATE_ERROR(
				spk::StringUtils::wstringToString(_name) + " - Can't assign a field named [" + spk::StringUtils::wstringToString(p_name) +
				"] larger than the current element - Requested [" + std::to_string(p_size) + "] bytes at offset [" + std::to_string(p_offset) +
				"] over [" + std::to_string(_size) + "] allocated bytes at offset [" + std::to_string(_offset) + "]");
		}

		if ((p_offset + p_size) > _buffer->size())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - Can't create a new element overflowing the buffer");
		}

		auto &structure = std::get<Structure>(_content);
		structure[p_name] = Element(_name + L"." + p_name, _buffer, p_offset, p_size);
		return structure[p_name];
	}

	DataBufferLayout::Element &DataBufferLayout::Element::addElement(
		const std::wstring &p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding)
	{
		if (isArray())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - Can't assign a field to an array");
		}

		Element &newElement = addElement(p_name, p_offset, p_nbElement * (p_elementSize + p_elementPadding));

		Array newArray;
		newArray.resize(p_nbElement);

		for (size_t i = 0; i < p_nbElement; ++i)
		{
			size_t childOffset = p_offset + i * (p_elementSize + p_elementPadding);
			newArray[i] = Element(_name + L"[" + std::to_wstring(i) + L"]", _buffer, childOffset, p_elementSize);
		}

		newElement._content = std::move(newArray);
		return newElement;
	}

	void DataBufferLayout::Element::removeElement(const std::wstring &p_name)
	{
		if (isArray())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - Can't remove a field from an array");
		}

		if (!contains(p_name))
		{
			GENERATE_ERROR(
				spk::StringUtils::wstringToString(_name) + " - Element [" + spk::StringUtils::wstringToString(p_name) +
				"] not contained in structure");
		}

		auto &structure = std::get<Structure>(_content);
		structure.erase(p_name);
	}

	DataBufferLayout::Element &DataBufferLayout::Element::operator[](size_t p_index)
	{
		if (!isArray())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - This element is not an array.");
		}

		auto &vec = std::get<Array>(_content);
		if (p_index >= vec.size())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - Array index out of range.");
		}

		return vec[p_index];
	}

	const DataBufferLayout::Element &DataBufferLayout::Element::operator[](size_t p_index) const
	{
		if (!isArray())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - This element is not an array.");
		}

		const auto &vec = std::get<Array>(_content);
		if (p_index >= vec.size())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - Array index out of range.");
		}

		return vec[p_index];
	}

	DataBufferLayout::Element &DataBufferLayout::Element::operator[](const std::wstring &p_key)
	{
		if (!isStructure())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - This element is not a map/struct.");
		}

		auto &map = std::get<Structure>(_content);
		auto it = map.find(p_key);
		if (it == map.end())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - Key not found in map: " + spk::StringUtils::wstringToString(p_key));
		}

		return it->second;
	}

	const DataBufferLayout::Element &DataBufferLayout::Element::operator[](const std::wstring &p_key) const
	{
		if (!isStructure())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - This element is not a structure.");
		}

		const auto &map = std::get<Structure>(_content);
		auto it = map.find(p_key);
		if (it == map.end())
		{
			GENERATE_ERROR(spk::StringUtils::wstringToString(_name) + " - Key not found in structure: " + spk::StringUtils::wstringToString(p_key));
		}

		return it->second;
	}

	size_t DataBufferLayout::Element::offset() const
	{
		return _offset;
	}

	size_t DataBufferLayout::Element::size() const
	{
		return _size;
	}

	DataBufferLayout::DataBufferLayout(DataBuffer *p_buffer) :
		_root(L"Root", nullptr, 0, ((p_buffer != nullptr) ? p_buffer->size() : 0))
	{
		if (p_buffer != nullptr)
		{
			_root.setBuffer(p_buffer);
		}
	}

	DataBufferLayout::DataBufferLayout(const std::wstring &p_name, DataBuffer *p_buffer) :
		_root(p_name, nullptr, 0, ((p_buffer != nullptr) ? p_buffer->size() : 0))
	{
		if (p_buffer != nullptr)
		{
			_root.setBuffer(p_buffer);
		}
	}

	DataBufferLayout::DataBufferLayout(const DataBufferLayout &p_other) :
		_root(p_other._root)
	{
	}

	DataBufferLayout::DataBufferLayout(DataBufferLayout &&p_other) :
		_root(std::move(p_other._root))
	{
	}

	DataBufferLayout &DataBufferLayout::operator=(const DataBufferLayout &p_other)
	{
		if (this != &p_other)
		{
			_root = p_other._root;
		}
		return *this;
	}

	DataBufferLayout &DataBufferLayout::operator=(DataBufferLayout &&p_other)
	{
		if (this != &p_other)
		{
			_root = std::move(p_other._root);
		}
		return *this;
	}

	void DataBufferLayout::setBuffer(DataBuffer *p_buffer)
	{
		_root.setBuffer(p_buffer);
	}

	void DataBufferLayout::updateRootSize()
	{
		if (_root._buffer != nullptr)
		{
			_root._size = _root._buffer->size();
		}
	}

	DataBufferLayout::Element &DataBufferLayout::root()
	{
		return _root;
	}

	const DataBufferLayout::Element &DataBufferLayout::root() const
	{
		return _root;
	}

	bool DataBufferLayout::contains(const std::wstring &p_name)
	{
		return _root.contains(p_name);
	}

	DataBufferLayout::Element &DataBufferLayout::addElement(const std::wstring &p_name, size_t p_offset, size_t p_size)
	{
		return _root.addElement(p_name, p_offset, p_size);
	}

	DataBufferLayout::Element &DataBufferLayout::addElement(
		const std::wstring &p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding)
	{
		return _root.addElement(p_name, p_offset, p_nbElement, p_elementSize, p_elementPadding);
	}

	void DataBufferLayout::removeElement(const std::wstring &p_name)
	{
		_root.removeElement(p_name);
	}

	DataBufferLayout::Element &DataBufferLayout::operator[](size_t p_index)
	{
		return _root[p_index];
	}

	DataBufferLayout::Element &DataBufferLayout::operator[](const std::wstring &p_key)
	{
		return _root[p_key];
	}

	const DataBufferLayout::Element &DataBufferLayout::operator[](size_t p_index) const
	{
		return _root[p_index];
	}

	const DataBufferLayout::Element &DataBufferLayout::operator[](const std::wstring &p_key) const
	{
		return _root[p_key];
	}
}
