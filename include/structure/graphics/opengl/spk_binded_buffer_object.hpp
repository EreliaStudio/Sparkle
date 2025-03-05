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
#include "structure/graphics/opengl/spk_vertex_buffer_object.hpp"

#include "structure/container/spk_data_buffer_layout.hpp"

#include "iostream"

#include "spk_debug_macro.hpp"

namespace spk::OpenGL
{
    class BindedBufferObject : public VertexBufferObject
    {
    public:
        using BindingPoint = int;
		using Element = spk::DataBufferLayout::Element;

    protected:
		std::string _blockName ="Unnamed binded buffer object";
        BindingPoint _bindingPoint = -1;
		spk::DataBufferLayout _dataBufferLayout;

    public:
		BindedBufferObject() = default;
        
		BindedBufferObject(Type p_type, Usage p_usage, const std::string& p_blockName, BindingPoint p_bindingPoint, size_t p_size) :
			_blockName(p_blockName),
			_bindingPoint(p_bindingPoint),
			_dataBufferLayout(&(dataBuffer()))
		{
			resize(p_size);
		}

        virtual ~BindedBufferObject() = default;

		BindedBufferObject(const BindedBufferObject& p_other) :
			_blockName(p_other._blockName),
			_bindingPoint(p_other._bindingPoint),
			_dataBufferLayout(p_other._dataBufferLayout)
		{
			_dataBufferLayout.setBuffer(&(dataBuffer()));
		}

		BindedBufferObject& operator = (const BindedBufferObject& p_other)
		{
			if (this != &p_other)
			{
				_blockName = p_other._blockName;
				_bindingPoint = p_other._bindingPoint;
				_dataBufferLayout = std::move(p_other._dataBufferLayout);
				_dataBufferLayout.setBuffer(&(dataBuffer()));
			}

			return (*this);
		}

		BindedBufferObject(BindedBufferObject&& p_other) :
			_blockName(std::move(p_other._blockName)),
			_bindingPoint(std::move(p_other._bindingPoint)),
			_dataBufferLayout(std::move(p_other._dataBufferLayout))
		{
			_dataBufferLayout.setBuffer(&(dataBuffer()));
		}

		BindedBufferObject& operator = (BindedBufferObject&& p_other)
		{
			if (this != &p_other)
			{
				_blockName = std::move(p_other._blockName);
				_bindingPoint = std::move(p_other._bindingPoint);
				_dataBufferLayout = std::move(p_other._dataBufferLayout);
				_dataBufferLayout.setBuffer(&(dataBuffer()));
			}

			return (*this);
		}

		const std::string& blockName() const
		{
			return (_blockName);
		}
		void setBlockName(const std::string& p_blockName)
		{
			_blockName = p_blockName;
		}

		BindingPoint bindingPoint() const
		{
			return (_bindingPoint);
		}
		void setBindingPoint(BindingPoint p_bindingPoint)
		{
			_bindingPoint = p_bindingPoint;
		}

		DataBufferLayout& layout()
		{
			return (_dataBufferLayout);
		}

		const DataBufferLayout& layout() const
		{
			return (_dataBufferLayout);
		}

		bool contains(const std::wstring& p_name)
		{
			return (_dataBufferLayout.contains(p_name));
		}

		DataBufferLayout::Element& addElement(const std::wstring& p_name, size_t p_offset, size_t p_size)
		{
			return (_dataBufferLayout.addElement(p_name, p_offset, p_size));
		}

		DataBufferLayout::Element& addElement(const std::wstring& p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding)
		{
			return (_dataBufferLayout.addElement(p_name, p_offset, p_nbElement, p_elementSize, p_elementPadding));
		}

		void removeElement(const std::wstring& p_name)
		{
			_dataBufferLayout.removeElement(p_name);
		}

        DataBufferLayout::Element& operator[](size_t index)
        {
            return _dataBufferLayout[index];
        }

        DataBufferLayout::Element& operator[](const std::wstring& key)
        {
            return _dataBufferLayout[key];
        }

        const DataBufferLayout::Element& operator[](size_t index) const
        {
            return _dataBufferLayout[index];
        }

        const DataBufferLayout::Element& operator[](const std::wstring& key) const
        {
            return _dataBufferLayout[key];
        }

		virtual void activate()
		{
			VertexBufferObject::activate();
		}
    };
}
