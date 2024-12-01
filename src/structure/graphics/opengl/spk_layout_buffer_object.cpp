#include "structure/graphics/opengl/spk_layout_buffer_object.hpp"

namespace spk::OpenGL
{
	LayoutBufferObject::Attribute::Attribute() : index(0), size(0), type(Type::Float) {}

	LayoutBufferObject::Attribute::Attribute(Index p_index, size_t p_size, Type p_type)
		: index(p_index), size(p_size), type(p_type) {}

	size_t LayoutBufferObject::Attribute::typeSize(Type p_type)
	{
		switch (p_type)
		{
		case Attribute::Type::Byte:
			return sizeof(GLbyte);
		case Attribute::Type::UByte:
			return sizeof(GLubyte);
		case Attribute::Type::Int:
			return sizeof(GLint);
		case Attribute::Type::UInt:
			return sizeof(GLuint);
		case Attribute::Type::Float:
			return sizeof(GLfloat);
		default:
			return 0;
		}
	}

	void LayoutBufferObject::Factory::insert(Attribute::Index p_index, size_t p_size, Attribute::Type p_type)
	{
		_attributes.emplace_back(p_index, p_size, p_type);
	}

	LayoutBufferObject LayoutBufferObject::Factory::construct() const
	{
		LayoutBufferObject result;

		for (const auto& attribute : _attributes)
		{
			result._insertAttribute(attribute);
		}

		return result;
	}

	LayoutBufferObject::LayoutBufferObject() :
		VertexBufferObject(Type::Storage, Usage::Static),
		_vertexSize(0)
	{
	
	}

	LayoutBufferObject::LayoutBufferObject(const LayoutBufferObject& p_other) :
		VertexBufferObject(p_other),
		_attributesToApply(p_other._attributesToApply),
		_vertexSize(p_other._vertexSize)
	{

	}

	LayoutBufferObject& LayoutBufferObject::operator=(const LayoutBufferObject& p_other)
	{
		if (this != &p_other)
		{
			VertexBufferObject::operator=(p_other);
			_attributesToApply = p_other._attributesToApply;
			_vertexSize = p_other._vertexSize;
		}
		return *this;
	}
	
	LayoutBufferObject::LayoutBufferObject(LayoutBufferObject&& p_other) noexcept
		: VertexBufferObject(std::move(p_other)), _attributesToApply(std::move(p_other._attributesToApply)), _vertexSize(p_other._vertexSize)
	{
		p_other._vertexSize = 0;
	}

	LayoutBufferObject& LayoutBufferObject::operator=(LayoutBufferObject&& p_other) noexcept
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

	void LayoutBufferObject::_insertAttribute(const Attribute& p_attribute)
	{
		_attributesToApply.push_back(p_attribute);
		_vertexSize += p_attribute.size * Attribute::typeSize(p_attribute.type);
	}

	void LayoutBufferObject::_applyAttributes()
	{
		size_t offset = 0;
		for (const auto& attr : _attributesToApply)
		{
			glEnableVertexAttribArray(attr.index);

			switch (attr.type)
			{
			case Attribute::Type::Byte:
			case Attribute::Type::UByte:
			case Attribute::Type::Int:
			case Attribute::Type::UInt:
				glVertexAttribIPointer(attr.index, static_cast<GLint>(attr.size), static_cast<GLenum>(attr.type), static_cast<GLsizei>(_vertexSize), reinterpret_cast<void*>(offset));
				break;
			case Attribute::Type::Float:
				glVertexAttribPointer(attr.index, static_cast<GLint>(attr.size), static_cast<GLenum>(attr.type), GL_FALSE, static_cast<GLsizei>(_vertexSize), reinterpret_cast<void*>(offset));
				break;
			default:
				throw std::runtime_error("Unexpected layout type.");
			}

			offset += attr.size * Attribute::typeSize(attr.type);
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
}
