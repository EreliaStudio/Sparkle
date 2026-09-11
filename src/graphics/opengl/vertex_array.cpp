#include "graphics/opengl/vertex_array.hpp"

#include <limits>
#include <stdexcept>
#include <vector>

#include "graphics/opengl/index_buffer.hpp"
#include "graphics/opengl/vertex_buffer.hpp"

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

	bool VertexArray::State::_needsConfiguration(const Instance &instance) const noexcept
	{
		return !_vertexBuffer ||
			   !_indexBuffer ||
			   instance.vertexBufferIdentifier != _vertexBuffer.identifier() ||
			   instance.indexBufferIdentifier != _indexBuffer.identifier() ||
			   instance.vertexConfigurationGeneration != _vertexBuffer->configurationGeneration();
	}

	void VertexArray::State::_disableAttributes(Instance &instance) const
	{
		for (const auto location : instance.enabledAttributes)
		{
			glDisableVertexAttribArray(location);
		}
		instance.enabledAttributes.clear();
	}

	void VertexArray::State::_configureAttributes(Instance &instance) const
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

	void VertexArray::State::_configure(Instance &instance, RenderContext &context) const
	{
		if (!_vertexBuffer || !_indexBuffer)
		{
			throw std::logic_error("VertexArray requires both buffers");
		}
		if (_vertexBuffer->stride() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
		{
			throw std::overflow_error("VertexBuffer stride exceeds OpenGL GLsizei range");
		}

		glBindVertexArray(instance.identifier);
		_disableAttributes(instance);
		_vertexBuffer.activate(context);
		_indexBuffer.activate(context);
		_configureAttributes(instance);

		instance.vertexBufferIdentifier = _vertexBuffer.identifier();
		instance.indexBufferIdentifier = _indexBuffer.identifier();
		instance.vertexConfigurationGeneration = _vertexBuffer->configurationGeneration();
	}

	std::unique_ptr<GPUResource::Instance> VertexArray::State::_create(RenderContext &) const
	{
		return std::make_unique<Instance>();
	}

	GPUResource::Kind VertexArray::State::_kind() const noexcept
	{
		return GPUResource::Kind::VertexArray;
	}

	void VertexArray::State::_synchronize(GPUResource::Instance &base, RenderContext &context) const
	{
		_configure(static_cast<Instance &>(base), context);
	}

	void VertexArray::State::_bind(GPUResource::Instance &base, RenderContext &context) const
	{
		auto &instance = static_cast<Instance &>(base);
		glBindVertexArray(instance.identifier);
		if (_needsConfiguration(instance))
		{
			_configure(instance, context);
		}
	}

	VertexArray::VertexArray() :
		GPUResource(std::make_shared<State>())
	{
	}

	void VertexArray::setVertexBuffer(const VertexBuffer &vertexBuffer)
	{
		auto &content = state<State>();
		if (content._vertexBuffer.identifier() == vertexBuffer.identifier())
		{
			return;
		}

		content._vertexBuffer = vertexBuffer.handle();
	}

	void VertexArray::setIndexBuffer(const IndexBuffer &indexBuffer)
	{
		auto &content = state<State>();
		if (content._indexBuffer.identifier() == indexBuffer.identifier())
		{
			return;
		}

		content._indexBuffer = indexBuffer.handle();
	}
}
