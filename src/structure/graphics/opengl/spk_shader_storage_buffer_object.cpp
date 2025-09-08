#include "structure/graphics/opengl/spk_shader_storage_buffer_object.hpp"

#include "structure/container/spk_json_object.hpp"
#include "utils/spk_string_utils.hpp"

namespace spk::OpenGL
{
	ShaderStorageBufferObject::DynamicArray::DynamicArray(
		const std::wstring &p_name, spk::DataBuffer *p_buffer, size_t p_fixedReservedSpace, size_t p_elementSize, size_t p_elementPadding) :
		_buffer(p_buffer),
		_defaultElement(p_name, p_buffer, 0, p_elementSize),
		_fixedReservedSpace(p_fixedReservedSpace),
		_elementSize(p_elementSize),
		_elementPadding(p_elementPadding)
	{
	}

	ShaderStorageBufferObject::DynamicArray::DynamicArray(DynamicArray &&p_other) :
		_resizeContractProvider(std::move(p_other._resizeContractProvider)),
		_buffer(std::move(p_other._buffer)),
		_elements(std::move(p_other._elements)),
		_defaultElement(std::move(p_other._defaultElement)),
		_fixedReservedSpace(p_other._fixedReservedSpace),
		_elementSize(p_other._elementSize),
		_elementPadding(p_other._elementPadding)
	{
	}

	ShaderStorageBufferObject::DynamicArray &ShaderStorageBufferObject::DynamicArray::operator=(DynamicArray &&p_other)
	{
		if (this != &p_other)
		{
			_buffer = std::move(p_other._buffer);
			_elements = std::move(p_other._elements);
			_defaultElement = std::move(p_other._defaultElement);
			_defaultElement.setBuffer(_buffer);
			_fixedReservedSpace = p_other._fixedReservedSpace;
			_elementSize = p_other._elementSize;
			_elementPadding = p_other._elementPadding;
		}
		return *this;
	}

	void ShaderStorageBufferObject::DynamicArray::_redoArray()
	{
		for (size_t i = 0; i < _elements.size(); ++i)
		{
			auto &element = _elements[i];
			element = _defaultElement.duplicate(_fixedReservedSpace + (_elementSize + _elementPadding) * i);
			element.setName(_defaultElement.name() + L"[" + std::to_wstring(i) + L"]");
			element.setBuffer(_buffer);
		}
	}

	ShaderStorageBufferObject::DynamicArray ShaderStorageBufferObject::DynamicArray::_duplicate() const
	{
		DynamicArray result(_defaultElement.name(), _buffer, _fixedReservedSpace, _elementSize, _elementPadding);
		result._elements.resize(_elements.size());
		return result;
	}

	bool ShaderStorageBufferObject::DynamicArray::contains(const std::wstring &p_name)
	{
		return _defaultElement.contains(p_name);
	}

	ShaderStorageBufferObject::DynamicArray::Element &ShaderStorageBufferObject::DynamicArray::addElement(
		const std::wstring &p_name, size_t p_offset, size_t p_size)
	{
		auto &result = _defaultElement.addElement(p_name, p_offset, p_size);
		_redoArray();
		return result;
	}

	ShaderStorageBufferObject::DynamicArray::Element &ShaderStorageBufferObject::DynamicArray::addElement(
		const std::wstring &p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding)
	{
		auto &result = _defaultElement.addElement(p_name, p_offset, p_nbElement, p_elementSize, p_elementPadding);
		_redoArray();
		return result;
	}

	void ShaderStorageBufferObject::DynamicArray::removeElement(const std::wstring &p_name)
	{
		_defaultElement.removeElement(p_name);
		_redoArray();
	}

	void ShaderStorageBufferObject::DynamicArray::clear()
	{
		resize(0);
	}

	void ShaderStorageBufferObject::DynamicArray::resize(size_t p_nbElement)
	{
		_resizeContractProvider.trigger(_fixedReservedSpace + (_elementSize + _elementPadding) * p_nbElement);
		_elements.resize(p_nbElement);
		_redoArray();
	}

