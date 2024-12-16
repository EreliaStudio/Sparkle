#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"

namespace spk::OpenGL
{
    UniformBufferObject::Element::Element(uint8_t* buffer, size_t size)
        : _buffer(buffer), _size(size)
    {
    }

    uint8_t* UniformBufferObject::Element::data()
    {
        return _buffer;
    }

    size_t UniformBufferObject::Element::size() const
    {
        return _size;
    }

    UniformBufferObject::Element& UniformBufferObject::Element::operator[](size_t index)
    {
        if (!std::holds_alternative<std::vector<Element>>(_content))
            throw std::runtime_error("Element is not an array.");

        auto& elements = std::get<std::vector<Element>>(_content);
        if (index >= elements.size())
            throw std::out_of_range("Array index out of bounds.");

        return elements[index];
    }

    const UniformBufferObject::Element& UniformBufferObject::Element::operator[](size_t index) const
    {
        if (!std::holds_alternative<std::vector<Element>>(_content))
            throw std::runtime_error("Element is not an array.");

        const auto& elements = std::get<std::vector<Element>>(_content);
        if (index >= elements.size())
            throw std::out_of_range("Array index out of bounds.");

        return elements[index];
    }

    UniformBufferObject::Element& UniformBufferObject::Element::addElement(const std::wstring& name, uint8_t* buffer, size_t elementSize, size_t arraySize)
    {
        if (!std::holds_alternative<std::unordered_map<std::wstring, Element>>(_content))
            _content = std::unordered_map<std::wstring, Element>();

        auto& subElements = std::get<std::unordered_map<std::wstring, Element>>(_content);

        if (subElements.find(name) != subElements.end())
            throw std::runtime_error("Element '" + std::string(name.begin(), name.end()) + "' already exists.");

        if (arraySize > 0)
        {
            std::vector<Element> elements;
            elements.reserve(arraySize);
            for (size_t i = 0; i < arraySize; ++i)
            {
                elements.emplace_back(Element(buffer + (i * elementSize), elementSize));
            }
            Element arrayElement(nullptr, 0);
            arrayElement._content = std::move(elements);

            auto [it, inserted] = subElements.emplace(name, std::move(arrayElement));
            return it->second;
        }
        else
        {
            auto [it, inserted] = subElements.emplace(name, Element(buffer, elementSize));
            return it->second;
        }
    }

    UniformBufferObject::Element& UniformBufferObject::Element::operator[](const std::wstring& name)
    {
        if (!std::holds_alternative<std::unordered_map<std::wstring, Element>>(_content))
            throw std::runtime_error("Element is not an object.");

        auto& subElements = std::get<std::unordered_map<std::wstring, Element>>(_content);
        auto it = subElements.find(name);
        if (it == subElements.end())
            throw std::runtime_error("Sub-element '" + std::string(name.begin(), name.end()) + "' not found.");

        return it->second;
    }

    UniformBufferObject::UniformBufferObject(const std::string& typeName, BindingPoint bindingPoint, size_t p_size)
        : VertexBufferObject(VertexBufferObject::Type::Uniform, VertexBufferObject::Usage::Dynamic),
          _typeName(typeName),
          _bindingPoint(bindingPoint),
          _blockSize(p_size)
    {
        resize(p_size);
    }

    UniformBufferObject::~UniformBufferObject()
    {
        deactivate();
    }

    void UniformBufferObject::activate()
    {
        VertexBufferObject::activate();

        GLint prog = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

        if (prog == 0)
            throw std::runtime_error("No shader program is currently bound.");

        GLint blockIndex = glGetUniformBlockIndex(prog, _typeName.c_str());
        if (blockIndex == GL_INVALID_INDEX)
            throw std::runtime_error("Uniform block '" + _typeName + "' not found in the shader program.");

        glUniformBlockBinding(prog, blockIndex, _bindingPoint);
        glBindBufferBase(GL_UNIFORM_BUFFER, _bindingPoint, _id);
    }

    UniformBufferObject::Element& UniformBufferObject::addElement(const std::wstring& name, size_t offset, size_t elementSize, size_t arraySize)
    {
        uint8_t* bufferPtr = static_cast<uint8_t*>(data()) + offset;

        if (_elements.find(name) != _elements.end())
            throw std::runtime_error("Element '" + std::string(name.begin(), name.end()) + "' already exists.");

        if (arraySize > 0)
        {
            std::vector<Element> elements;
            elements.reserve(arraySize);
            for (size_t i = 0; i < arraySize; ++i)
            {
                elements.emplace_back(Element(bufferPtr + (i * elementSize), elementSize));
            }
            Element arrayElement(nullptr, 0);
            arrayElement._content = std::move(elements);

            auto [it, inserted] = _elements.emplace(name, std::move(arrayElement));
            return it->second;
        }
        else
        {
            auto [it, inserted] = _elements.emplace(name, Element(bufferPtr, elementSize));
            return it->second;
        }
    }

    UniformBufferObject::Element& UniformBufferObject::operator[](const std::wstring& name)
    {
        return _elements[name];
    }

    const UniformBufferObject::Element& UniformBufferObject::operator[](const std::wstring& name) const
    {
        return _elements.at(name);
    }
}
