#include "structure/graphics/opengl/spk_shader_storage_buffer_object.hpp"

#include "spk_debug_macro.hpp"

namespace spk::OpenGL
{
	ShaderStorageBufferObject::ShaderStorageBufferObject() :
		BindedBufferObject()
	{
	}

	ShaderStorageBufferObject::ShaderStorageBufferObject(const std::string &p_name, BindingPoint p_bindingPoint, size_t p_fixedSize, size_t p_fixedPadding, size_t p_dynamicElementSize, size_t p_dynamicElementPadding) :
		BindedBufferObject(VertexBufferObject::Type::ShaderStorage, VertexBufferObject::Usage::Dynamic, p_name, p_bindingPoint, p_fixedSize),
		_fixedSize(p_fixedSize),
		_fixedPadding(p_fixedPadding),
		_dynamicElementSize(p_dynamicElementSize),
		_dynamicPadding(p_dynamicElementPadding),
		_dynamicArray(static_cast<uint8_t*>(data()) + p_fixedSize + p_fixedPadding, p_dynamicElementSize)
	{
		_dynamicArray._content.emplace<std::vector<ShaderStorageBufferObject::Element>>();
	}

	void ShaderStorageBufferObject::activate()
	{
		BindedBufferObject::activate();

		GLint prog = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

		if (prog == 0)
			throw std::runtime_error("No shader program is currently bound.");

		GLint blockIndex = glGetProgramResourceIndex(prog, GL_SHADER_STORAGE_BLOCK, _name.c_str());
		if (blockIndex == GL_INVALID_INDEX)
			throw std::runtime_error("Shader storage block '" + _name + "' not found in the shader program.");

		glShaderStorageBlockBinding(prog, blockIndex, _bindingPoint);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _bindingPoint, _id);
	}

	void ShaderStorageBufferObject::resizeDynamicArray(size_t arraySize)
	{
		uint8_t *initialBuffer = static_cast<uint8_t*>(data());

		size_t newSize = _fixedSize + _fixedPadding + (_dynamicElementSize + _dynamicPadding) * arraySize;
	
		resize(newSize);

		for (auto& [key, element] : _elements)
		{
			size_t offset = element._buffer - initialBuffer;
			element._buffer = static_cast<uint8_t*>(data()) + offset;
		}

		std::vector<Element>& contentElement = std::get<std::vector<Element>>(_dynamicArray._content);
		
		_dynamicArray._buffer = static_cast<uint8_t*>(data()) + _fixedSize + _fixedPadding;

		contentElement.resize(arraySize);

		for (size_t i = 0; i < arraySize; ++i)
		{
			size_t offset = (i * (_dynamicElementSize + _dynamicPadding));
			contentElement[i] = Element(static_cast<uint8_t *>(_dynamicArray._buffer) + offset, _dynamicElementSize);
		}
	}

	void ShaderStorageBufferObject::pop_back()
	{
		auto& dynArray = std::get<std::vector<Element>>(_dynamicArray._content);
		if (dynArray.empty())
		{
			throw std::runtime_error("ShaderStorageBufferObject::pop_back() - Cannot pop_back from an empty dynamic array.");
		}

		size_t newSize = dynArray.size() - 1;
		resizeDynamicArray(newSize);
	}

	ShaderStorageBufferObject::Element& ShaderStorageBufferObject::dynamicArray()
	{
		return (_dynamicArray);
	}
}