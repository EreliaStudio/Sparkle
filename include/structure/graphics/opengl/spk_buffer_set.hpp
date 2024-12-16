#pragma once

#include "spk_vertex_array_object.hpp"
#include "spk_index_buffer_object.hpp"
#include "spk_layout_buffer_object.hpp"

namespace spk::OpenGL
{
	class BufferSet
	{
	private:
		VertexArrayObject _vao;
		LayoutBufferObject _layout;
		IndexBufferObject _indexes;

	public:
		LayoutBufferObject& layout();
		IndexBufferObject& indexes();

		const LayoutBufferObject& layout() const;
		const IndexBufferObject& indexes() const;

		void activate();
		void deactivate();
	};
}