	ShaderStorageBufferObject::DynamicArray::Element &ShaderStorageBufferObject::DynamicArray::operator[](size_t p_index)
	{
		return _elements[p_index];
	}

	const ShaderStorageBufferObject::DynamicArray::Element &ShaderStorageBufferObject::DynamicArray::operator[](size_t p_index) const
	{
		return _elements[p_index];
	}

	size_t ShaderStorageBufferObject::DynamicArray::nbElement() const
	{
		return _elements.size();
	}

	size_t ShaderStorageBufferObject::DynamicArray::elementSize() const
	{
		return (_elementSize);
	}

	ShaderStorageBufferObject::ShaderStorageBufferObject() :
		VertexBufferObject(VertexBufferObject::Type::ShaderStorage, VertexBufferObject::Usage::Dynamic),
		_blockName(L"Unnamed SSBO"),
		_bindingPoint(-1),
		_blockIndex(-1),
		_fixedData(L"FixedData", &dataBuffer(), 0, 0),
		_dynamicArray(L"DynamicArray", &dataBuffer(), 0, 0, 0)
	{
		_onResizeContract = _dynamicArray._resizeContractProvider.subscribe([&](size_t p_size) {
			resize(p_size);
		});
		_dynamicArray._buffer = &dataBuffer();
	}

	ShaderStorageBufferObject::ShaderStorageBufferObject(
		const std::wstring &p_blockName,
		BindingPoint p_bindingPoint,
		size_t p_fixedSize,
		size_t p_paddingFixedToDynamic,
		size_t p_dynamicElementSize,
		size_t p_dynamicElementPadding) :
		VertexBufferObject(VertexBufferObject::Type::ShaderStorage, VertexBufferObject::Usage::Dynamic),
		_blockName(p_blockName),
		_bindingPoint(p_bindingPoint),
		_blockIndex(-1),
		_fixedData(p_blockName + L" - FixedData", &dataBuffer(), 0, p_fixedSize),
		_dynamicArray(
			p_blockName + L" - DynamicArray", &dataBuffer(), p_fixedSize + p_paddingFixedToDynamic, p_dynamicElementSize, p_dynamicElementPadding)
	{
		resize(p_fixedSize + p_paddingFixedToDynamic + p_dynamicElementSize);
		_onResizeContract = _dynamicArray._resizeContractProvider.subscribe([&](size_t p_size) { resize(p_size); });
		_dynamicArray._buffer = &dataBuffer();
		_dynamicArray._redoArray();
	}

	ShaderStorageBufferObject::ShaderStorageBufferObject(const spk::JSON::Object &p_desc) :
		ShaderStorageBufferObject(
			p_desc[L"BlockName"].as<std::wstring>(),
			static_cast<BindingPoint>(p_desc[L"BindingPoint"].as<long>()),
			static_cast<size_t>(p_desc[L"FixedSize"].as<long>()),
			static_cast<size_t>(p_desc[L"PaddingFixedToDynamic"].as<long>()),
			static_cast<size_t>(p_desc[L"DynamicElementSize"].as<long>()),
			static_cast<size_t>(p_desc[L"DynamicElementPadding"].as<long>()))
	{
		if (p_desc.contains(L"FixedElements"))
		{
			for (auto &elem : p_desc[L"FixedElements"].asArray())
			{
				_loadElement(fixedData(), *elem);
			}
		}

		if (p_desc.contains(L"DynamicElementComposition"))
		{
			for (auto &elem : p_desc[L"DynamicElementComposition"].asArray())
			{
				_loadElement(dynamicArray(), *elem);
			}
		}

		if (p_desc.contains(L"InitialDynamicCount"))
		{
			dynamicArray().resize(static_cast<size_t>(p_desc[L"InitialDynamicCount"].as<long>()));
		}
	}

