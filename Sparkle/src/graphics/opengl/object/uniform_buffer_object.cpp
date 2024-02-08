#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk::OpenGL
{
	UniformBufferObject::UniformBufferObject() :
		VertexBufferObject(VertexBufferObject::Mode::UniformBlock),
		_blockIndex(0),
		_blockBinding(0) 
	{

	}

	UniformBufferObject::UniformBufferObject(const GLuint& p_program, const std::string& p_uniformType, size_t p_blockBinding) :
		VertexBufferObject(VertexBufferObject::Mode::UniformBlock),
        _blockIndex(0),
        _blockBinding(static_cast<GLuint>(p_blockBinding)) 
    {
        _blockIndex = glGetUniformBlockIndex(p_program, p_uniformType.c_str());
        if (_blockIndex == GL_INVALID_INDEX)
		{
            throwException("Uniform block '" + p_uniformType + "' not found in shader.");
        }

        glUniformBlockBinding(p_program, _blockIndex, _blockBinding);
    }

	void UniformBufferObject::activate()
	{
		VertexBufferObject::activate();
		glBindBufferBase(GL_UNIFORM_BUFFER, _blockBinding, handle());
	}
	
	void UniformBufferObject::deactivate()
	{
		VertexBufferObject::deactivate();
		glBindBufferBase(GL_UNIFORM_BUFFER, _blockBinding, 0);
	}
}