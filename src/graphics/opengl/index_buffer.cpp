#include "graphics/opengl/index_buffer.hpp"

#include <limits>
#include <stdexcept>

namespace spk
{
	std::size_t IndexBuffer::_typeSize(Type type) noexcept
	{
		switch (type)
		{
		case Type::UnsignedByte:
			return sizeof(std::uint8_t);
		case Type::UnsignedShort:
			return sizeof(std::uint16_t);
		case Type::UnsignedInt:
			return sizeof(std::uint32_t);
		}
		return 0;
	}

	std::size_t IndexBuffer::_checkedSize(std::size_t count, std::size_t stride)
	{
		if (stride != 0 && count > std::numeric_limits<std::size_t>::max() / stride)
		{
			throw std::overflow_error("IndexBuffer size overflow");
		}
		return count * stride;
	}

	GLenum IndexBuffer::_target() const noexcept
	{
		return GL_ELEMENT_ARRAY_BUFFER;
	}

	void IndexBuffer::setType(Type type)
	{
		if (_type == type)
		{
			return;
		}
		if (size() != 0)
		{
			throw std::logic_error("IndexBuffer type cannot change while it contains data");
		}
		_type = type;
		_stride = _typeSize(type);
	}

	void IndexBuffer::clearConfiguration()
	{
		if (size() != 0)
		{
			throw std::logic_error("IndexBuffer configuration cannot change while it contains data");
		}
		_type.reset();
		_stride = 0;
	}

	bool IndexBuffer::isConfigured() const noexcept
	{
		return _type.has_value();
	}

	std::optional<IndexBuffer::Type> IndexBuffer::type() const noexcept
	{
		return _type;
	}

	std::size_t IndexBuffer::stride() const noexcept
	{
		return _stride;
	}

	std::size_t IndexBuffer::count() const noexcept
	{
		return _stride == 0 ? 0 : size() / _stride;
	}
}
