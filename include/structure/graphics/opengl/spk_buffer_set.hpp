#pragma once

#include "spk_index_buffer_object.hpp"
#include "spk_layout_buffer_object.hpp"
#include "spk_vertex_array_object.hpp"

template <typename Container>
concept AttributeContainer = requires(const Container &c) {
	{ std::data(c) } -> std::convertible_to<const spk::OpenGL::LayoutBufferObject::Attribute *>;
	{ std::size(c) } -> std::convertible_to<std::size_t>;
};

namespace spk::OpenGL
{
	class BufferSet
	{
	private:
VertexArrayObject _vertexArrayObject;
LayoutBufferObject _layout;
IndexBufferObject _indexBufferObject;

	public:
		BufferSet() = default;

		explicit BufferSet(std::span<const LayoutBufferObject::Attribute> p_attributes);
		BufferSet(std::initializer_list<LayoutBufferObject::Attribute> p_attributes);

		template <AttributeContainer Container>
		explicit BufferSet(const Container &p_attributes) :
			BufferSet(std::span(p_attributes.data(), p_attributes.size()))
		{
		}

		LayoutBufferObject &layout();
		IndexBufferObject &indexes();

		const LayoutBufferObject &layout() const;
		const IndexBufferObject &indexes() const;

		void clear();
		void validate();

		void activate();
		void deactivate();
	};
}
