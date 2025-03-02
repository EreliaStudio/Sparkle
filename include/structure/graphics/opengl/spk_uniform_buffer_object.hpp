#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <cstring>

#include "spk_sfinae.hpp"

#include "structure/graphics/opengl/spk_binded_buffer_object.hpp"

#include "spk_debug_macro.hpp"

namespace spk::OpenGL
{
    class UniformBufferObject : public BindedBufferObject
    {
	private:
		std::unordered_map<GLint, GLint> _programBlockIndex;
    public:
		UniformBufferObject() = default;
        UniformBufferObject(const std::string& p_name, BindingPoint p_bindingPoint, size_t p_size);

		UniformBufferObject(const UniformBufferObject& p_other);
		UniformBufferObject(UniformBufferObject&& p_other);

		UniformBufferObject& operator =(const UniformBufferObject& p_other);
		UniformBufferObject& operator =(UniformBufferObject&& p_other);

        void activate();

        template<typename TType>
        BindedBufferObject& operator=(const TType& p_data)
        {
            if (sizeof(p_data) != _blockSize)
            {
                throw std::runtime_error("Invalid data size passed to ShaderStorageBufferObject. Expected size of [" + std::to_string(sizeof(p_data)) + "] bytes and received [" + std::to_string(_blockSize) + "]");
            }
            edit(&p_data, _blockSize, 0);
            return (*this);
        }

		template<typename TType>
		TType get()
		{
			activate();

			size_t totalSize = this->size(); 
			if (totalSize == 0)
			{
				return {};
			}

			if (size() % sizeof(TType) != 0)
			{
				throw std::runtime_error(
					"UniformBufferObject::get() - The buffer size (" + std::to_string(size()) +
					" bytes) is incompatible with TType size (" + std::to_string(sizeof(TType)) + " bytes)."
				);
			}

			TType result;

			glGetBufferSubData(GL_UNIFORM_BUFFER, 0, totalSize, &result);

			return result;
		}
	};
}
