#include "structure/graphics/opengl/spk_layout_buffer_object.hpp"

#include "spk_debug_macro.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk::OpenGL
{
	LayoutBufferObject::Attribute::Attribute() :
		index(0),
		type(Type::None)
	{
	}

	LayoutBufferObject::Attribute::Attribute(Index p_index, Type p_type) :
		index(p_index),
		type(p_type)
	{
	}

	size_t LayoutBufferObject::Attribute::typeSize(Type p_type)
	{
		switch (p_type)
		{
		case Type::Int:
			return sizeof(GLint);
		case Type::UInt:
			return sizeof(GLuint);
		case Type::Float:
			return sizeof(GLfloat);
		case Type::Bool:
			return sizeof(GLboolean);
		case Type::Vector2:
			return sizeof(GLfloat) * 2;
		case Type::Vector3:
			return sizeof(GLfloat) * 3;
		case Type::Vector4:
			return sizeof(GLfloat) * 4;
		case Type::Vector2Int:
			return sizeof(GLint) * 2;
		case Type::Vector3Int:
			return sizeof(GLint) * 3;
		case Type::Vector4Int:
			return sizeof(GLint) * 4;
		case Type::Vector2UInt:
			return sizeof(GLuint) * 2;
		case Type::Vector3UInt:
			return sizeof(GLuint) * 3;
		case Type::Vector4UInt:
			return sizeof(GLuint) * 4;
		case Type::Matrix2x2:
			return sizeof(GLfloat) * 4;
		case Type::Matrix3x3:
			return sizeof(GLfloat) * 9;
		case Type::Matrix4x4:
			return sizeof(GLfloat) * 16;
		default:
			return 0;
		}
	}

	bool LayoutBufferObject::Attribute::operator<(const LayoutBufferObject::Attribute &p_other) const
	{
		if (index < p_other.index)
		{
			return true;
		}
		else if (index > p_other.index)
		{
			return false;
		}
		return static_cast<int>(type) < static_cast<int>(p_other.type);
	}

	LayoutBufferObject::LayoutBufferObject() :
		VertexBufferObject(Type::Storage, Usage::Static),
		_vertexSize(0)
	{
	}

	LayoutBufferObject::LayoutBufferObject(std::span<const LayoutBufferObject::Attribute> p_attributes) :
		LayoutBufferObject()
	{
		for (const auto &attribute : p_attributes)
		{
			addAttribute(attribute);
		}
	}

	LayoutBufferObject::LayoutBufferObject(std::initializer_list<LayoutBufferObject::Attribute> p_attributes) :
		LayoutBufferObject(std::span(p_attributes.begin(), p_attributes.end()))
	{
	}

	LayoutBufferObject::LayoutBufferObject(const LayoutBufferObject &p_other) :
		VertexBufferObject(p_other),
		_attributesToApply(p_other._attributesToApply),
		_vertexSize(p_other._vertexSize)
	{
	}

	LayoutBufferObject &LayoutBufferObject::operator=(const LayoutBufferObject &p_other)
	{
		if (this != &p_other)
		{
			VertexBufferObject::operator=(p_other);
			_attributesToApply = p_other._attributesToApply;
			_vertexSize = p_other._vertexSize;
		}
		return *this;
	}

	LayoutBufferObject::LayoutBufferObject(LayoutBufferObject &&p_other) noexcept :
		VertexBufferObject(std::move(p_other)),
		_attributesToApply(std::move(p_other._attributesToApply)),
		_vertexSize(p_other._vertexSize)
	{
		p_other._vertexSize = 0;
	}

	LayoutBufferObject &LayoutBufferObject::operator=(LayoutBufferObject &&p_other) noexcept
	{
		if (this != &p_other)
		{
			VertexBufferObject::operator=(std::move(p_other));

			_attributesToApply = std::move(p_other._attributesToApply);
			_vertexSize = p_other._vertexSize;

			p_other._vertexSize = 0;
		}
		return *this;
	}

	bool LayoutBufferObject::hasAttribute(Attribute::Index p_index) const
	{
		for (const auto &attr : _attributesToApply)
		{
			if (attr.index == p_index)
			{
				return true;
			}
		}
		return false;
	}

	void LayoutBufferObject::addAttribute(const Attribute &p_attribute)
	{
		for (const auto &attr : _attributesToApply)
		{
			if (attr.index == p_attribute.index)
			{
				GENERATE_ERROR("Attribute location " + std::to_string(p_attribute.index) + " has already been defined.");
			}
		}

		_attributesToApply.push_back(p_attribute);
		_vertexSize += Attribute::typeSize(p_attribute.type);
	}

	void LayoutBufferObject::addAttribute(Attribute::Index p_index, Attribute::Type p_type)
	{
		addAttribute({p_index, p_type});
	}

	size_t LayoutBufferObject::nbVertex() const
	{
		if (!_attributesToApply.empty())
		{
			_applyAttributes();
		}

		if (_vertexSize == 0)
		{
			GENERATE_ERROR("Vertex size is zero, no attributes defined.");
		}

		return size() / _vertexSize;
	}

	void LayoutBufferObject::_applyAttributes() const
	{
		size_t offset = 0;
		for (const auto &attr : _attributesToApply)
		{
			glEnableVertexAttribArray(attr.index);

			switch (attr.type)
			{
			case Attribute::Type::Int:
				glVertexAttribIPointer(attr.index, 1, GL_INT, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::UInt:
				glVertexAttribIPointer(attr.index, 1, GL_UNSIGNED_INT, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::Float:
				glVertexAttribPointer(attr.index, 1, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::Bool:
				glVertexAttribPointer(attr.index, 1, GL_BOOL, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::Vector2:
				glVertexAttribPointer(attr.index, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::Vector3:
				glVertexAttribPointer(attr.index, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::Vector4:
				glVertexAttribPointer(attr.index, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::Vector2Int:
				glVertexAttribIPointer(attr.index, 2, GL_INT, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::Vector3Int:
				glVertexAttribIPointer(attr.index, 3, GL_INT, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::Vector4Int:
				glVertexAttribIPointer(attr.index, 4, GL_INT, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::Vector2UInt:
				glVertexAttribIPointer(attr.index, 2, GL_UNSIGNED_INT, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::Vector3UInt:
				glVertexAttribIPointer(attr.index, 3, GL_UNSIGNED_INT, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::Vector4UInt:
				glVertexAttribIPointer(attr.index, 4, GL_UNSIGNED_INT, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				break;
			case Attribute::Type::Matrix2x2:
				glVertexAttribPointer(attr.index, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				glEnableVertexAttribArray(attr.index + 1);
				glVertexAttribPointer(
					attr.index + 1, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset + sizeof(GLfloat) * 2));
				break;
			case Attribute::Type::Matrix3x3:
				glVertexAttribPointer(attr.index, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				glEnableVertexAttribArray(attr.index + 1);
				glVertexAttribPointer(
					attr.index + 1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset + sizeof(GLfloat) * 3));
				glEnableVertexAttribArray(attr.index + 2);
				glVertexAttribPointer(
					attr.index + 2, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset + sizeof(GLfloat) * 6));
				break;
			case Attribute::Type::Matrix4x4:
				glVertexAttribPointer(attr.index, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset));
				glEnableVertexAttribArray(attr.index + 1);
				glVertexAttribPointer(
					attr.index + 1, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset + sizeof(GLfloat) * 4));
				glEnableVertexAttribArray(attr.index + 2);
				glVertexAttribPointer(
					attr.index + 2, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void *>(offset + sizeof(GLfloat) * 8));
				glEnableVertexAttribArray(attr.index + 3);
				glVertexAttribPointer(
					attr.index + 3,
					4,
					GL_FLOAT,
					GL_FALSE,
					static_cast<GLsizei>(_vertexSize),
					reinterpret_cast<void *>(offset + sizeof(GLfloat) * 12));
				break;
			default:
				GENERATE_ERROR("Unexpected layout type.");
			}

			// Update offset based on the type
			switch (attr.type)
			{
			case Attribute::Type::Int:
			case Attribute::Type::UInt:
			case Attribute::Type::Float:
			case Attribute::Type::Bool:
				offset += sizeof(GLfloat);
				break;
			case Attribute::Type::Vector2:
			case Attribute::Type::Vector2Int:
			case Attribute::Type::Vector2UInt:
				offset += sizeof(GLfloat) * 2;
				break;
			case Attribute::Type::Vector3:
			case Attribute::Type::Vector3Int:
			case Attribute::Type::Vector3UInt:
				offset += sizeof(GLfloat) * 3;
				break;
			case Attribute::Type::Vector4:
			case Attribute::Type::Vector4Int:
			case Attribute::Type::Vector4UInt:
				offset += sizeof(GLfloat) * 4;
				break;
			case Attribute::Type::Matrix2x2:
				offset += sizeof(GLfloat) * 4;
				break;
			case Attribute::Type::Matrix3x3:
				offset += sizeof(GLfloat) * 9;
				break;
			case Attribute::Type::Matrix4x4:
				offset += sizeof(GLfloat) * 16;
				break;
			default:
				GENERATE_ERROR("Unexpected layout type.");
			}
		}
		_attributesToApply.clear();
	}

	void LayoutBufferObject::activate()
	{
		VertexBufferObject::activate();
		if (!_attributesToApply.empty())
		{
			_applyAttributes();
		}
	}

	std::string toString(const LayoutBufferObject::Attribute::Type &p_type)
	{
		switch (p_type)
		{
			using Type = LayoutBufferObject::Attribute::Type;

		case Type::Float:
			return ("Float");
		case Type::Bool:
			return ("Bool");
		case Type::Int:
			return ("Int");
		case Type::UInt:
			return ("UInt");
		case Type::Vector2:
			return ("Vector2");
		case Type::Vector3:
			return ("Vector3");
		case Type::Vector4:
			return ("Vector4");
		case Type::Vector2Int:
			return ("Vector2Int");
		case Type::Vector3Int:
			return ("Vector3Int");
		case Type::Vector4Int:
			return ("Vector4Int");
		case Type::Vector2UInt:
			return ("Vector2UInt");
		case Type::Vector3UInt:
			return ("Vector3UInt");
		case Type::Vector4UInt:
			return ("Vector4UInt");
		case Type::Matrix2x2:
			return ("Matrix2x2");
		case Type::Matrix3x3:
			return ("Matrix3x3");
		case Type::Matrix4x4:
			return ("Matrix4x4");
		default:
			return ("None");
		}
	}
	std::wstring toWstring(const LayoutBufferObject::Attribute::Type &p_type)
	{
		switch (p_type)
		{
			using Type = LayoutBufferObject::Attribute::Type;

		case Type::Float:
			return (L"Float");
		case Type::Bool:
			return (L"Bool");
		case Type::Int:
			return (L"Int");
		case Type::UInt:
			return (L"UInt");
		case Type::Vector2:
			return (L"Vector2");
		case Type::Vector3:
			return (L"Vector3");
		case Type::Vector4:
			return (L"Vector4");
		case Type::Vector2Int:
			return (L"Vector2Int");
		case Type::Vector3Int:
			return (L"Vector3Int");
		case Type::Vector4Int:
			return (L"Vector4Int");
		case Type::Vector2UInt:
			return (L"Vector2UInt");
		case Type::Vector3UInt:
			return (L"Vector3UInt");
		case Type::Vector4UInt:
			return (L"Vector4UInt");
		case Type::Matrix2x2:
			return (L"Matrix2x2");
		case Type::Matrix3x3:
			return (L"Matrix3x3");
		case Type::Matrix4x4:
			return (L"Matrix4x4");
		default:
			return (L"None");
		}
	}
}