	ShaderStorageBufferObject::ShaderStorageBufferObject(const ShaderStorageBufferObject &p_other) :
		VertexBufferObject(p_other),
		_blockName(p_other._blockName),
		_bindingPoint(p_other._bindingPoint),
		_blockIndex(-1),
		_fixedData(p_other._fixedData),
		_dynamicArray(p_other._dynamicArray._duplicate())
	{
		resize(p_other.size());
		_onResizeContract = _dynamicArray._resizeContractProvider.subscribe([&](size_t p_size) { resize(p_size); });
		_dynamicArray._buffer = &dataBuffer();
		_dynamicArray._redoArray();
	}

	ShaderStorageBufferObject &ShaderStorageBufferObject::operator=(const ShaderStorageBufferObject &p_other)
	{
		if (this != &p_other)
		{
			VertexBufferObject::operator=(p_other);
			_blockName = p_other._blockName;
			_bindingPoint = p_other._bindingPoint;
			_blockIndex = -1;
			_fixedData = p_other._fixedData;
			_dynamicArray = p_other._dynamicArray._duplicate();
			resize(p_other.size());
			_onResizeContract = _dynamicArray._resizeContractProvider.subscribe([&](size_t p_size) { resize(p_size); });
			_dynamicArray._buffer = &dataBuffer();
			_dynamicArray._redoArray();
		}
		return *this;
	}

	ShaderStorageBufferObject::ShaderStorageBufferObject(ShaderStorageBufferObject &&p_other) :
		VertexBufferObject(std::move(p_other)),
		_blockName(std::move(p_other._blockName)),
		_bindingPoint(p_other._bindingPoint),
		_blockIndex(p_other._blockIndex),
		_fixedData(std::move(p_other._fixedData)),
		_dynamicArray(std::move(p_other._dynamicArray))
	{
		resize(p_other.size());
		_onResizeContract = _dynamicArray._resizeContractProvider.subscribe([&](size_t p_size) { resize(p_size); });
		_dynamicArray._buffer = &dataBuffer();
		_dynamicArray._redoArray();
	}

	ShaderStorageBufferObject &ShaderStorageBufferObject::operator=(ShaderStorageBufferObject &&p_other)
	{
		if (this != &p_other)
		{
			_blockName = std::move(p_other._blockName);
			_bindingPoint = p_other._bindingPoint;
			_blockIndex = p_other._blockIndex;
			_fixedData = std::move(p_other._fixedData);
			_dynamicArray = std::move(p_other._dynamicArray);
			resize(p_other.size());
			_onResizeContract = _dynamicArray._resizeContractProvider.subscribe([&](size_t p_size) { resize(p_size); });
			_dynamicArray._buffer = &dataBuffer();
			_dynamicArray._redoArray();
		}
		return *this;
	}

	const std::wstring &ShaderStorageBufferObject::blockName() const
	{
		return _blockName;
	}

	void ShaderStorageBufferObject::setBlockName(const std::wstring &p_blockName)
	{
		_blockName = p_blockName;
	}

	ShaderStorageBufferObject::BindingPoint ShaderStorageBufferObject::bindingPoint() const
	{
		return _bindingPoint;
	}

	void ShaderStorageBufferObject::setBindingPoint(BindingPoint p_bindingPoint)
	{
		_bindingPoint = p_bindingPoint;
	}

	spk::DataBufferLayout::Element &ShaderStorageBufferObject::fixedData()
	{
		return _fixedData;
	}

	const spk::DataBufferLayout::Element &ShaderStorageBufferObject::fixedData() const
	{
		return _fixedData;
	}

	ShaderStorageBufferObject::DynamicArray &ShaderStorageBufferObject::dynamicArray()
	{
		return _dynamicArray;
	}

	const ShaderStorageBufferObject::DynamicArray &ShaderStorageBufferObject::dynamicArray() const
	{
		return _dynamicArray;
	}

