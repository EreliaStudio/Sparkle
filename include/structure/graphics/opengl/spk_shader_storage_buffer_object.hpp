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
	public:
		template<typename TFixedType, typename TDynamicType>
		struct Content
		{
			TFixedType fixed;
			std::vector<TDynamicType> dynamic;
		};

	private:
		size_t _fixedSize;
		size_t _dynamicElementSize;
		Element _dynamicArray;

    public:
        ShaderStorageBufferObject();
        ShaderStorageBufferObject(const std::string& typeName, BindingPoint bindingPoint, size_t p_fixedSize, size_t p_dynamicElementSize);

        void activate() override;

		Element& dynamicArray();
		void resizeDynamicArray(size_t arraySize);

		template<typename TFixedType, typename TDynamicType>
		Content<TFixedType, TDynamicType> get()
		{
			activate();

			if (_fixedSize % sizeof(TFixedType) != 0)
			{
				throw std::runtime_error(
					"ShaderStorageBufferObject::get() - The fixed buffer section size (" + std::to_string(_fixedSize) +
					" bytes) is incompatible with TFixedType size (" + std::to_string(sizeof(TFixedType)) + " bytes)."
				);
			}

			if (_dynamicElementSize % sizeof(TDynamicType) != 0)
			{
				throw std::runtime_error(
					"ShaderStorageBufferObject::get() - The dynamic buffer section element size (" + std::to_string(_dynamicElementSize) +
					" bytes) is incompatible with TDynamicType size (" + std::to_string(sizeof(TDynamicType)) + " bytes)."
				);
			}

			Content<TFixedType, TDynamicType> result;

			result.dynamic.resize(_dynamicArray.nbElement());

			/*
			Retrieve fixed buffer content
			*/
		
			/*
			Retrieve dynamic buffer content
			*/

			return std::move(result);
		}
    };
}
