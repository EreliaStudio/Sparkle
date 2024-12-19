#include "structure/graphics/opengl/spk_shader_storage_buffer_object.hpp"

namespace spk::OpenGL
{
	ShaderStorageBufferObject::ShaderStorageBufferObject() :
		BindedBufferObject()
	{
	}

	ShaderStorageBufferObject::ShaderStorageBufferObject(const std::string &p_name, BindingPoint p_bindingPoint, size_t p_fixedSize, size_t p_dynamicElementSize) :
		BindedBufferObject(VertexBufferObject::Type::ShaderStorage, VertexBufferObject::Usage::Dynamic, p_name, p_bindingPoint, p_fixedSize),
		_dynamicArray(static_cast<uint8_t*>(data()) + p_fixedSize, p_dynamicElementSize)
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
		auto& element = _dynamicArray;

		if (std::holds_alternative<std::monostate>(element._content) ||
			std::holds_alternative<std::unordered_map<std::string, Element>>(element._content))
			throw std::runtime_error("Element is not an array.");

		if (arraySize > element.nbElement())
		{
			size_t sizeToAdd = element._size * (arraySize - element.nbElement());
		
			resize(size() + sizeToAdd);
		}

		std::vector<Element> elements;
		elements.reserve(arraySize);
		for (size_t i = 0; i < arraySize; ++i)
		{
			elements.emplace_back(Element(element._buffer + (i * element._size), element._size));
		}
		std::get<std::vector<Element>>(element._content).swap(elements);
	}

	ShaderStorageBufferObject::Element& ShaderStorageBufferObject::dynamicArray()
	{
		return (_dynamicArray);
	}
}