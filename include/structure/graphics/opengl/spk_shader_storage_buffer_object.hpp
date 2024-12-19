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
    class ShaderStorageBufferObject : public BindedBufferObject
    {
	private:
		Element _dynamicArray;

    public:
        ShaderStorageBufferObject();
        ShaderStorageBufferObject(const std::string& typeName, BindingPoint bindingPoint, size_t p_fixedSize, size_t p_dynamicElementSize);

        void activate() override;

		Element& dynamicArray();
		void resizeDynamicArray(size_t arraySize);
    };
}
