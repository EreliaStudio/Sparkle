#include "structure/graphics/opengl/spk_binded_buffer_object.hpp"

namespace spk::OpenGL
{
	BindedBufferObject::Element::Element(uint8_t *buffer, size_t size)
		: _buffer(buffer), _size(size)
	{
	}

	uint8_t *BindedBufferObject::Element::data() { return _buffer; }
	size_t BindedBufferObject::Element::size() const { return _size; }

	size_t BindedBufferObject::Element::nbElement() const
	{
		if (std::holds_alternative<std::vector<Element>>(_content))
			return std::get<std::vector<Element>>(_content).size();
		else if (std::holds_alternative<std::unordered_map<std::string, Element>>(_content))
			return std::get<std::unordered_map<std::string, Element>>(_content).size();
		return 0;
	}

	BindedBufferObject::Element &BindedBufferObject::Element::operator[](size_t index)
	{
		auto &arr = std::get<std::vector<Element>>(_content);
		if (index >= arr.size())
			throw std::out_of_range("Index out of range");
		return arr[index];
	}

	const BindedBufferObject::Element &BindedBufferObject::Element::operator[](size_t index) const
	{
		const auto &arr = std::get<std::vector<Element>>(_content);
		if (index >= arr.size())
			throw std::out_of_range("Index out of range");
		return arr[index];
	}

	BindedBufferObject::Element &BindedBufferObject::Element::operator[](const std::string &name)
	{
		auto &map = std::get<std::unordered_map<std::string, Element>>(_content);
		auto it = map.find(name);
		if (it == map.end())
			throw std::out_of_range("No element with name: " + name);
		return it->second;
	}

	const BindedBufferObject::Element &BindedBufferObject::Element::operator[](const std::string &name) const
	{
		const auto &map = std::get<std::unordered_map<std::string, Element>>(_content);
		auto it = map.find(name);
		if (it == map.end())
			throw std::out_of_range("No element with name: " + name);
		return it->second;
	}

	BindedBufferObject::BindedBufferObject(Type p_type, Usage p_usage, const std::string &p_name, BindingPoint p_bindingPoint, size_t p_size) :
		VertexBufferObject(p_type, p_usage),
		_name(p_name),
		_bindingPoint(p_bindingPoint),
		_blockSize(p_size)
	{
		resize(p_size);
	}

	void BindedBufferObject::activate()
	{
		VertexBufferObject::activate();
	}

	BindedBufferObject::Element &BindedBufferObject::operator[](const std::string &name)
	{
		auto it = _elements.find(name);
		if (it == _elements.end())
			throw std::out_of_range("No element with name: " + name);
		return it->second;
	}

	const BindedBufferObject::Element &BindedBufferObject::operator[](const std::string &name) const
	{
		auto it = _elements.find(name);
		if (it == _elements.end())
			throw std::out_of_range("No element with name: " + name);
		return it->second;
	}

	BindedBufferObject::Element &BindedBufferObject::addElement(const std::string &name, size_t offset, size_t elementSize, size_t arraySize)
	{
		uint8_t *bufferPtr = static_cast<uint8_t *>(data()) + offset;

		if (_elements.find(name) != _elements.end())
			throw std::runtime_error("Element '" + name + "' already exists.");

		if (arraySize > 0)
		{
			std::vector<Element> elements;
			elements.reserve(arraySize);
			for (size_t i = 0; i < arraySize; ++i)
			{
				elements.emplace_back(Element(bufferPtr + (i * elementSize), elementSize));
			}
			Element arrayElement(bufferPtr, elementSize);
			arrayElement._content = std::move(elements);

			auto [it, inserted] = _elements.emplace(name, std::move(arrayElement));
			return it->second;
		}
		else
		{
			auto [it, inserted] = _elements.emplace(name, Element(bufferPtr, elementSize));
			return it->second;
		}
	}

}