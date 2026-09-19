#include "graphics/opengl/uniform_buffer.hpp"

#include "core/context/render_context.hpp"

namespace spk
{
	GLenum UniformBuffer::State::_target() const noexcept
	{
		return GL_UNIFORM_BUFFER;
	}

	void UniformBuffer::State::_bind(GPUResource::Instance &instance, RenderContext &) const
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLuint>(_bindingPoint), BufferGPUResource::_identifier(instance));
	}

	UniformBuffer::UniformBuffer() :
		BufferGPUResource(std::make_shared<State>(0))
	{
	}

	UniformBuffer::UniformBuffer(std::size_t bindingPoint, std::size_t size) :
		BufferGPUResource(std::make_shared<State>(bindingPoint))
	{
		_resize(size);
	}

	std::size_t UniformBuffer::bindingPoint() const noexcept
	{
		return state<State>()._bindingPoint;
	}

	void UniformBuffer::setData(const void *data, std::size_t size)
	{
		if (size != BufferGPUResource::size())
		{
			throw std::invalid_argument("Data size invalid");
		}

		_write(data, size);
	}

	void UniformBuffer::setData(const std::vector<std::byte> &data)
	{
		setData(data.data(), data.size());
	}
}
