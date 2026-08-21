#include "uniform_buffer.hpp"

#include "render_context.hpp"

namespace spk
{
	GLenum UniformBuffer::_target() const noexcept
	{
		return GL_UNIFORM_BUFFER;
	}

	void UniformBuffer::_bind(GPUResource::Instance &instance, RenderContext &) const
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLuint>(_bindingPoint), _identifier(instance));
	}

	UniformBuffer::UniformBuffer(std::size_t bindingPoint, std::size_t size)
	{
		_bindingPoint = bindingPoint;
		_resize(size);
	}

	std::size_t UniformBuffer::bindingPoint() const noexcept
	{
		return _bindingPoint;
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