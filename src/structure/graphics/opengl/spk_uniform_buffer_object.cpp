#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"

#include "utils/spk_string_utils.hpp"
#include "spk_debug_macro.hpp"

namespace spk::OpenGL
{

	UniformBufferObject::UniformBufferObject(const std::wstring &p_name, BindingPoint p_bindingPoint, size_t p_size) :
		VertexBufferObject(VertexBufferObject::Type::Uniform, VertexBufferObject::Usage::Dynamic),
		_blockName(p_name),
		_bindingPoint(p_bindingPoint),
		_dataBufferLayout(p_name, &(dataBuffer()))
	{
		resize(p_size);
	}

	UniformBufferObject::UniformBufferObject(const UniformBufferObject& p_other) :
		VertexBufferObject(p_other),
		_blockName(p_other._blockName),
		_bindingPoint(p_other._bindingPoint),
		_dataBufferLayout(p_other._dataBufferLayout)
	{
		_dataBufferLayout.setBuffer(&(dataBuffer()));
		_dataBufferLayout.updateRootSize();
	}
	
	UniformBufferObject::UniformBufferObject(UniformBufferObject&& p_other) :
		VertexBufferObject(p_other),
		_blockName(std::move(p_other._blockName)),
		_bindingPoint(std::move(p_other._bindingPoint)),
		_dataBufferLayout(std::move(p_other._dataBufferLayout))
	{
		_dataBufferLayout.setBuffer(&(dataBuffer()));
		_dataBufferLayout.updateRootSize();
	}

	UniformBufferObject& UniformBufferObject::operator =(const UniformBufferObject& p_other)
	{
		if (this != &p_other)
		{
			VertexBufferObject::operator=(p_other);
			_blockName = p_other._blockName;
			_bindingPoint = p_other._bindingPoint;
			_dataBufferLayout = std::move(p_other._dataBufferLayout);
			_dataBufferLayout.setBuffer(&(dataBuffer()));
			_dataBufferLayout.updateRootSize();
		}

		return (*this);
	}
	
	UniformBufferObject& UniformBufferObject::operator =(UniformBufferObject&& p_other)
	{
		if (this != &p_other)
		{
			VertexBufferObject::operator=(std::move(p_other));

			_blockName = std::move(p_other._blockName);
			_bindingPoint = std::move(p_other._bindingPoint);
			_dataBufferLayout = std::move(p_other._dataBufferLayout);
			_dataBufferLayout.setBuffer(&(dataBuffer()));
			_dataBufferLayout.updateRootSize();
		}

		return (*this);
	}

	const std::wstring& UniformBufferObject::blockName() const
	{
		return (_blockName);
	}

	void UniformBufferObject::setBlockName(const std::wstring& p_blockName)
	{
		_blockName = p_blockName;
	}

	UniformBufferObject::BindingPoint UniformBufferObject::bindingPoint() const
	{
		return (_bindingPoint);
	}
	void UniformBufferObject::setBindingPoint(BindingPoint p_bindingPoint)
	{
		_bindingPoint = p_bindingPoint;
	}

	DataBufferLayout& UniformBufferObject::layout()
	{
		return (_dataBufferLayout);
	}

	const DataBufferLayout& UniformBufferObject::layout() const
	{
		return (_dataBufferLayout);
	}

	bool UniformBufferObject::contains(const std::wstring& p_name)
	{
		return (_dataBufferLayout.contains(p_name));
	}

	void UniformBufferObject::resize(size_t p_size)
	{
		VertexBufferObject::resize(p_size);
		_dataBufferLayout.updateRootSize();
	}

	DataBufferLayout::Element& UniformBufferObject::addElement(const std::wstring& p_name, size_t p_offset, size_t p_size)
	{
		if (size() <= (p_offset + p_size))
		{
			resize(p_offset + p_size);
		}
		return (_dataBufferLayout.addElement(p_name, p_offset, p_size));
	}

	DataBufferLayout::Element& UniformBufferObject::addElement(const std::wstring& p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding)
	{
		if (size() <= (p_offset + p_nbElement * (p_elementSize + p_elementPadding)))
		{
			resize(p_offset + p_nbElement * (p_elementSize + p_elementPadding));
		}
		return (_dataBufferLayout.addElement(p_name, p_offset, p_nbElement, p_elementSize, p_elementPadding));
	}

	void UniformBufferObject::removeElement(const std::wstring& p_name)
	{
		_dataBufferLayout.removeElement(p_name);
	}

	DataBufferLayout::Element& UniformBufferObject::operator[](size_t index)
	{
		return _dataBufferLayout[index];
	}

	const DataBufferLayout::Element& UniformBufferObject::operator[](size_t index) const
	{
		return _dataBufferLayout[index];
	}

	DataBufferLayout::Element& UniformBufferObject::operator[](const std::wstring& key)
	{
		return _dataBufferLayout[key];
	}

	const DataBufferLayout::Element& UniformBufferObject::operator[](const std::wstring& key) const
	{
		return _dataBufferLayout[key];
	}

    void UniformBufferObject::activate()
    {
		VertexBufferObject::activate();

		GLint prog = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

		if (prog == 0)
			throw std::runtime_error("No shader program is currently bound.");

		if (_programBlockIndex.contains(prog) == false)
		{
			std::string str = spk::StringUtils::wstringToString(_blockName);

			GLint blockIndex = glGetUniformBlockIndex(prog, str.c_str());

			if (blockIndex == GL_INVALID_INDEX)
				throw std::runtime_error("Uniform block '" + str + "' not found in the shader program.");

			_programBlockIndex[prog] = blockIndex;
		}

		glUniformBlockBinding(prog, _programBlockIndex[prog], _bindingPoint);
        glBindBufferBase(GL_UNIFORM_BUFFER, _bindingPoint, _id);
    }
}
