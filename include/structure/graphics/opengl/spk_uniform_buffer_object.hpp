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

        void activate();

		template<typename TType>
		TType get()
		{
		DEBUG_LINE();
			activate();

		DEBUG_LINE();
			size_t totalSize = this->size(); 
			if (totalSize == 0)
			{
				return {};
			}
		DEBUG_LINE();

			if (size() % sizeof(TType) != 0)
			{
				throw std::runtime_error(
					"UniformBufferObject::get() - The buffer size (" + std::to_string(size()) +
					" bytes) is incompatible with TType size (" + std::to_string(sizeof(TType)) + " bytes)."
				);
			}
		DEBUG_LINE();

			TType result;
		DEBUG_LINE();

			glGetBufferSubData(GL_ARRAY_BUFFER, 0, totalSize, &result);
		DEBUG_LINE();

			return result;
		}
	};
}
