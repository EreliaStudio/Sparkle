#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"

#include "spk_debug_macro.hpp"
#include "utils/spk_string_utils.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk::OpenGL
{
	void UniformBufferObject::_loadElement(const spk::JSON::Object &p_elemDesc)
	{
		std::wstring name = p_elemDesc[L"Name"].as<std::wstring>();
		size_t offset = static_cast<size_t>(p_elemDesc[L"Offset"].as<long>());

		if (p_elemDesc.contains(L"Size") == true)
		{
			size_t size = static_cast<size_t>(p_elemDesc[L"Size"].as<long>());
			auto &elem = addElement(name, offset, size);

			if (p_elemDesc.contains(L"NestedElements") == true)
			{
				for (auto *child : p_elemDesc[L"NestedElements"].asArray())
				{
					_loadElement(elem, *child);
				}
			}
		}
		else if (p_elemDesc.contains(L"NbElements") == true)
		{
			size_t nbElem = static_cast<size_t>(p_elemDesc[L"NbElements"].as<long>());
			size_t elemSize = static_cast<size_t>(p_elemDesc[L"ElementSize"].as<long>());
			size_t padding = static_cast<size_t>(p_elemDesc[L"ElementPadding"].as<long>());

			auto &elemArr = addElement(name, offset, nbElem, elemSize, padding);

			if (p_elemDesc.contains(L"ElementComposition") == true)
			{
				for (size_t i = 0; i < elemArr.nbElement(); ++i)
				{
					for (auto *comp : p_elemDesc[L"ElementComposition"].asArray())
					{
						_loadElement(elemArr[i], *comp);
					}
				}
			}
		}
		else
		{
			GENERATE_ERROR("[UniformBufferObject] - Element description must contain either 'Size' or 'NbElements'.");
		}
	}

	void UniformBufferObject::_loadElement(spk::DataBufferLayout::Element &p_parent, const spk::JSON::Object &p_elemDesc)
	{
		std::wstring name = p_elemDesc[L"Name"].as<std::wstring>();
		size_t offset = p_parent.offset() + static_cast<size_t>(p_elemDesc[L"Offset"].as<long>());

		if (p_elemDesc.contains(L"Size") == true)
		{
			size_t size = static_cast<size_t>(p_elemDesc[L"Size"].as<long>());
			auto &elem = p_parent.addElement(name, offset, size);

			if (p_elemDesc.contains(L"NestedElements") == true)
			{
				for (auto *child : p_elemDesc[L"NestedElements"].asArray())
				{
					_loadElement(elem, *child);
				}
			}
		}
		else if (p_elemDesc.contains(L"NbElements") == true)
		{
			size_t nbElem = static_cast<size_t>(p_elemDesc[L"NbElements"].as<long>());
			size_t elemSize = static_cast<size_t>(p_elemDesc[L"ElementSize"].as<long>());
			size_t padding = static_cast<size_t>(p_elemDesc[L"ElementPadding"].as<long>());

			auto &elemArr = p_parent.addElement(name, offset, nbElem, elemSize, padding);

			if (p_elemDesc.contains(L"ElementComposition") == true)
			{
				for (size_t i = 0; i < elemArr.nbElement(); ++i)
				{
					for (auto *comp : p_elemDesc[L"ElementComposition"].asArray())
					{
						_loadElement(elemArr[i], *comp);
					}
				}
			}
		}
		else
		{
			GENERATE_ERROR("[UniformBufferObject] - Element description must contain either 'Size' or 'NbElements'.");
		}
	}

	UniformBufferObject::UniformBufferObject(const std::wstring &p_blockName, BindingPoint p_bindingPoint, size_t p_size) :
		VertexBufferObject(VertexBufferObject::Type::Uniform, VertexBufferObject::Usage::Dynamic),
		_blockName(p_blockName),
		_bindingPoint(p_bindingPoint),
		_dataBufferLayout(p_blockName, &(dataBuffer()))
	{
		resize(p_size);
	}

	UniformBufferObject::UniformBufferObject(const spk::JSON::Object &p_desc) :
		UniformBufferObject(p_desc[L"BlockName"].as<std::wstring>(),
							static_cast<BindingPoint>(p_desc[L"BindingPoint"].as<long>()),
							static_cast<size_t>(p_desc[L"Size"].as<long>()))
	{
		if (p_desc.contains(L"Elements") == true)
		{
			for (auto *elem : p_desc[L"Elements"].asArray())
			{
				_loadElement(*elem);
			}
		}
	}

	UniformBufferObject::UniformBufferObject(const UniformBufferObject &p_other) :
		VertexBufferObject(p_other),
		_blockName(p_other._blockName),
		_bindingPoint(p_other._bindingPoint),
		_dataBufferLayout(p_other._dataBufferLayout)
	{
		_dataBufferLayout.setBuffer(&(dataBuffer()));
		_dataBufferLayout.updateRootSize();
	}

	UniformBufferObject::UniformBufferObject(UniformBufferObject &&p_other) :
		VertexBufferObject(p_other),
		_blockName(std::move(p_other._blockName)),
		_bindingPoint(std::move(p_other._bindingPoint)),
		_dataBufferLayout(std::move(p_other._dataBufferLayout))
	{
		_dataBufferLayout.setBuffer(&(dataBuffer()));
		_dataBufferLayout.updateRootSize();
	}

	UniformBufferObject &UniformBufferObject::operator=(const UniformBufferObject &p_other)
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

	UniformBufferObject &UniformBufferObject::operator=(UniformBufferObject &&p_other)
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

	const std::wstring &UniformBufferObject::blockName() const
	{
		return (_blockName);
	}

	void UniformBufferObject::setBlockName(const std::wstring &p_blockName)
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

	DataBufferLayout &UniformBufferObject::layout()
	{
		return (_dataBufferLayout);
	}

	const DataBufferLayout &UniformBufferObject::layout() const
	{
		return (_dataBufferLayout);
	}

	bool UniformBufferObject::contains(const std::wstring &p_name)
	{
		return (_dataBufferLayout.contains(p_name));
	}

	void UniformBufferObject::resize(size_t p_size)
	{
		VertexBufferObject::resize(p_size);
		_dataBufferLayout.updateRootSize();
	}

	DataBufferLayout::Element &UniformBufferObject::addElement(const std::wstring &p_name, size_t p_offset, size_t p_size)
	{
		if (size() <= (p_offset + p_size))
		{
			resize(p_offset + p_size);
		}
		return (_dataBufferLayout.addElement(p_name, p_offset, p_size));
	}

	DataBufferLayout::Element &
	UniformBufferObject::addElement(const std::wstring &p_name, size_t p_offset, size_t p_nbElement, size_t p_elementSize, size_t p_elementPadding)
	{
		if (size() <= (p_offset + p_nbElement * (p_elementSize + p_elementPadding)))
		{
			resize(p_offset + p_nbElement * (p_elementSize + p_elementPadding));
		}
		return (_dataBufferLayout.addElement(p_name, p_offset, p_nbElement, p_elementSize, p_elementPadding));
	}

	void UniformBufferObject::removeElement(const std::wstring &p_name)
	{
		_dataBufferLayout.removeElement(p_name);
	}

	DataBufferLayout::Element &UniformBufferObject::operator[](size_t p_index)
	{
		return _dataBufferLayout[p_index];
	}

	const DataBufferLayout::Element &UniformBufferObject::operator[](size_t p_index) const
	{
		return _dataBufferLayout[p_index];
	}

	DataBufferLayout::Element &UniformBufferObject::operator[](const std::wstring &p_key)
	{
		return _dataBufferLayout[p_key];
	}

	const DataBufferLayout::Element &UniformBufferObject::operator[](const std::wstring &p_key) const
	{
		return _dataBufferLayout[p_key];
	}

	void UniformBufferObject::activate()
	{
		VertexBufferObject::activate();

		GLint prog = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

		if (prog == 0)
		{
			GENERATE_ERROR("No shader program is currently bound.");
		}

		if (_programBlockIndex.contains(prog) == false)
		{
			std::string str = spk::StringUtils::wstringToString(_blockName);

			GLint blockIndex = glGetUniformBlockIndex(prog, str.c_str());

			if (blockIndex == GL_INVALID_INDEX)
			{
				GENERATE_ERROR("Uniform block '" + str + "' not found in the shader program.");
			}

			_programBlockIndex[prog] = blockIndex;
		}

		glUniformBlockBinding(prog, _programBlockIndex[prog], _bindingPoint);
		glBindBufferBase(GL_UNIFORM_BUFFER, _bindingPoint, _id);
	}
}
