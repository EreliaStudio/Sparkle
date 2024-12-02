#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <vector>
#include <span>
#include <stdexcept>
#include <string>

#include "spk_vertex_buffer_object.hpp"

namespace spk::OpenGL
{
	/**
	 * @class LayoutBufferObject
	 * @brief Represents a specialized Vertex Buffer Object (VBO) with attributes for defining vertex layouts.
	 * 
	 * This class extends VertexBufferObject to include the ability to define and apply layouts for vertex attributes.
	 * 
	 * Example usage:
	 * @code
	 * spk::OpenGL::LayoutBufferObject lbo;
	 * spk::OpenGL::LayoutBufferObject::Factory factory;
	 * factory.insert(0, 3, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float); // Position attribute
	 * factory.insert(1, 3, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float); // Normal attribute
	 * lbo = factory.construct();
	 * 
	 * std::vector<float> vertexData = { ... };
	 * lbo << vertexData; // Upload data
	 * lbo.validate();	// Validating the data before rendering
	 * @endcode
	 */
	class LayoutBufferObject : public VertexBufferObject
	{
	public:
		/**
		 * @struct Attribute
		 * @brief Defines a vertex attribute with index, size, and data type.
		 */
		struct Attribute
		{
			using Index = GLuint;

			/**
			 * @enum Type
			 * @brief Enumeration of supported attribute data types.
			 */
			enum class Type
			{
				Byte = GL_BYTE,		   ///< Signed byte.
				UByte = GL_UNSIGNED_BYTE, ///< Unsigned byte.
				Int = GL_INT,			 ///< Signed integer.
				Float = GL_FLOAT,		 ///< Floating-point number.
				UInt = GL_UNSIGNED_INT	///< Unsigned integer.
			};

			Index index; ///< Index of the attribute in the shader program.
			size_t size; ///< Number of components in the attribute (e.g., 3 for vec3).
			Type type; ///< Data type of the attribute.

			/**
			 * @brief Default constructor.
			 */
			Attribute();

			/**
			 * @brief Constructs an Attribute with specified parameters.
			 * @param p_index Index of the attribute.
			 * @param p_size Number of components in the attribute.
			 * @param p_type Data type of the attribute.
			 */
			Attribute(Index p_index, size_t p_size, Type p_type);

			/**
			 * @brief Computes the size in bytes of the specified attribute type.
			 * @param p_type The attribute type.
			 * @return The size of the attribute type in bytes.
			 */
			static size_t typeSize(Type p_type);
		};

		/**
		 * @class Factory
		 * @brief Builder class for constructing LayoutBufferObject instances with defined attributes.
		 */
		class Factory
		{
		private:
			std::vector<Attribute> _attributes; ///< List of attributes to be added.

		public:
			/**
			 * @brief Inserts a new attribute into the layout.
			 * @param p_index Index of the attribute.
			 * @param p_size Number of components in the attribute.
			 * @param p_type Data type of the attribute.
			 */
			void insert(Attribute::Index p_index, size_t p_size, Attribute::Type p_type);

			/**
			 * @brief Constructs a LayoutBufferObject with the configured attributes.
			 * @return A new LayoutBufferObject instance.
			 */
			LayoutBufferObject construct() const;
		};

	private:
		std::vector<Attribute> _attributesToApply; ///< Attributes to be applied to the buffer.
		size_t _vertexSize; ///< Size of a single vertex in bytes.

		/**
		 * @brief Adds an attribute to the internal list.
		 * @param p_attribute The attribute to add.
		 */
		void _insertAttribute(const Attribute& p_attribute);

		/**
		 * @brief Applies the configured attributes to the buffer.
		 */
		void _applyAttributes();

	public:
		/**
		 * @brief Default constructor.
		 */
		LayoutBufferObject();

		/**
		 * @brief Copy constructor.
		 * @param p_other The object to copy.
		 */
		LayoutBufferObject(const LayoutBufferObject& p_other);

		/**
		 * @brief Copy assignment operator.
		 * @param p_other The object to assign.
		 * @return A reference to the assigned object.
		 */
		LayoutBufferObject& operator=(const LayoutBufferObject& p_other);

		/**
		 * @brief Move constructor.
		 * @param p_other The object to move.
		 */
		LayoutBufferObject(LayoutBufferObject&& p_other) noexcept;

		/**
		 * @brief Move assignment operator.
		 * @param p_other The object to assign.
		 * @return A reference to the assigned object.
		 */
		LayoutBufferObject& operator=(LayoutBufferObject&& p_other) noexcept;

		/**
		 * @brief Activates the buffer and applies the vertex layout.
		 * Overrides the base implementation.
		 */
		virtual void activate() override;

		/**
		 * @brief Overload of the insertion operator for appending a single element.
		 * @tparam TType Type of the data to append.
		 * @param data The element to append.
		 * @return A reference to this object.
		 */
		template<typename TType>
		LayoutBufferObject& operator<<(const TType& data)
		{
			append(std::span<const TType>(&data, 1));
			return *this;
		}

		/**
		 * @brief Overload of the insertion operator for appending a vector of elements.
		 * @tparam TType Type of the data to append.
		 * @param data The vector of elements to append.
		 * @return A reference to this object.
		 */
		template<typename TType>
		LayoutBufferObject& operator<<(const std::vector<TType>& data)
		{
			append(std::span<const TType>(data));
			return *this;
		}

		/**
		 * @brief Overload of the insertion operator for appending a span of elements.
		 * @tparam TType Type of the data to append.
		 * @param data The span of elements to append.
		 * @return A reference to this object.
		 */
		template<typename TType>
		LayoutBufferObject& operator<<(const std::span<TType>& data)
		{
			append(data);
			return *this;
		}

		/**
		 * @brief Appends a single element to the buffer.
		 * @tparam TType Type of the data to append.
		 * @param data The element to append.
		 */
		template<typename TType>
		void append(const TType& data)
		{
			append(std::span<const TType>(&data, 1));
		}

		/**
		 * @brief Appends a vector of elements to the buffer.
		 * @tparam TType Type of the data to append.
		 * @param data The vector of elements to append.
		 */
		template<typename TType>
		void append(const std::vector<TType>& data)
		{
			append(std::span<const TType>(data));
		}

		/**
		 * @brief Appends a span of elements to the buffer.
		 * @tparam TType Type of the data to append.
		 * @param data The span of elements to append.
		 * @throws std::runtime_error If the size of TType does not match the vertex size.
		 */
		template<typename TType>
		void append(const std::span<TType>& data)
		{
			if (sizeof(TType) != _vertexSize)
			{
				throw std::runtime_error("Size mismatch inside LayoutBufferObject: Expected vertex size is " +
					std::to_string(_vertexSize) + " bytes, but received " +
					std::to_string(sizeof(TType)) + " bytes.");
			}
			VertexBufferObject::append(data.data(), data.size() * sizeof(TType));
		}
	};
}
