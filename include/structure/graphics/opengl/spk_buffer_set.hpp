#pragma once

#include "spk_vertex_array_object.hpp"
#include "spk_index_buffer_object.hpp"
#include "spk_layout_buffer_object.hpp"

namespace spk::OpenGL
{
	/**
	 * @class BufferSet
	 * @brief Combines a Vertex Array Object (VAO), Layout Buffer Object (LBO), and Index Buffer Object (IBO) into a single set.
	 * 
	 * The BufferSet simplifies managing and using OpenGL buffers for rendering. It encapsulates the VAO for attribute configuration,
	 * the LBO for vertex data, and the IBO for indexed rendering.
	 * 
	 * Example usage:
	 * @code
	 * spk::OpenGL::BufferSet::Factory factory;
	 * factory.insert(0, 3, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float); // Position attribute
	 * factory.insert(1, 3, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float); // Normal attribute
	 * spk::OpenGL::BufferSet bufferSet = factory.construct();
	 * 
	 * bufferSet.layout() << vertexData; // Upload vertex data
	 * bufferSet.indexes() << indexData; // Upload index data
	 * 
	 * bufferSet.layout().validate();
	 * bufferSet.indexes().validate();
	 * @endcode
	 */
	class BufferSet
	{
	public:
		/**
		 * @class Factory
		 * @brief Facilitates the creation of BufferSet instances.
		 * 
		 * The Factory class provides a way to define layout attributes for the LayoutBufferObject
		 * and create a complete BufferSet instance.
		 */
		class Factory
		{
		private:
			OpenGL::LayoutBufferObject::Factory _layoutFactory; ///< Factory for defining layout attributes.

		public:
			/**
			 * @brief Inserts a layout attribute into the LayoutBufferObject.
			 * @param p_index The attribute index in the shader program.
			 * @param p_size The number of components in the attribute.
			 * @param p_type The data type of the attribute.
			 */
			void insert(OpenGL::LayoutBufferObject::Attribute::Index p_index, size_t p_size, OpenGL::LayoutBufferObject::Attribute::Type p_type);

			/**
			 * @brief Constructs a BufferSet with the configured layout attributes.
			 * @return A new BufferSet instance.
			 */
			BufferSet construct() const;
		};

	private:
		VertexArrayObject _vao; ///< Vertex Array Object for managing attribute configurations.
		LayoutBufferObject _layout; ///< Layout Buffer Object for vertex data.
		IndexBufferObject _indexes; ///< Index Buffer Object for indexed rendering.

	public:
		/**
		 * @brief Provides access to the LayoutBufferObject for modifications.
		 * @return A reference to the LayoutBufferObject.
		 */
		LayoutBufferObject& layout();

		/**
		 * @brief Provides access to the IndexBufferObject for modifications.
		 * @return A reference to the IndexBufferObject.
		 */
		IndexBufferObject& indexes();

		/**
		 * @brief Provides constant access to the LayoutBufferObject.
		 * @return A constant reference to the LayoutBufferObject.
		 */
		const LayoutBufferObject& layout() const;

		/**
		 * @brief Provides constant access to the IndexBufferObject.
		 * @return A constant reference to the IndexBufferObject.
		 */
		const IndexBufferObject& indexes() const;

		/**
		 * @brief Activates the BufferSet, binding the VAO, LBO, and IBO for rendering.
		 */
		void activate();

		/**
		 * @brief Deactivates the BufferSet, unbinding the VAO, LBO, and IBO.
		 */
		void deactivate();
	};
}
