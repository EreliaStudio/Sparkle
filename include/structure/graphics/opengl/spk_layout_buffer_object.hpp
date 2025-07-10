#pragma once

#include <GL/glew.h>

#include <GL/gl.h>

#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "spk_vertex_buffer_object.hpp"

#include "structure/system/spk_exception.hpp"

#include <iostream>

template <typename T>
using element_type_t = std::remove_reference_t<decltype(*std::begin(std::declval<T &>()))>;

namespace spk::OpenGL
{
	class LayoutBufferObject : public VertexBufferObject
	{
	public:
		struct Attribute
		{
			using Index = GLuint;

			enum class Type {
				None,
				Float,
				Bool,
				Int,
				UInt,
				Vector2,
				Vector3,
				Vector4,
				Vector2Int,
				Vector3Int,
				Vector4Int,
				Vector2UInt,
				Vector3UInt,
				Vector4UInt,
				Matrix2x2,
				Matrix3x3,
				Matrix4x4
			};

			Index index;
			Type type;

			Attribute();
			Attribute(Index p_index, Type p_type);

			static size_t typeSize(Type p_type);

			bool operator<(const Attribute& other) const;
		};

	private:
		std::vector<Attribute> _attributesToApply;
		size_t _vertexSize;

		void _applyAttributes();

	public:
		LayoutBufferObject();

		explicit LayoutBufferObject(std::span<const LayoutBufferObject::Attribute> attributes);

		LayoutBufferObject(std::initializer_list<LayoutBufferObject::Attribute> attributes);

		template <typename Container>
			requires(std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Container>().data())>>,
									LayoutBufferObject::Attribute> &&
					 requires(const Container &c) {
						 { c.data() } -> std::convertible_to<const LayoutBufferObject::Attribute *>;
						 { c.size() } -> std::convertible_to<std::size_t>;
					 })
		explicit LayoutBufferObject(const Container &attributes) :
			LayoutBufferObject(std::span(attributes.data(), attributes.size()))
		{
		}

		LayoutBufferObject(const LayoutBufferObject &p_other);
		LayoutBufferObject &operator=(const LayoutBufferObject &p_other);
		LayoutBufferObject(LayoutBufferObject &&p_other) noexcept;
		LayoutBufferObject &operator=(LayoutBufferObject &&p_other) noexcept;

		virtual void activate() override;

		template <typename TType>
		void append(std::span<const TType> data)
		{
			if (data.empty())
			{
				return;
			}

			if (sizeof(TType) != _vertexSize)
			{
				GENERATE_ERROR("Size mismatch in LayoutBufferObject: Expected vertex size is " + std::to_string(_vertexSize) +
										 " bytes, but received " + std::to_string(sizeof(TType)) + " bytes.");
			}
			VertexBufferObject::append(data.data(), data.size() * sizeof(TType));
		}

		template <typename TType>
		LayoutBufferObject &operator<<(const TType &element)
		{
			append(std::span<const TType>(&element, 1));
			return *this;
		}

		template <typename Container>
			requires requires(const Container &c) {
				{ std::data(c) };
				{ std::size(c) };
			}
		LayoutBufferObject &operator<<(const Container &container)
		{
			append(std::span<const element_type_t<Container>>(std::data(container), std::size(container)));
			return *this;
		}

		bool hasAttribute(Attribute::Index index) const;

		void addAttribute(const Attribute &p_attribute);
		void addAttribute(Attribute::Index p_index, Attribute::Type p_type);

		template <typename TType>
		std::vector<TType> get()
		{
			activate();

			size_t totalSize = this->size();
			if (totalSize == 0)
			{
				return {};
			}

			if (_vertexSize % sizeof(TType) != 0)
			{
				GENERATE_ERROR("LayoutBufferObject::get() - The buffer element size (" + std::to_string(_vertexSize) +
										 " bytes) is incompatible with TType size (" + std::to_string(sizeof(TType)) + " bytes).");
			}

			size_t elementCount = totalSize / sizeof(TType);
			std::vector<TType> result(elementCount);

			glGetBufferSubData(GL_ARRAY_BUFFER, 0, totalSize, result.data());

			return result;
		}
	};

	std::string to_string(const LayoutBufferObject::Attribute::Type &p_type);
	std::wstring to_wstring(const LayoutBufferObject::Attribute::Type &p_type);
}
