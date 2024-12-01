#include "structure/graphics/opengl/spk_buffer_set.hpp"

namespace spk::OpenGL
{
	void BufferSet::Factory::insert(OpenGL::LayoutBufferObject::Attribute::Index p_index, size_t p_size, OpenGL::LayoutBufferObject::Attribute::Type p_type)
	{
		_layoutFactory.insert(p_index, p_size, p_type);
	}

	BufferSet BufferSet::Factory::construct() const
	{
		BufferSet result;
		result._layout = _layoutFactory.construct();
		return result;
	}

	LayoutBufferObject& BufferSet::layout()
	{
		return _layout;
	}

	IndexBufferObject& BufferSet::indexes()
	{
		return _indexes;
	}

	const LayoutBufferObject& BufferSet::layout() const
	{
		return _layout;
	}

	const IndexBufferObject& BufferSet::indexes() const
	{
		return _indexes;
	}

	void BufferSet::activate()
	{
		_vao.activate();
		_layout.activate();
		_indexes.activate();
	}

	void BufferSet::deactivate()
	{
		_vao.deactivate();
		_layout.deactivate();
		_indexes.deactivate();
	}
}
