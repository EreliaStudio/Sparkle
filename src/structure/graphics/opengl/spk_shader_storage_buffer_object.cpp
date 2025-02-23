#include "structure/graphics/opengl/spk_shader_storage_buffer_object.hpp"

namespace spk::OpenGL
{
    ShaderStorageBufferObject::ShaderStorageBufferObject() :
		BindedBufferObject(),
        _fixedSize(0),
        _fixedPadding(0),
        _fixedData(this, static_cast<uint8_t*>(data()), 0),
        _dynamicArray(this, static_cast<uint8_t*>(data()), 0, 0)
    {
    }

    ShaderStorageBufferObject::ShaderStorageBufferObject(const std::string &p_name, BindingPoint p_bindingPoint, size_t p_fixedSize, size_t p_fixedPadding, size_t p_dynamicElementSize, size_t p_dynamicPadding) :
		BindedBufferObject(VertexBufferObject::Type::ShaderStorage, VertexBufferObject::Usage::Dynamic, p_name, p_bindingPoint, p_fixedSize),
        _fixedSize(p_fixedSize),
        _fixedPadding(p_fixedPadding),
        _fixedData(this, static_cast<uint8_t*>(data()), p_fixedSize),
        _dynamicArray(this, static_cast<uint8_t*>(data()) + p_fixedSize + p_fixedPadding, p_dynamicElementSize, p_dynamicPadding)
    {
        std::get<std::vector<Element>>(_dynamicArray._element._content);
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

    void ShaderStorageBufferObject::DynamicArray::resize(size_t arraySize)
    {
        uint8_t* initialBuffer = static_cast<uint8_t*>(_parent->data());
        size_t newSize = _parent->_fixedSize + _parent->_fixedPadding + (_dynamicElementSize + _dynamicPadding) * arraySize;
        _parent->resize(newSize);

        for (auto& [key, element] : _parent->_elements)
        {
            size_t offset = element._buffer - initialBuffer;
            element._buffer = static_cast<uint8_t*>(_parent->data()) + offset;
        }

        _element._buffer = static_cast<uint8_t*>(_parent->data()) + _parent->_fixedSize + _parent->_fixedPadding;

        auto& vec = std::get<std::vector<Element>>(_element._content);
        vec.resize(arraySize);
        for (size_t i = 0; i < arraySize; ++i)
        {
            size_t offset = i * (_dynamicElementSize + _dynamicPadding);
            vec[i] = Element(static_cast<uint8_t*>(_element._buffer) + offset, _dynamicElementSize);
        }
    }

    void ShaderStorageBufferObject::DynamicArray::pop_back()
    {
        auto& vec = std::get<std::vector<Element>>(_element._content);
        if (vec.empty())
            throw std::runtime_error("DynamicArray::pop_back() - Cannot pop_back from an empty dynamic array.");
        resize(vec.size() - 1);
    }
}
