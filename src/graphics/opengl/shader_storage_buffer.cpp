#include "graphics/opengl/shader_storage_buffer.hpp"

#include <limits>
#include <stdexcept>

#include "core/context/render_context.hpp"

namespace spk
{
	void ShaderStorageBuffer::State::_validateBuffer(const Buffer &buffer) const
	{
		if (_dynamicElementSize == 0 || buffer.size() < _fixedPartSize ||
			(buffer.size() - _fixedPartSize) % _dynamicElementSize != 0)
		{
			throw std::invalid_argument("ShaderStorageBuffer data size is invalid");
		}
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
	}

	ShaderStorageBuffer::ShaderStorageBuffer() :
		BufferGPUResource(std::make_shared<State>())
	{
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
		return state<State>().dynamicElementCount();
	}
}
