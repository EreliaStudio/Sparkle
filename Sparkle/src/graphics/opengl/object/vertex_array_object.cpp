#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk::OpenGL
{
	VertexArrayObject::VertexArrayObject()
	{
        glGenVertexArrays(1, &_handle);
        activate();
    }

	VertexArrayObject::~VertexArrayObject()
	{
        glDeleteVertexArrays(1, &_handle);
    }

    void VertexArrayObject::activate()
	{
        glBindVertexArray(_handle);
    }

    void VertexArrayObject::deactivate()
	{
        glBindVertexArray(0);
    }

    GLuint VertexArrayObject::handle() const
	{
        return _handle;
    }
}