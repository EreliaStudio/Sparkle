#include "graphics/opengl/vertex_buffer.hpp"

#include <limits>
#include <stdexcept>

namespace spk
{
	VertexBuffer::VertexBuffer() :
		BufferGPUResource(std::make_shared<State>())
	{
	}

	void VertexBuffer::_validateConfigurationEdition() const
	{
		if (size() != 0)
		{
			throw std::logic_error("Cannot modify VertexBuffer configuration while it contains vertices");
		}
	}

	void VertexBuffer::_touchConfiguration() noexcept
	{
		auto &content = state<State>();
		++content._configurationGeneration;
		if (content._configurationGeneration == 0)
		{
			content._configurationGeneration = 1;
		}
	}

	void VertexBuffer::State::_validateBuffer(const Buffer &buffer) const
	{
		if (!buffer.empty() && (_layout.stride() == 0 || buffer.size() % _layout.stride() != 0))
		{
			throw std::logic_error("VertexBuffer data size does not match its configured stride");
		}
	}

	GLenum VertexBuffer::State::_target() const noexcept
	{
		return GL_ARRAY_BUFFER;
	}

	void VertexBuffer::setLayout(const VertexLayout &layout)
	{
		_validateConfigurationEdition();
		state<State>()._layout = layout;
		_touchConfiguration();
	}

	void VertexBuffer::addAttribute(Attribute attribute)
	{
		_validateConfigurationEdition();
		state<State>()._layout.addAttribute(attribute);
		_touchConfiguration();
	}

	void VertexBuffer::addAttribute(
		std::uint32_t location,
		Attribute::Type type,
		std::uint32_t componentCount,
		Interpretation interpretation,
		bool normalized)
	{
		addAttribute({.location = location, .type = type, .componentCount = componentCount, .interpretation = interpretation, .normalized = normalized});
	}

	void VertexBuffer::addPadding(std::size_t byteCount)
	{
		_validateConfigurationEdition();
		if (byteCount != 0)
		{
			state<State>()._layout.addPadding(byteCount);
			_touchConfiguration();
		}
	}

	void VertexBuffer::clearConfiguration()
	{
		_validateConfigurationEdition();
		auto &layout = state<State>()._layout;
		if (layout.attributes().empty() && layout.stride() == 0)
		{
			return;
		}
		layout.clear();
		_touchConfiguration();
	}

	std::size_t VertexBuffer::stride() const noexcept
	{
		return state<State>()._layout.stride();
	}

	std::size_t VertexBuffer::count() const noexcept
	{
		const auto stride = state<State>()._layout.stride();
		if (stride == 0)
		{
			return 0;
		}

		return size() / stride;
	}

	std::span<const VertexBuffer::ResolvedAttribute> VertexBuffer::attributes() const noexcept
	{
		return state<State>()._layout.attributes();
	}

	GPUResource::Generation VertexBuffer::configurationGeneration() const noexcept
	{
		return state<State>()._configurationGeneration;
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
