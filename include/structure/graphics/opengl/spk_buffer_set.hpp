#pragma once

#include "spk_vertex_array_object.hpp"
#include "spk_index_buffer_object.hpp"
#include "spk_layout_buffer_object.hpp"

namespace spk::OpenGL
{
	class BufferSet
	{
	public:
		class Factory
		{
		private:
			OpenGL::LayoutBufferObject::Factory _layoutFactory;

		public:
			void insert(OpenGL::LayoutBufferObject::Attribute::Index p_index, size_t p_size, OpenGL::LayoutBufferObject::Attribute::Type p_type);
			BufferSet construct() const;
		};

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
