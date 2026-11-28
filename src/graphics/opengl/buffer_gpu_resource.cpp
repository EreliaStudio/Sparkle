#include "graphics/opengl/buffer_gpu_resource.hpp"

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
		const Buffer *uploadedBuffer = nullptr;
		std::uint64_t uploadedVersion = 0;
		std::size_t uploadedSize = 0;

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

	std::size_t BufferGPUResource::State::Storage::_unitCount(std::size_t size) noexcept
	{
		return (size + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t);
	}

	void BufferGPUResource::State::Storage::resize(std::size_t size)
	{
		const std::size_t previousSize = _size;
		_storage.resize(_unitCount(size));
		_size = size;
		if (size > previousSize)
		{
			std::memset(data() + previousSize, 0, size - previousSize);
		}
	}

	void BufferGPUResource::State::Storage::reserve(std::size_t size)
	{
		_storage.reserve(_unitCount(size));
	}

	void BufferGPUResource::State::Storage::append(const void *source, std::size_t size)
	{
		if (size == 0)
		{
			return;
		}
		const std::size_t offset = _size;
		_storage.resize(_unitCount(_size + size));
		std::memcpy(data() + offset, source, size);
		_size += size;
	}

	void BufferGPUResource::State::Storage::clear()
	{
		_storage.clear();
		_size = 0;
	}

	std::byte *BufferGPUResource::State::Storage::data() noexcept
	{
		return reinterpret_cast<std::byte *>(_storage.data());
	}

	const std::byte *BufferGPUResource::State::Storage::data() const noexcept
	{
		return reinterpret_cast<const std::byte *>(_storage.data());
	}

	std::size_t BufferGPUResource::State::Storage::size() const noexcept
	{
		return _size;
	}

	GPUResource::Kind BufferGPUResource::State::_kind() const noexcept
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

	void BufferGPUResource::State::_allocate(GPUResource::Instance &base) const
	{
		auto &instance = static_cast<BufferGPUResource::Instance &>(base);
		instance.allocatedSize = std::max(instance.allocatedSize, _nextCapacity(size()));
		glBufferData(_target(), static_cast<GLsizeiptr>(instance.allocatedSize), nullptr, _openGLUsage(_usage));
		instance.allocationUsage = _usage;
	}

	void BufferGPUResource::_append(const void *data, std::size_t size)
	{
		auto &content = state<State>();
		if (content._buffer != nullptr)
		{
			throw std::logic_error("Edit the bound CPU Buffer instead of the GPU resource");
		}
		if (size > std::numeric_limits<std::size_t>::max() - content._storage.size())
		{
			throw std::overflow_error("GPU buffer size overflow");
		}
		content._storage.append(data, size);
	}

	void BufferGPUResource::_write(const void *data, std::size_t size, std::size_t offset)
	{
		auto &content = state<State>();
		if (content._buffer != nullptr)
		{
			throw std::logic_error("Edit the bound CPU Buffer instead of the GPU resource");
		}
		if (offset > content._storage.size() || size > content._storage.size() - offset)
		{
			throw std::out_of_range("GPU buffer write exceeds buffer size");
		}

		if (size == 0)
		{
			return;
		}

		std::memcpy(content._storage.data() + offset, data, size);
	}

	void BufferGPUResource::_resize(std::size_t size)
	{
		auto &content = state<State>();
		if (content._buffer != nullptr)
		{
			throw std::logic_error("Edit the bound CPU Buffer instead of the GPU resource");
		}
		if (content._storage.size() == size)
		{
			return;
		}
		content._storage.resize(size);
	}

	void BufferGPUResource::_reserve(std::size_t size)
	{
		auto &content = state<State>();
		if (content._buffer != nullptr)
		{
			throw std::logic_error("Edit the bound CPU Buffer instead of the GPU resource");
		}
		content._storage.reserve(size);
	}

	std::byte *BufferGPUResource::_data()
	{
		auto &content = state<State>();
		if (content._buffer != nullptr)
		{
			throw std::logic_error("Edit the bound CPU Buffer instead of the GPU resource");
		}
		return content._storage.data();
	}

	const std::byte *BufferGPUResource::_data() const
	{
		const auto &content = state<State>();
		return content._buffer != nullptr ? content._buffer->data() : content._storage.data();
	}

	std::unique_ptr<GPUResource::Instance> BufferGPUResource::State::_create(RenderContext &) const
	{
		return std::make_unique<Instance>();
	}

	bool BufferGPUResource::State::_needsSynchronization(const GPUResource::Instance &base) const
	{
		const auto &instance = static_cast<const BufferGPUResource::Instance &>(base);
		return _buffer != nullptr && (instance.uploadedBuffer != _buffer ||
			instance.uploadedVersion != _buffer->version() || instance.uploadedSize != _buffer->size());
	}

	void BufferGPUResource::State::_synchronize(GPUResource::Instance &base, RenderContext &) const
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
			glBufferSubData(_target(), 0, static_cast<GLsizeiptr>(size()), _buffer != nullptr ? _buffer->data() : _storage.data());
		}
		instance.uploadedBuffer = _buffer;
		instance.uploadedVersion = dataVersion();
		instance.uploadedSize = size();
	}

	void BufferGPUResource::State::_bind(GPUResource::Instance &base, RenderContext &) const
	{
		glBindBuffer(_target(), static_cast<Instance &>(base).identifier);
	}

	void BufferGPUResource::bind(const Buffer *buffer)
	{
		auto &content = state<State>();
		content._storage = {};
		content._buffer = buffer;
		validate();
	}

	const Buffer *BufferGPUResource::buffer() const noexcept
	{
		return state<State>()._buffer;
	}

	void BufferGPUResource::clear()
	{
		if (buffer() != nullptr)
		{
			throw std::logic_error("Edit the bound CPU Buffer instead of the GPU resource");
		}
		if (size() == 0)
		{
			return;
		}
		state<State>()._storage.clear();
	}

	void BufferGPUResource::setUsage(Usage usage)
	{
		auto &content = state<State>();
		if (content._usage == usage)
		{
			return;
		}
		content._usage = usage;
	}

	BufferGPUResource::Usage BufferGPUResource::usage() const noexcept
	{
		return state<State>()._usage;
	}

	std::size_t BufferGPUResource::size() const noexcept
	{
		return state<State>().size();
	}

	std::vector<std::byte> BufferGPUResource::retrieve(RenderContext &context) const
	{
		activate(context);

		std::vector<std::byte> result(size());

		if (result.empty())
		{
			return result;
		}

		glGetBufferSubData(
			state<State>()._target(),
			0,
			static_cast<GLsizeiptr>(result.size()),
			result.data());

		return result;
	}
}
