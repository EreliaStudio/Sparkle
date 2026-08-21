#include "shader_storage_buffer.hpp"

#include <limits>
#include <stdexcept>

#include "render_context.hpp"

namespace spk
{
	std::size_t ShaderStorageBuffer::_checkedSize(std::size_t nbElement) const
	{
		if (nbElement > (std::numeric_limits<std::size_t>::max() - _fixedPartSize) / _dynamicElementSize)
		{
			throw std::overflow_error("ShaderStorageBuffer size overflow");
		}

		return _fixedPartSize + nbElement * _dynamicElementSize;
	}

	GLenum ShaderStorageBuffer::_target() const noexcept
	{
		return GL_SHADER_STORAGE_BUFFER;
	}

	void ShaderStorageBuffer::_bind(GPUResource::Instance &instance, RenderContext &) const
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(_bindingPoint), _identifier(instance));
	}

	ShaderStorageBuffer::ShaderStorageBuffer(std::size_t bindingPoint, std::size_t fixedPartSize, std::size_t dynamicElementSize)
	{
		if (dynamicElementSize == 0)
		{
			throw std::invalid_argument("ShaderStorageBuffer dynamic element size cannot be zero");
		}

		_bindingPoint = bindingPoint;
		_fixedPartSize = fixedPartSize;
		_dynamicElementSize = dynamicElementSize;

		_resize(_fixedPartSize);
	}

	void ShaderStorageBuffer::resize(std::size_t nbElement)
	{
		if (_dynamicElementCount == nbElement)
		{
			return;
		}

		_resize(_checkedSize(nbElement));
		_dynamicElementCount = nbElement;
	}

	void ShaderStorageBuffer::setFixedData(const void *data, std::size_t size)
	{
		if (size != _fixedPartSize)
		{
			throw std::invalid_argument("ShaderStorageBuffer fixed data size is invalid");
		}

		_write(data, size);
	}

	void ShaderStorageBuffer::setDynamicData(const void *data, std::size_t nbElement)
	{
		if (nbElement != _dynamicElementCount)
		{
			throw std::invalid_argument("ShaderStorageBuffer dynamic element count is invalid");
		}

		_write(data, nbElement * _dynamicElementSize, _fixedPartSize);
	}

	std::size_t ShaderStorageBuffer::bindingPoint() const noexcept
	{
		return _bindingPoint;
	}

	std::size_t ShaderStorageBuffer::fixedPartSize() const noexcept
	{
		return _fixedPartSize;
	}

	std::size_t ShaderStorageBuffer::dynamicElementSize() const noexcept
	{
		return _dynamicElementSize;
	}

	std::size_t ShaderStorageBuffer::dynamicElementCount() const noexcept
	{
		return _dynamicElementCount;
	}
}