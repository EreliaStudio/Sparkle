#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"

#include "spk_debug_macro.hpp"

namespace spk::OpenGL
{

	UniformBufferObject::UniformBufferObject(const std::string &p_name, BindingPoint p_bindingPoint, size_t p_size) :
		BindedBufferObject(VertexBufferObject::Type::Uniform, VertexBufferObject::Usage::Dynamic, p_name, p_bindingPoint, p_size)
	{

	}

	UniformBufferObject::UniformBufferObject(const UniformBufferObject& p_other)
	{
		
	}
	
	UniformBufferObject::UniformBufferObject(UniformBufferObject&& p_other)
	{

	}

	UniformBufferObject& UniformBufferObject::operator =(const UniformBufferObject& p_other)
	{

	}
	
	UniformBufferObject& UniformBufferObject::operator =(UniformBufferObject&& p_other)
	{
		if (this != &p_other)
		{
			BindedBufferObject::operator=(std::move(p_other));
		}

		return(*this);
	}

    void UniformBufferObject::activate()
    {
		BindedBufferObject::activate();

		GLint prog = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

		if (prog == 0)
			throw std::runtime_error("No shader program is currently bound.");

		if (_programBlockIndex.contains(prog) == false)
		{
			GLint blockIndex = glGetUniformBlockIndex(prog, _name.c_str());
			if (blockIndex == GL_INVALID_INDEX)
				throw std::runtime_error("Uniform block '" + _name + "' not found in the shader program.");

			_programBlockIndex[prog] = blockIndex;
		}

		glUniformBlockBinding(prog, _programBlockIndex[prog], _bindingPoint);
        glBindBufferBase(GL_UNIFORM_BUFFER, _bindingPoint, _id);
    }
}
