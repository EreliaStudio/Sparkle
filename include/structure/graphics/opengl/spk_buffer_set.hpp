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
		VertexArrayObject _vao;
		LayoutBufferObject _layout;
		IndexBufferObject _indexes;

	public:
		BufferSet() = default;

		explicit BufferSet(std::span<const LayoutBufferObject::Attribute> attributes);
		BufferSet(std::initializer_list<LayoutBufferObject::Attribute> attributes);

		template <AttributeContainer Container>
		explicit BufferSet(const Container &attributes) :
			BufferSet(std::span(attributes.data(), attributes.size()))
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
