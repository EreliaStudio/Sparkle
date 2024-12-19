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
	};
}
