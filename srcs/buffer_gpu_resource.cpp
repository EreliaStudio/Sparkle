#include "buffer_gpu_resource.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace spk
{
	class BufferGPUResource::Instance final : public GPUResource::Instance
	{
	public:
		GLuint identifier = 0;
		std::size_t allocatedSize = 0;
		std::optional<Usage> allocationUsage;

		Instance()
		{
			glGenBuffers(1, &identifier);
			if (identifier == 0)
			{
				throw std::runtime_error("Failed to create OpenGL buffer");
			}
		}

		~Instance() override
		{
			if (identifier != 0)
			{
				glDeleteBuffers(1, &identifier);
			}
		}
	};

	std::size_t BufferGPUResource::Storage::_unitCount(std::size_t size) noexcept
	{
		return (size + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t);
	}

	void BufferGPUResource::Storage::resize(std::size_t size)
	{
		const std::size_t previousSize = _size;
		_storage.resize(_unitCount(size));
		_size = size;
		if (size > previousSize)
		{
			std::memset(data() + previousSize, 0, size - previousSize);
		}
	}

	void BufferGPUResource::Storage::reserve(std::size_t size)
	{
		_storage.reserve(_unitCount(size));
	}

	void BufferGPUResource::Storage::append(const void *source, std::size_t size)
	{
		if (size == 0)
		{
			return;
		}
		const std::size_t offset = _size;
		resize(_size + size);
		std::memcpy(data() + offset, source, size);
	}

	void BufferGPUResource::Storage::clear()
	{
		_storage.clear();
		_size = 0;
	}

	std::byte *BufferGPUResource::Storage::data() noexcept
	{
		return reinterpret_cast<std::byte *>(_storage.data());
	}

	const std::byte *BufferGPUResource::Storage::data() const noexcept
	{
		return reinterpret_cast<const std::byte *>(_storage.data());
	}

	std::size_t BufferGPUResource::Storage::size() const noexcept
	{
		return _size;
	}

	GPUResource::Kind BufferGPUResource::_kind() const noexcept
	{
		return GPUResource::Kind::Buffer;
	}

	GLenum BufferGPUResource::_openGLUsage(Usage usage) noexcept
	{
		switch (usage)
		{
		case Usage::StaticDraw:
			return GL_STATIC_DRAW;
		case Usage::DynamicDraw:
			return GL_DYNAMIC_DRAW;
		case Usage::StreamDraw:
			return GL_STREAM_DRAW;
		}
		return GL_STATIC_DRAW;
	}

	std::size_t BufferGPUResource::_nextCapacity(std::size_t required)
	{
		std::size_t capacity = 64;
		while (capacity < required && capacity <= std::numeric_limits<std::size_t>::max() / 2)
		{
			capacity *= 2;
		}
		return std::max(capacity, required);
	}

	GLuint BufferGPUResource::_identifier(GPUResource::Instance &instance) noexcept
	{
		return static_cast<Instance &>(instance).identifier;
	}

	void BufferGPUResource::_allocate(Instance &instance) const
	{
		instance.allocatedSize = std::max(instance.allocatedSize, _nextCapacity(size()));
		glBufferData(_target(), static_cast<GLsizeiptr>(instance.allocatedSize), nullptr, _openGLUsage(_usage));
		instance.allocationUsage = _usage;
	}

	void BufferGPUResource::_append(const void *data, std::size_t size)
	{
		if (size > std::numeric_limits<std::size_t>::max() - _storage.size())
		{
			throw std::overflow_error("GPU buffer size overflow");
		}
		_storage.append(data, size);
	}

	void BufferGPUResource::_write(const void *data, std::size_t size, std::size_t offset)
	{
		if (offset > _storage.size() || size > _storage.size() - offset)
		{
			throw std::out_of_range("GPU buffer write exceeds buffer size");
		}

		if (size == 0)
		{
			return;
		}

		std::memcpy(_storage.data() + offset, data, size);
	}

	void BufferGPUResource::_resize(std::size_t size)
	{
		if (_storage.size() == size)
		{
			return;
		}
		_storage.resize(size);
	}

	void BufferGPUResource::_reserve(std::size_t size)
	{
		_storage.reserve(size);
	}

	std::byte *BufferGPUResource::_data() noexcept
	{
		return _storage.data();
	}

	const std::byte *BufferGPUResource::_data() const noexcept
	{
		return _storage.data();
	}

	std::unique_ptr<GPUResource::Instance> BufferGPUResource::_create(RenderContext &) const
	{
		return std::make_unique<Instance>();
	}

	void BufferGPUResource::_synchronize(GPUResource::Instance &base, RenderContext &) const
	{
		auto &instance = static_cast<Instance &>(base);
		glBindBuffer(_target(), instance.identifier);
		const bool usageChanged = !instance.allocationUsage.has_value() || *instance.allocationUsage != _usage;
		if (size() != 0 && (size() > instance.allocatedSize || usageChanged))
		{
			_allocate(instance);
		}
		if (size() != 0)
		{
			glBufferSubData(_target(), 0, static_cast<GLsizeiptr>(size()), _data());
		}
	}

	void BufferGPUResource::_bind(GPUResource::Instance &base, RenderContext &) const
	{
		glBindBuffer(_target(), static_cast<Instance &>(base).identifier);
	}

	void BufferGPUResource::clear()
	{
		if (size() == 0)
		{
			return;
		}
		_storage.clear();
	}

	void BufferGPUResource::setUsage(Usage usage)
	{
		if (_usage == usage)
		{
			return;
		}
		_usage = usage;
	}

	BufferGPUResource::Usage BufferGPUResource::usage() const noexcept
	{
		return _usage;
	}

	std::size_t BufferGPUResource::size() const noexcept
	{
		return _storage.size();
	}

	std::vector<std::byte> BufferGPUResource::retrieve(RenderContext &context) const
	{
		activate(context);

		std::vector<std::byte> result(_storage.size());

		if (result.empty())
		{
			return result;
		}

		glGetBufferSubData(
			_target(),
			0,
			static_cast<GLsizeiptr>(result.size()),
			result.data());

		return result;
	}
}