	void ShaderStorageBufferObject::activate()
	{
		VertexBufferObject::activate();

		GLint prog = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
		if (prog == 0)
		{
			GENERATE_ERROR("No shader program is currently bound.");
		}

		if (_blockIndex == -1)
		{
			std::string str = spk::StringUtils::wstringToString(_blockName);
			_blockIndex = glGetProgramResourceIndex(prog, GL_SHADER_STORAGE_BLOCK, str.c_str());
			if (_blockIndex == GL_INVALID_INDEX)
			{
				GENERATE_ERROR("Shader storage block '" + str + "' not found in the shader program.");
			}
		}

		glShaderStorageBlockBinding(prog, _blockIndex, _bindingPoint);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _bindingPoint, _id);
	}

	void ShaderStorageBufferObject::_loadElement(spk::DataBufferLayout::Element &p_parent, const spk::JSON::Object &p_elemDesc)
	{
		std::wstring name = p_elemDesc[L"Name"].as<std::wstring>();
		size_t offset = p_parent.offset() + static_cast<size_t>(p_elemDesc[L"Offset"].as<long>());

		if (p_elemDesc.contains(L"Size"))
		{
			size_t size = static_cast<size_t>(p_elemDesc[L"Size"].as<long>());
			auto &elem = p_parent.addElement(name, offset, size);

			if (p_elemDesc.contains(L"NestedElements"))
			{
				for (auto &child : p_elemDesc[L"NestedElements"].asArray())
				{
					_loadElement(elem, *child);
				}
			}
		}
		else if (p_elemDesc.contains(L"NbElements"))
		{
			size_t nbElem = static_cast<size_t>(p_elemDesc[L"NbElements"].as<long>());
			size_t elemSize = static_cast<size_t>(p_elemDesc[L"ElementSize"].as<long>());
			size_t padding = static_cast<size_t>(p_elemDesc[L"ElementPadding"].as<long>());

			auto &elemArr = p_parent.addElement(name, offset, nbElem, elemSize, padding);

			if (p_elemDesc.contains(L"ElementComposition"))
			{
				for (size_t i = 0; i < elemArr.nbElement(); ++i)
				{
					for (auto &comp : p_elemDesc[L"ElementComposition"].asArray())
					{
						_loadElement(elemArr[i], *comp);
					}
				}
			}
		}
		else
		{
			GENERATE_ERROR("[ShaderStorageBufferObject] - Element description must contain either 'Size' or 'NbElements'.");
		}
	}

	void ShaderStorageBufferObject::_loadElement(DynamicArray &p_array, const spk::JSON::Object &p_elemDesc)
	{
		std::wstring name = p_elemDesc[L"Name"].as<std::wstring>();
		size_t offset = static_cast<size_t>(p_elemDesc[L"Offset"].as<long>());

		if (p_elemDesc.contains(L"Size"))
		{
			size_t size = static_cast<size_t>(p_elemDesc[L"Size"].as<long>());
			auto &elem = p_array.addElement(name, offset, size);

			if (p_elemDesc.contains(L"NestedElements"))
			{
				for (auto &child : p_elemDesc[L"NestedElements"].asArray())
				{
					_loadElement(elem, *child);
				}
			}
		}
		else if (p_elemDesc.contains(L"NbElements"))
		{
			size_t nbElem = static_cast<size_t>(p_elemDesc[L"NbElements"].as<long>());
			size_t elemSize = static_cast<size_t>(p_elemDesc[L"ElementSize"].as<long>());
			size_t padding = static_cast<size_t>(p_elemDesc[L"ElementPadding"].as<long>());

			auto &elemArr = p_array.addElement(name, offset, nbElem, elemSize, padding);

			if (p_elemDesc.contains(L"ElementComposition"))
			{
				for (size_t i = 0; i < elemArr.nbElement(); ++i)
				{
					for (auto &comp : p_elemDesc[L"ElementComposition"].asArray())
					{
						_loadElement(elemArr[i], *comp);
					}
				}
			}
		}
		else
		{
			GENERATE_ERROR("[ShaderStorageBufferObject] - Element description must contain either 'Size' or 'NbElements'.");
		}
	}
}