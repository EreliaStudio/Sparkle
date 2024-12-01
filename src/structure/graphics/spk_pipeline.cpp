#include "structure/graphics/spk_pipeline.hpp"

#include "utils/spk_string_utils.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	void Pipeline::Object::_activate()
	{
		_bufferSet.activate();

		for (auto& [name, attribute] : _attributes)
		{
			attribute.activate();
		}

		for (auto& [name, sampler] : _samplers)
		{
			sampler.activate();
		}
	}

	void Pipeline::Object::_deactivate()
	{
		_bufferSet.deactivate();

		for (auto& [name, attribute] : _attributes)
		{
			attribute.deactivate();
		}
	}

	OpenGL::LayoutBufferObject& Pipeline::Object::layout()
	{
		return _bufferSet.layout();
	}

	OpenGL::IndexBufferObject& Pipeline::Object::indexes()
	{
		return _bufferSet.indexes();
	}

	Pipeline::Object::Attribute& Pipeline::Object::attribute(const std::wstring& p_name)
	{
		if (_attributes.find(p_name) == _attributes.end())
		{
			throw std::runtime_error("Attribute [" + StringUtils::wstringToString(p_name) + "] not found inside Object");
		}
		return _attributes.at(p_name);
	}

	Pipeline::Object::Sampler2D& Pipeline::Object::sampler2D(const std::wstring& p_name)
	{
		if (_samplers.find(p_name) == _samplers.end())
		{
			throw std::runtime_error("Sampler2D [" + StringUtils::wstringToString(p_name) + "] not found inside Object");
		}
		return _samplers.at(p_name);
	}
	
	Pipeline::Object::FrameBuffer& Pipeline::Object::frameBuffer()
	{
		return _frameBuffer;
	}

	void Pipeline::Object::render(size_t p_nbInstance)
	{
		if (_owner == nullptr)
		{
			return;
		}

		_owner->_render(*this, p_nbInstance);
	}

	void Pipeline::_render(Object& p_object, size_t p_nbInstance)
	{
		if (p_object.indexes().nbIndexes() == 0)
		{
			return ;
		}

		_program.activate();

		p_object._activate();

		for (auto& constant : _activeConstants)
		{
			constant->activate();
		}

		_program.render(static_cast<GLsizei>(p_object.indexes().nbIndexes()), static_cast<GLsizei>(p_nbInstance));

		p_object._deactivate();

		_program.deactivate();
	}


	Pipeline Pipeline::fromFile(const std::filesystem::path& p_path)
	{
		return (Pipeline(FileUtils::readFileAsString(p_path)));
	}

	Pipeline::Pipeline(const std::string& p_shaderContent) :
		_parser(p_shaderContent),
		_program(_parser.getVertexShaderCode(), _parser.getFragmentShaderCode())
	{
		for (const auto& [key, factory] : _parser.getCurrentConstantFactories())
		{
			if (_constants.contains(key) == true)
			{
				Constant newConstants = factory.construct();

				if (_constants.at(key).size() != newConstants.size())
				{
					throw std::runtime_error("Constants [" + newConstants.typeName() + "] already declared with a different size before spk shader");
				}
			}
			else
			{
				_constants.emplace(key, factory.construct());
			}
			_activeConstants.push_back(&(_constants.at(key)));
		}
	}

	Pipeline::Object Pipeline::createObject()
	{
		Object result;

		result._owner = this;
		result._bufferSet = _parser.getLayoutFactory().construct();

		for (const auto& [key, factory] : _parser.getAttributeFactories())
		{
			result._attributes.emplace(key, factory.construct());
		}

		result._frameBuffer = _parser.getFrameBufferFactory().construct(spk::Vector2UInt(1000, 1000));

		for (const auto& [key, factory] : _parser.getSamplerFactories())
		{
			result._samplers.emplace(key, factory.construct());
		}

		return result;
	}

	Pipeline::Constant& Pipeline::constants(const std::wstring& p_name)
	{
		if (_constants.find(p_name) == _constants.end())
		{
			throw std::runtime_error("Constants [" + StringUtils::wstringToString(p_name) + "] not found inside Pipeline");
		}
		return _constants.at(p_name);
	}
}
