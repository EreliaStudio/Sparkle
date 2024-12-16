#include "structure/graphics/opengl/spk_buffer_set.hpp"

namespace spk::OpenGL
{
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
