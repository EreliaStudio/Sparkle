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
	class LayoutBufferObject : public VertexBufferObject
	{
	public:
		struct Attribute
		{
			using Index = GLuint;

			enum class Type
			{
				Byte = GL_BYTE,
				UByte = GL_UNSIGNED_BYTE,
				Int = GL_INT,
				Float = GL_FLOAT,
				UInt = GL_UNSIGNED_INT
			};

			Index index;
			size_t size;
			Type type;

			Attribute();
			Attribute(Index p_index, size_t p_size, Type p_type);

			static size_t typeSize(Type p_type);
		};

		class Factory
		{
		private:
			std::vector<Attribute> _attributes;

		public:
			void insert(Attribute::Index p_index, size_t p_size, Attribute::Type p_type);
			LayoutBufferObject construct() const;
		};

	private:
		std::vector<Attribute> _attributesToApply;
		size_t _vertexSize;

		void _insertAttribute(const Attribute& p_attribute);
		void _applyAttributes();

	public:
		LayoutBufferObject();
		LayoutBufferObject(const LayoutBufferObject& p_other);
		LayoutBufferObject& operator=(const LayoutBufferObject& p_other);
		LayoutBufferObject(LayoutBufferObject&& p_other) noexcept;
		LayoutBufferObject& operator=(LayoutBufferObject&& p_other) noexcept;

		virtual void activate() override;

		template<typename TType>
		LayoutBufferObject& operator<<(const TType& data)
		{
			append(std::span<const TType>(&data, 1));
			return *this;
		}

		template<typename TType>
		LayoutBufferObject& operator<<(const std::vector<TType>& data)
		{
			append(std::span<const TType>(data));
			return *this;
		}

		template<typename TType>
		LayoutBufferObject& operator<<(const std::span<TType>& data)
		{
			append(data);
			return *this;
		}

		template<typename TType>
		void append(const TType& data)
		{
			append(std::span<const TType>(&data, 1));
		}

		template<typename TType>
		void append(const std::vector<TType>& data)
		{
			append(std::span<const TType>(data));
		}

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
