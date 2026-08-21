#include "vertex_array.hpp"

#include <limits>
#include <stdexcept>
#include <vector>

#include "index_buffer.hpp"
#include "vertex_buffer.hpp"

namespace spk
{
	class VertexArray::Instance final : public GPUResource::Instance
	{
	public:
		GLuint identifier = 0;
		GPUResource::Identifier vertexBufferIdentifier = 0;
		GPUResource::Identifier indexBufferIdentifier = 0;
		GPUResource::Generation vertexConfigurationGeneration = 0;
		std::vector<GLuint> enabledAttributes;

		Instance()
		{
			glGenVertexArrays(1, &identifier);
			if (identifier == 0)
			{
				throw std::runtime_error("Failed to create OpenGL vertex array");
			}
		}

		~Instance() override
		{
			if (identifier != 0)
			{
				glDeleteVertexArrays(1, &identifier);
			}
		}
	};

	bool VertexArray::_needsConfiguration(const Instance &instance) const noexcept
	{
		return _vertexBuffer == nullptr ||
			   _indexBuffer == nullptr ||
			   instance.vertexBufferIdentifier != _vertexBuffer->identifier() ||
			   instance.indexBufferIdentifier != _indexBuffer->identifier() ||
			   instance.vertexConfigurationGeneration != _vertexBuffer->configurationGeneration();
	}

	void VertexArray::_disableAttributes(Instance &instance) const
	{
		for (const auto location : instance.enabledAttributes)
		{
			glDisableVertexAttribArray(location);
		}
		instance.enabledAttributes.clear();
	}

	void VertexArray::_configureAttributes(Instance &instance) const
	{
		const auto stride = static_cast<GLsizei>(_vertexBuffer->stride());

		for (const auto &element : _vertexBuffer->attributes())
		{
			const auto &attribute = element.attribute;
			const GLenum type = VertexBuffer::openGLType(attribute.type);
			const auto pointer = reinterpret_cast<const void *>(element.offset);

			glEnableVertexAttribArray(attribute.location);

			switch (attribute.interpretation)
			{
			case VertexBuffer::Interpretation::Floating:
				glVertexAttribPointer(
					attribute.location,
					static_cast<GLint>(attribute.componentCount),
					type,
					attribute.normalized ? GL_TRUE : GL_FALSE,
					stride,
					pointer);
				break;

			case VertexBuffer::Interpretation::Integer:
				glVertexAttribIPointer(
					attribute.location,
					static_cast<GLint>(attribute.componentCount),
					type,
					stride,
					pointer);
				break;

			case VertexBuffer::Interpretation::Double:
				glVertexAttribLPointer(
					attribute.location,
					static_cast<GLint>(attribute.componentCount),
					type,
					stride,
					pointer);
				break;
			}

			instance.enabledAttributes.push_back(attribute.location);
		}
	}

	void VertexArray::_configure(Instance &instance, RenderContext &context) const
	{
		if (_vertexBuffer == nullptr || _indexBuffer == nullptr)
		{
			throw std::logic_error("VertexArray requires both buffers");
		}
		if (_vertexBuffer->stride() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
		{
			throw std::overflow_error("VertexBuffer stride exceeds OpenGL GLsizei range");
		}

		glBindVertexArray(instance.identifier);
		_disableAttributes(instance);
		_vertexBuffer->activate(context);
		_indexBuffer->activate(context);
		_configureAttributes(instance);

		instance.vertexBufferIdentifier = _vertexBuffer->identifier();
		instance.indexBufferIdentifier = _indexBuffer->identifier();
		instance.vertexConfigurationGeneration = _vertexBuffer->configurationGeneration();
	}

	std::unique_ptr<GPUResource::Instance> VertexArray::_create(RenderContext &) const
	{
		return std::make_unique<Instance>();
	}

	GPUResource::Kind VertexArray::_kind() const noexcept
	{
		return GPUResource::Kind::VertexArray;
	}

	void VertexArray::_synchronize(GPUResource::Instance &base, RenderContext &context) const
	{
		_configure(static_cast<Instance &>(base), context);
	}

	void VertexArray::_bind(GPUResource::Instance &base, RenderContext &context) const
	{
		auto &instance = static_cast<Instance &>(base);
		glBindVertexArray(instance.identifier);
		if (_needsConfiguration(instance))
		{
			_configure(instance, context);
		}
	}

	void VertexArray::setVertexBuffer(const VertexBuffer &vertexBuffer)
	{
		if (_vertexBuffer == &vertexBuffer)
		{
			return;
		}

		_vertexBuffer = &vertexBuffer;
	}

	void VertexArray::setIndexBuffer(const IndexBuffer &indexBuffer)
	{
		if (_indexBuffer == &indexBuffer)
		{
			return;
		}

		_indexBuffer = &indexBuffer;
	}
}