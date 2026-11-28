#include "geometry/vertex_layout.hpp"

#include <limits>
#include <stdexcept>

namespace spk
{
	std::size_t VertexLayout::_typeSize(Attribute::Type type)
	{
		switch (type)
		{
		case Attribute::Type::Byte:
			return 1;

		case Attribute::Type::UnsignedByte:
			return 1;

		case Attribute::Type::Short:
			return 2;

		case Attribute::Type::UnsignedShort:
			return 2;

		case Attribute::Type::Int:
			return 4;

		case Attribute::Type::UnsignedInt:
			return 4;

		case Attribute::Type::HalfFloat:
			return 2;

		case Attribute::Type::Float:
			return 4;

		case Attribute::Type::Double:
			return 8;
		}

		throw std::invalid_argument("Unknown VertexLayout attribute type");
	}

	bool VertexLayout::_isIntegerType(Attribute::Type type) noexcept
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

	void VertexLayout::_validateAttribute(const Attribute &attribute)
	{
		if (attribute.componentCount == 0 || attribute.componentCount > 4)
		{
			throw std::invalid_argument("VertexLayout attribute component count must be between 1 and 4");
		}

		switch (attribute.interpretation)
		{
		case Interpretation::Floating:
			if (attribute.type == Attribute::Type::Double)
			{
				throw std::invalid_argument("Double attributes require double interpretation");
			}
			if (attribute.normalized && !_isIntegerType(attribute.type))
			{
				throw std::invalid_argument("Only integer attribute types can be normalized");
			}
			break;

		case Interpretation::Integer:
			if (!_isIntegerType(attribute.type))
			{
				throw std::invalid_argument("Integer interpretation requires an integer attribute type");
			}
			if (attribute.normalized)
			{
				throw std::invalid_argument("Integer interpretation cannot use normalization");
			}
			break;

		case Interpretation::Double:
			if (attribute.type != Attribute::Type::Double)
			{
				throw std::invalid_argument("Double interpretation requires a double attribute type");
			}
			if (attribute.normalized)
			{
				throw std::invalid_argument("Double interpretation cannot use normalization");
			}
			break;
		}
	}

	void VertexLayout::addAttribute(Attribute attribute)
	{
		_validateAttribute(attribute);

		for (const auto &element : _attributes)
		{
			if (element.attribute.location == attribute.location)
			{
				throw std::invalid_argument("VertexLayout attribute location is already configured");
			}
		}

		const std::size_t typeSize = _typeSize(attribute.type);
		if (attribute.componentCount > std::numeric_limits<std::size_t>::max() / typeSize)
		{
			throw std::overflow_error("VertexLayout attribute size overflow");
		}

		const std::size_t attributeSize = typeSize * attribute.componentCount;
		if (_stride > std::numeric_limits<std::size_t>::max() - attributeSize)
		{
			throw std::overflow_error("VertexLayout stride overflow");
		}

		_attributes.push_back({.attribute = attribute, .offset = _stride});

		_stride += attributeSize;
	}

	void VertexLayout::addAttribute(
		std::uint32_t location,
		Attribute::Type type,
		std::uint32_t componentCount,
		Interpretation interpretation,
		bool normalized)
	{
		addAttribute({.location = location, .type = type, .componentCount = componentCount, .interpretation = interpretation, .normalized = normalized});
	}

	void VertexLayout::addPadding(std::size_t byteCount)
	{
		if (_stride > std::numeric_limits<std::size_t>::max() - byteCount)
		{
			throw std::overflow_error("VertexLayout stride overflow");
		}

		if (byteCount == 0)
		{
			return;
		}

		_stride += byteCount;
	}

	void VertexLayout::clear()
	{
		_attributes.clear();
		_stride = 0;
	}

	std::size_t VertexLayout::stride() const noexcept
	{
		return _stride;
	}

	std::span<const VertexLayout::ResolvedAttribute> VertexLayout::attributes() const noexcept
	{
		return _attributes;
	}
}
