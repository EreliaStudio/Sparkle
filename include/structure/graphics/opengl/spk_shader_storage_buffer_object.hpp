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

        class FixedData
        {
            friend class ShaderStorageBufferObject;
        public:
            Element& operator[](const std::string& name)
            {
                return _element[name];
            }

            const Element& operator[](const std::string& name) const
            {
                return _element[name];
            }

            uint8_t* data() { return _element.data(); }
            const uint8_t* data() const { return _element.data(); }
            size_t size() const { return _element.size(); }

        private:
            FixedData(ShaderStorageBufferObject* parent, uint8_t* buffer, size_t fixedSize) :
				_parent(parent), _element(buffer, fixedSize)
            {
            }
            ShaderStorageBufferObject* _parent;
            Element _element;
        };

		
        class DynamicArray
        {
            friend class ShaderStorageBufferObject;
        public:
            void resize(size_t arraySize);

			void clear();

            void pop_back();

            template<typename TDynamicType>
            void push_back(const TDynamicType& value)
            {
                if (_dynamicElementSize % sizeof(TDynamicType) != 0)
                {
                    throw std::runtime_error(
                        "DynamicArray::push_back() - The dynamic element size (" +
                        std::to_string(_dynamicElementSize) + " bytes) is incompatible with TDynamicType size (" +
                        std::to_string(sizeof(TDynamicType)) + " bytes)."
                    );
                }
                auto& vec = std::get<std::vector<Element>>(_element._content);
                size_t currentSize = vec.size();
                resize(currentSize + 1);
                auto& newVec = std::get<std::vector<Element>>(_element._content);
                newVec[currentSize] = value;
            }

            Element& operator[](size_t index)
            {
                auto& vec = std::get<std::vector<Element>>(_element._content);
                if (index >= vec.size())
                    throw std::out_of_range("DynamicArray::operator[] - Index out of range.");
                return vec[index];
            }

            const Element& operator[](size_t index) const
            {
                const auto& vec = std::get<std::vector<Element>>(_element._content);
                if (index >= vec.size())
                    throw std::out_of_range("DynamicArray::operator[] - Index out of range.");
                return vec[index];
            }

            size_t nbElement() const
            {
                const auto& vec = std::get<std::vector<Element>>(_element._content);
                return vec.size();
            }

            size_t getDynamicElementSize() const { return _dynamicElementSize; }

        private:
            DynamicArray(ShaderStorageBufferObject* parent, uint8_t* buffer, size_t dynamicElementSize, size_t dynamicPadding)
                : _parent(parent), _dynamicElementSize(dynamicElementSize), _dynamicPadding(dynamicPadding)
            {
                _element = Element(buffer, dynamicElementSize);
                _element._content = std::vector<Element>();
            }
            ShaderStorageBufferObject* _parent;
            Element _element;
            size_t _dynamicElementSize;
            size_t _dynamicPadding;
        };

    private:
        size_t _fixedSize;
        size_t _fixedPadding;

        FixedData _fixedData;
        DynamicArray _dynamicArray;

    public:
        // Constructors
        ShaderStorageBufferObject();
        ShaderStorageBufferObject(const std::string& typeName,
                                  BindingPoint bindingPoint,
                                  size_t p_fixedSize,
                                  size_t p_fixedPadding,
                                  size_t p_dynamicElementSize,
                                  size_t p_dynamicPadding);

        void activate() override;

        // Accessors to the nested classes.
        FixedData& fixedData() { return _fixedData; }
        const FixedData& fixedData() const { return _fixedData; }

        DynamicArray& dynamicArray() { return _dynamicArray; }
        const DynamicArray& dynamicArray() const { return _dynamicArray; }

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

            if (_dynamicArray.getDynamicElementSize() % sizeof(TDynamicType) != 0)
            {
                throw std::runtime_error(
                    "ShaderStorageBufferObject::get() - The dynamic buffer section element size (" +
                    std::to_string(_dynamicArray.getDynamicElementSize()) + " bytes) is incompatible with TDynamicType size (" +
                    std::to_string(sizeof(TDynamicType)) + " bytes)."
                );
            }

            Content<TFixedType, TDynamicType> result;
            result.dynamic.resize(_dynamicArray.nbElement());
            return std::move(result);
        }
    };
}
