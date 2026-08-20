#include "vertex_buffer.hpp"

#include <limits>
#include <stdexcept>

namespace spk
{
	std::size_t VertexBuffer::_typeSize(Attribute::Type type)
	{
		switch (type)
		{
		case Attribute::Type::Byte:
			return sizeof(GLbyte);

		case Attribute::Type::UnsignedByte:
			return sizeof(GLubyte);

		case Attribute::Type::Short:
			return sizeof(GLshort);

		case Attribute::Type::UnsignedShort:
			return sizeof(GLushort);

		case Attribute::Type::Int:
			return sizeof(GLint);

		case Attribute::Type::UnsignedInt:
			return sizeof(GLuint);

		case Attribute::Type::HalfFloat:
			return sizeof(GLhalf);

		case Attribute::Type::Float:
			return sizeof(GLfloat);

		case Attribute::Type::Double:
			return sizeof(GLdouble);
		}

		throw std::invalid_argument("Unknown VertexBuffer attribute type");
	}

	bool VertexBuffer::_isIntegerType(Attribute::Type type) noexcept
	{
		switch (type)
		{
		case Attribute::Type::Byte:
		case Attribute::Type::UnsignedByte:
		case Attribute::Type::Short:
		case Attribute::Type::UnsignedShort:
		case Attribute::Type::Int:
		case Attribute::Type::UnsignedInt:
			return true;

		case Attribute::Type::HalfFloat:
		case Attribute::Type::Float:
		case Attribute::Type::Double:
			return false;
		}

		return false;
	}

	void VertexBuffer::_validateAttribute(const Attribute &attribute)
	{
		if (attribute.componentCount == 0 || attribute.componentCount > 4)
			throw std::invalid_argument("VertexBuffer attribute component count must be between 1 and 4");

		switch (attribute.interpretation)
		{
		case Interpretation::Floating:
			if (attribute.type == Attribute::Type::Double)
				throw std::invalid_argument("Double attributes require double interpretation");
			if (attribute.normalized && !_isIntegerType(attribute.type))
				throw std::invalid_argument("Only integer attribute types can be normalized");
			break;

		case Interpretation::Integer:
			if (!_isIntegerType(attribute.type))
				throw std::invalid_argument("Integer interpretation requires an integer attribute type");
			if (attribute.normalized)
				throw std::invalid_argument("Integer interpretation cannot use normalization");
			break;

		case Interpretation::Double:
			if (attribute.type != Attribute::Type::Double)
				throw std::invalid_argument("Double interpretation requires a double attribute type");
			if (attribute.normalized)
				throw std::invalid_argument("Double interpretation cannot use normalization");
			break;
		}
	}

	void VertexBuffer::_validateConfigurationEdition() const
	{
		if (size() != 0)
			throw std::logic_error("Cannot modify VertexBuffer configuration while it contains vertices");
	}

	void VertexBuffer::_touchConfiguration() noexcept
	{
		++_configurationGeneration;
		if (_configurationGeneration == 0)
			_configurationGeneration = 1;
	}

	std::size_t VertexBuffer::_checkedSize(std::size_t count, std::size_t stride)
	{
		if (stride != 0 && count > std::numeric_limits<std::size_t>::max() / stride)
			throw std::overflow_error("VertexBuffer size overflow");

		return count * stride;
	}

	GLenum VertexBuffer::_target() const noexcept
	{
		return GL_ARRAY_BUFFER;
	}

	void VertexBuffer::addAttribute(Attribute attribute)
	{
		_validateConfigurationEdition();
		_validateAttribute(attribute);

		for (const auto &element : _attributes)
		{
			if (element.attribute.location == attribute.location)
				throw std::invalid_argument("VertexBuffer attribute location is already configured");
		}

		const std::size_t typeSize = _typeSize(attribute.type);
		if (attribute.componentCount > std::numeric_limits<std::size_t>::max() / typeSize)
			throw std::overflow_error("VertexBuffer attribute size overflow");

		const std::size_t attributeSize = typeSize * attribute.componentCount;
		if (_stride > std::numeric_limits<std::size_t>::max() - attributeSize)
			throw std::overflow_error("VertexBuffer stride overflow");

		_attributes.push_back({
			.attribute = attribute,
			.offset = _stride
		});

		_stride += attributeSize;
		_touchConfiguration();
	}

	void VertexBuffer::addAttribute(
		std::uint32_t location,
		Attribute::Type type,
		std::uint32_t componentCount,
		Interpretation interpretation,
		bool normalized)
	{
		addAttribute({
			.location = location,
			.type = type,
			.componentCount = componentCount,
			.interpretation = interpretation,
			.normalized = normalized
		});
	}

	void VertexBuffer::addPadding(std::size_t byteCount)
	{
		_validateConfigurationEdition();

		if (_stride > std::numeric_limits<std::size_t>::max() - byteCount)
			throw std::overflow_error("VertexBuffer stride overflow");

		if (byteCount == 0)
			return;

		_stride += byteCount;
		_touchConfiguration();
	}

	void VertexBuffer::clearConfiguration()
	{
		_validateConfigurationEdition();

		if (_attributes.empty() && _stride == 0)
			return;

		_attributes.clear();
		_stride = 0;
		_touchConfiguration();
	}

	std::size_t VertexBuffer::stride() const noexcept
	{
		return _stride;
	}

	std::size_t VertexBuffer::count() const noexcept
	{
		if (_stride == 0)
			return 0;

		return size() / _stride;
	}

	std::span<const VertexBuffer::ResolvedAttribute> VertexBuffer::attributes() const noexcept
	{
		return _attributes;
	}

	GPUResource::Generation VertexBuffer::configurationGeneration() const noexcept
	{
		return _configurationGeneration;
	}

	GLenum VertexBuffer::openGLType(Attribute::Type type)
	{
		switch (type)
		{
		case Attribute::Type::Byte:
			return GL_BYTE;

		case Attribute::Type::UnsignedByte:
			return GL_UNSIGNED_BYTE;

		case Attribute::Type::Short:
			return GL_SHORT;

		case Attribute::Type::UnsignedShort:
			return GL_UNSIGNED_SHORT;

		case Attribute::Type::Int:
			return GL_INT;

		case Attribute::Type::UnsignedInt:
			return GL_UNSIGNED_INT;

		case Attribute::Type::HalfFloat:
			return GL_HALF_FLOAT;

		case Attribute::Type::Float:
			return GL_FLOAT;

		case Attribute::Type::Double:
			return GL_DOUBLE;
		}

		throw std::invalid_argument("Unknown VertexBuffer attribute type");
	}
}