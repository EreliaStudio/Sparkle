#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{   
	Pipeline::UniformObject::Element::Element(void *p_data, const Structure* p_structure) : 
		_data(p_data),
		_structure(p_structure)
	{
		for (const auto& [key, element] : _structure->elements)
		{
			_elements.emplace(key, Element(static_cast<char *>(_data) + element.offsetWithPadding, element.pointer));
		}
	}

	Pipeline::UniformObject::Element&Pipeline::UniformObject::Element::operator[](const std::string& p_elementName)
	{
		if (_elements.contains(p_elementName) == false)
			throwException("Element [" + p_elementName + "] doesn't exist");
		return (_elements.at(p_elementName));
	}

	Pipeline::UniformObject::UniformObject(const GLuint& p_program, const Pipeline::UniformObject::Layout& p_layout) :
		_needUpdate(false),
		_handle(p_program, p_layout.type, p_layout.binding),
		_buffer(p_layout.structure->sizeWithPadding),
		_generalElement(_buffer.data(), p_layout.structure)
	{
		std::memset(_buffer.data(), 0, _buffer.size());
	}

	Pipeline::UniformObject::Element& Pipeline::UniformObject::operator[](const std::string& p_elementName)
	{
		return (_generalElement[p_elementName]);
	}

	const std::vector<uint8_t>& Pipeline::UniformObject::buffer() const
	{
		return (_buffer);
	}

	void Pipeline::UniformObject::update()
	{
		_needUpdate = true;
	}

	bool Pipeline::UniformObject::needUpdate() const
	{
		return (_needUpdate);
	}

	void Pipeline::UniformObject::push()
	{
		_handle.push(_buffer.data(), _buffer.size());
		_needUpdate = false;
	}

	void Pipeline::UniformObject::activate()
	{
		if (_needUpdate == true)
			push();
		_handle.activate();
	}

	void Pipeline::UniformObject::deactivate()
	{
		_handle.deactivate();
	}
}