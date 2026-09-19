#include "graphics/opengl/shader_storage_buffer.hpp"

#include <limits>
#include <stdexcept>

#include "core/context/render_context.hpp"

namespace spk
{
	std::size_t ShaderStorageBuffer::_checkedSize(std::size_t nbElement) const
	{
		const auto &content = state<State>();
		if (content._dynamicElementSize == 0)
		{
			throw std::logic_error("ShaderStorageBuffer has no configured dynamic element size");
		}
		if (nbElement > (std::numeric_limits<std::size_t>::max() - content._fixedPartSize) / content._dynamicElementSize)
		{
			throw std::overflow_error("ShaderStorageBuffer size overflow");
		}

		return content._fixedPartSize + nbElement * content._dynamicElementSize;
	}

	GLenum ShaderStorageBuffer::State::_target() const noexcept
	{
		return GL_SHADER_STORAGE_BUFFER;
	}

	void ShaderStorageBuffer::State::_bind(GPUResource::Instance &instance, RenderContext &) const
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(_bindingPoint), BufferGPUResource::_identifier(instance));
	}

	ShaderStorageBuffer::ShaderStorageBuffer(std::size_t bindingPoint, std::size_t fixedPartSize, std::size_t dynamicElementSize) :
		BufferGPUResource(std::make_shared<State>(bindingPoint, fixedPartSize, dynamicElementSize))
	{
		if (dynamicElementSize == 0)
		{
			throw std::invalid_argument("ShaderStorageBuffer dynamic element size cannot be zero");
		}

		_resize(fixedPartSize);
	}

	ShaderStorageBuffer::ShaderStorageBuffer() :
		BufferGPUResource(std::make_shared<State>())
	{
	}

	void ShaderStorageBuffer::resize(std::size_t nbElement)
	{
		auto &content = state<State>();
		if (content._dynamicElementCount == nbElement)
		{
			return;
		}

		_resize(_checkedSize(nbElement));
		content._dynamicElementCount = nbElement;
	}

	void ShaderStorageBuffer::setFixedData(const void *data, std::size_t size)
	{
		if (size != state<State>()._fixedPartSize)
		{
			throw std::invalid_argument("ShaderStorageBuffer fixed data size is invalid");
		}

		_write(data, size);
	}

	void ShaderStorageBuffer::setDynamicData(const void *data, std::size_t nbElement)
	{
		const auto &content = state<State>();
		if (nbElement != content._dynamicElementCount)
		{
			throw std::invalid_argument("ShaderStorageBuffer dynamic element count is invalid");
		}

		_write(data, nbElement * content._dynamicElementSize, content._fixedPartSize);
	}

	std::size_t ShaderStorageBuffer::bindingPoint() const noexcept
	{
		return state<State>()._bindingPoint;
	}

	std::size_t ShaderStorageBuffer::fixedPartSize() const noexcept
	{
		return state<State>()._fixedPartSize;
	}

	std::size_t ShaderStorageBuffer::dynamicElementSize() const noexcept
	{
		return state<State>()._dynamicElementSize;
	}

	std::size_t ShaderStorageBuffer::dynamicElementCount() const noexcept
	{
		return state<State>()._dynamicElementCount;
	}
}
