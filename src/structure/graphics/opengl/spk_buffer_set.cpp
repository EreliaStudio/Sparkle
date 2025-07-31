#include "structure/graphics/opengl/spk_buffer_set.hpp"

#include "spk_debug_macro.hpp"

namespace spk::OpenGL
{
	BufferSet::BufferSet(std::span<const LayoutBufferObject::Attribute> p_attributes) :
		_layout(p_attributes)
	{
	}

	BufferSet::BufferSet(std::initializer_list<LayoutBufferObject::Attribute> p_attributes) :
		BufferSet(std::span(p_attributes.begin(), p_attributes.end()))
	{
	}

	LayoutBufferObject &BufferSet::layout()
	{
		return (_layout);
	}

	IndexBufferObject &BufferSet::indexes()
	{
		return (_indexBufferObject);
	}

	const LayoutBufferObject &BufferSet::layout() const
	{
		return (_layout);
	}

	const IndexBufferObject &BufferSet::indexes() const
	{
		return (_indexBufferObject);
	}

	void BufferSet::clear()
	{
		_layout.clear();
		_indexBufferObject.clear();
	}

	void BufferSet::validate()
	{
		_layout.validate();
		_indexBufferObject.validate();
	}

	void BufferSet::activate()
	{
		_vertexArrayObject.activate();
		_layout.activate();
		_indexBufferObject.activate();
	}

	void BufferSet::deactivate()
	{
		_vertexArrayObject.deactivate();
		_layout.deactivate();
		_indexBufferObject.deactivate();
	}
}
