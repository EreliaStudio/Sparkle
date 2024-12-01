#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"

#include "utils/spk_string_utils.hpp"

#include <bitset>
#include <iostream>
#include <cstring> // For std::memcpy

namespace spk::OpenGL
{
	// UniformBufferObject::Layout implementation

	UniformBufferObject::Layout::Layout()
		: _destination(nullptr), _cpu{ 0, 0 }, _gpu{ 0, 0 }, _tightlyPacked(false) {}

	UniformBufferObject::Layout::Layout(const DataRepresentation& p_cpu, const DataRepresentation& p_gpu)
		: _destination(nullptr), _cpu(p_cpu), _gpu(p_gpu), _tightlyPacked(p_cpu.size == p_gpu.size) {}

	void UniformBufferObject::Layout::bind(char* p_destination)
	{
		_tightlyPacked = (_cpu.size == _gpu.size);
		_destination = p_destination;
		for (auto& [key, element] : _innerLayouts)
		{
			for (auto& member : element.layouts)
			{
				member.bind(p_destination + member._gpu.offset);
			}
		}
	}

	UniformBufferObject::Element& UniformBufferObject::Layout::operator[](const std::wstring& p_name)
	{
		auto it = _innerLayouts.find(p_name);
		if (it == _innerLayouts.end())
			throw std::runtime_error("No layout named [" + spk::StringUtils::wstringToString(p_name) + "] found.");
		return it->second;
	}

	void UniformBufferObject::Layout::_pushData(const char* p_basePtr)
	{
		if (_tightlyPacked)
		{
			std::memcpy(_destination, p_basePtr, _cpu.size);
		}
		else
		{
			for (auto& [name, element] : _innerLayouts)
			{
				for (auto& layout : element.layouts)
				{
					layout._pushData(p_basePtr + layout._cpu.offset);
				}
			}
		}
	}

	// UniformBufferObject::Factory implementation

	void UniformBufferObject::Factory::setTypeName(const std::string& p_name)
	{
		_typeName = p_name;
	}

	void UniformBufferObject::Factory::setBindingPoint(BindingPoint p_bindingPoint)
	{
		_bindingPoint = p_bindingPoint;
	}

	const std::string& UniformBufferObject::Factory::typeName() const
	{
		return _typeName;
	}

	UniformBufferObject::BindingPoint UniformBufferObject::Factory::bindingPoint() const
	{
		return _bindingPoint;
	}

	UniformBufferObject::Layout& UniformBufferObject::Factory::mainLayout()
	{
		return _layout;
	}

	const UniformBufferObject::Layout& UniformBufferObject::Factory::mainLayout() const
	{
		return _layout;
	}

	UniformBufferObject::Layout& UniformBufferObject::Factory::addInnerLayout(Layout& p_layout, const std::wstring& p_name, const Layout::DataRepresentation& p_cpu, const Layout::DataRepresentation& p_gpu)
	{
		p_layout._innerLayouts[p_name].layouts.emplace_back(p_cpu, p_gpu);
		return p_layout._innerLayouts[p_name].layouts.back();
	}

	UniformBufferObject UniformBufferObject::Factory::construct() const
	{
		UniformBufferObject result;

		result._typeName = _typeName;
		result._bindingPoint = _bindingPoint;
		result._layout = _layout;
		result.resize(result._layout._gpu.size);
		result._layout.bind(static_cast<char*>(result.data()));

		return std::move(result);
	}

	UniformBufferObject::UniformBufferObject()
		: VertexBufferObject(VertexBufferObject::Type::Uniform, VertexBufferObject::Usage::Static),
		_bindingPoint(-1),
		_blockIndex(GL_INVALID_ENUM) {}

	UniformBufferObject::UniformBufferObject(const UniformBufferObject& p_other)
		: VertexBufferObject(p_other),
		_typeName(p_other._typeName),
		_bindingPoint(p_other._bindingPoint),
		_blockIndex(p_other._blockIndex),
		_layout(p_other._layout) 
	{

	}

	UniformBufferObject& UniformBufferObject::operator=(const UniformBufferObject& p_other)
	{
		if (this != &p_other)
		{
			VertexBufferObject::operator=(p_other);

			_typeName = p_other._typeName;
			_bindingPoint = p_other._bindingPoint;
			_blockIndex = p_other._blockIndex;
			_layout = p_other._layout;
		}
		return *this;
	}

	UniformBufferObject::UniformBufferObject(UniformBufferObject&& p_other) noexcept
		: VertexBufferObject(std::move(p_other)),
		_typeName(std::move(p_other._typeName)),
		_bindingPoint(p_other._bindingPoint),
		_blockIndex(p_other._blockIndex),
		_layout(std::move(p_other._layout))
	{
		p_other._bindingPoint = -1;
		p_other._blockIndex = GL_INVALID_ENUM;
	}

	UniformBufferObject& UniformBufferObject::operator=(UniformBufferObject&& p_other) noexcept
	{
		if (this != &p_other)
		{
			VertexBufferObject::operator=(std::move(p_other));

			_typeName = std::move(p_other._typeName);
			_bindingPoint = p_other._bindingPoint;
			_blockIndex = p_other._blockIndex;
			_layout = std::move(p_other._layout);

			p_other._bindingPoint = -1;
			p_other._blockIndex = GL_INVALID_ENUM;
		}
		return *this;
	}

	const std::string& UniformBufferObject::typeName() const
	{
		return _typeName;
	}

	const UniformBufferObject::BindingPoint& UniformBufferObject::bindingPoint() const
	{
		return _bindingPoint;
	}

	void UniformBufferObject::activate()
	{
		VertexBufferObject::activate();

		if (_blockIndex == GL_INVALID_ENUM)
		{
			GLint prog = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

			_blockIndex = glGetUniformBlockIndex(prog, _typeName.c_str());

			glUniformBlockBinding(prog, _blockIndex, _bindingPoint);
		}

		glBindBufferBase(GL_UNIFORM_BUFFER, _bindingPoint, _id);
	}

	UniformBufferObject::Element& UniformBufferObject::operator[](const std::wstring& p_name)
	{
		return _layout[p_name];
	}
}
