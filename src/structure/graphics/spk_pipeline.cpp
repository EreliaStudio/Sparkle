#include "structure/graphics/spk_pipeline.hpp"
#include "utils/spk_string_utils.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	Pipeline::Object::Object(spk::SafePointer<Pipeline> p_owner) :
		_owner(p_owner)
	{
	}

	Pipeline::Object::Object() :
		_owner(nullptr)
	{
	}

	void Pipeline::Object::setNbInstance(size_t p_nbInstance)
	{
		_nbInstance = p_nbInstance;
	}

	spk::OpenGL::BufferSet &Pipeline::Object::bufferSet()
	{
		return _bufferSet;
	}

	spk::OpenGL::UniformBufferObject &Pipeline::Object::ubo(const std::wstring &p_name)
	{
		if (_ubos.find(p_name) == _ubos.end())
		{
			GENERATE_ERROR("[Object] - Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Object.");
		}
		return _ubos.at(p_name);
	}

	spk::OpenGL::ShaderStorageBufferObject &Pipeline::Object::ssbo(const std::wstring &p_name)
	{
		if (_ssbos.find(p_name) == _ssbos.end())
		{
			GENERATE_ERROR("[Object] - Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] not found in Object.");
		}
		return _ssbos.at(p_name);
	}

	spk::OpenGL::SamplerObject &Pipeline::Object::sampler(const std::wstring &p_name)
	{
		if (_samplers.find(p_name) == _samplers.end())
		{
			GENERATE_ERROR("[Object] - Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Object.");
		}
		return _samplers.at(p_name);
	}

	spk::OpenGL::UniformObject &Pipeline::Object::uniform(const std::wstring &p_name)
	{
		if (_uniforms.find(p_name) == _uniforms.end())
		{
			GENERATE_ERROR("[Object] - Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Object.");
		}
		return _uniforms.at(p_name);
	}

	const spk::OpenGL::UniformBufferObject &Pipeline::Object::ubo(const std::wstring &p_name) const
	{
		if (_ubos.find(p_name) == _ubos.end())
		{
			GENERATE_ERROR("[Object] - Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _ubos.at(p_name);
	}

	const spk::OpenGL::ShaderStorageBufferObject &Pipeline::Object::ssbo(const std::wstring &p_name) const
	{
		if (_ssbos.find(p_name) == _ssbos.end())
		{
			GENERATE_ERROR("[Object] - Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] not found in Pipeline.");
		}
		return _ssbos.at(p_name);
	}

	const spk::OpenGL::SamplerObject &Pipeline::Object::sampler(const std::wstring &p_name) const
	{
		if (_samplers.find(p_name) == _samplers.end())
		{
			GENERATE_ERROR("[Object] - Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _samplers.at(p_name);
	}

	const spk::OpenGL::UniformObject &Pipeline::Object::uniform(const std::wstring &p_name) const
	{
		if (_uniforms.find(p_name) == _uniforms.end())
		{
			GENERATE_ERROR("[Object] - Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _uniforms.at(p_name);
	}

	void Pipeline::Object::render()
	{
		if (_owner == nullptr)
		{
			GENERATE_ERROR("[Object] - Pipeline owner is null.");
		}

		_owner->beginRender();

		_bufferSet.activate();

		for (auto &[name, ubo] : _ubos)
		{
			ubo.activate();
		}

		for (auto &[name, ssbo] : _ssbos)
		{
			ssbo.activate();
		}

		for (auto &[name, sampler] : _samplers)
		{
			sampler.activate();
		}

		for (auto &[name, uniform] : _uniforms)
		{
			uniform.activate();
		}

		_owner->render(_bufferSet.indexes().nbIndexes(), _nbInstance);

		_bufferSet.deactivate();

		for (auto &[name, ubo] : _ubos)
		{
			ubo.deactivate();
		}

		for (auto &[name, ssbo] : _ssbos)
		{
			ssbo.deactivate();
		}

		for (auto &[name, sampler] : _samplers)
		{
			sampler.deactivate();
		}

		_owner->endRender();
	}

	void Pipeline::beginRender()
	{
		_program.activate();

		for (auto &[name, ubo] : _ubos)
		{
			ubo.activate();
		}

		for (auto &[name, ssbo] : _ssbos)
		{
			ssbo.activate();
		}

		for (auto &[name, sampler] : _samplers)
		{
			sampler.activate();
		}

		for (auto &[name, uniform] : _uniforms)
		{
			uniform.activate();
		}
	}

	void Pipeline::render(size_t p_nbIndexes, size_t p_nbInstance)
	{
		_program.render(p_nbIndexes, p_nbInstance);
	}

	void Pipeline::endRender()
	{
		for (auto &[name, ubo] : _ubos)
		{
			ubo.deactivate();
		}

		for (auto &[name, ssbo] : _ssbos)
		{
			ssbo.deactivate();
		}

		for (auto &[name, sampler] : _samplers)
		{
			sampler.deactivate();
		}

		_program.deactivate();
	}

	Pipeline::Pipeline() :
		_program()
	{
	}

	Pipeline::Pipeline(const std::string &p_vertexShaderCode, const std::string &p_fragmentShaderCode) :
		_program(p_vertexShaderCode, p_fragmentShaderCode)
	{
	}

	void Pipeline::load(const std::string &p_vertexShaderCode, const std::string &p_fragmentShaderCode)
	{
		_program.load(p_vertexShaderCode, p_fragmentShaderCode);
	}

	void Pipeline::addUniformBufferObject(const std::wstring &p_name, spk::OpenGL::UniformBufferObject &&p_ubo)
	{
		if (_ubos.contains(p_name))
		{
			GENERATE_ERROR("[Pipeline] - Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] already exists in Pipeline.");
		}
		_ubos.emplace(p_name, std::move(p_ubo));
	}

	void Pipeline::addShaderStorageBufferObject(const std::wstring &p_name, spk::OpenGL::ShaderStorageBufferObject &&p_ssbo)
	{
		if (_ssbos.contains(p_name))
		{
			GENERATE_ERROR("[Pipeline] - Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] already exists in Pipeline.");
		}
		_ssbos.emplace(p_name, std::move(p_ssbo));
	}

	void Pipeline::addSamplerObject(const std::wstring &p_name, spk::OpenGL::SamplerObject &&p_sampler)
	{
		if (_samplers.contains(p_name))
		{
			GENERATE_ERROR("[Pipeline] - Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
		}
		_samplers.emplace(p_name, std::move(p_sampler));
	}

	void Pipeline::addUniformObject(const std::wstring &p_name, spk::OpenGL::UniformObject &&p_uniform)
	{
		if (_uniforms.contains(p_name))
		{
			GENERATE_ERROR("[Pipeline] - Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
		}
		_uniforms.emplace(p_name, std::move(p_uniform));
	}

	spk::OpenGL::UniformBufferObject &Pipeline::ubo(const std::wstring &p_name)
	{
		if (_ubos.find(p_name) == _ubos.end())
		{
			GENERATE_ERROR("[Pipeline] - Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _ubos.at(p_name);
	}

	spk::OpenGL::ShaderStorageBufferObject &Pipeline::ssbo(const std::wstring &p_name)
	{
		if (_ssbos.find(p_name) == _ssbos.end())
		{
			GENERATE_ERROR("[Pipeline] - Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] not found in Pipeline.");
		}
		return _ssbos.at(p_name);
	}

	spk::OpenGL::SamplerObject &Pipeline::sampler(const std::wstring &p_name)
	{
		if (_samplers.find(p_name) == _samplers.end())
		{
			GENERATE_ERROR("[Pipeline] - Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _samplers.at(p_name);
	}

	spk::OpenGL::UniformObject &Pipeline::uniform(const std::wstring &p_name)
	{
		if (_uniforms.find(p_name) == _uniforms.end())
		{
			GENERATE_ERROR("[Pipeline] - Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _uniforms.at(p_name);
	}

	const spk::OpenGL::UniformBufferObject &Pipeline::ubo(const std::wstring &p_name) const
	{
		if (_ubos.find(p_name) == _ubos.end())
		{
			GENERATE_ERROR("[Pipeline] - Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _ubos.at(p_name);
	}

	const spk::OpenGL::ShaderStorageBufferObject &Pipeline::ssbo(const std::wstring &p_name) const
	{
		if (_ssbos.find(p_name) == _ssbos.end())
		{
			GENERATE_ERROR("[Pipeline] - Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] not found in Pipeline.");
		}
		return _ssbos.at(p_name);
	}

	const spk::OpenGL::SamplerObject &Pipeline::sampler(const std::wstring &p_name) const
	{
		if (_samplers.find(p_name) == _samplers.end())
		{
			GENERATE_ERROR("[Pipeline] - Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _samplers.at(p_name);
	}

	const spk::OpenGL::UniformObject &Pipeline::uniform(const std::wstring &p_name) const
	{
		if (_uniforms.find(p_name) == _uniforms.end())
		{
			GENERATE_ERROR("[Pipeline] - Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _uniforms.at(p_name);
	}

	bool Pipeline::containsUbo(const std::wstring &p_name) const
	{
		return _ubos.contains(p_name);
	}

	bool Pipeline::containsSsbo(const std::wstring &p_name) const
	{
		return _ssbos.contains(p_name);
	}

	bool Pipeline::containsSampler(const std::wstring &p_name) const
	{
		return _samplers.contains(p_name);
	}

	bool Pipeline::containsUniform(const std::wstring &p_name) const
	{
		return _uniforms.contains(p_name);
	}

	void Pipeline::addObjectLayoutAttribute(spk::OpenGL::LayoutBufferObject::Attribute::Index p_index,
											spk::OpenGL::LayoutBufferObject::Attribute::Type p_type)
	{
		if (_defaultObject._bufferSet.layout().hasAttribute(p_index))
		{
			GENERATE_ERROR("[Object] - Attribute location " + std::to_string(p_index) + " has already been defined.");
		}
		_defaultObject._bufferSet.layout().addAttribute(p_index, p_type);
	}

	void Pipeline::addObjectUniformBufferObject(const std::wstring &p_name, spk::OpenGL::UniformBufferObject &&p_ubo)
	{
		if (_defaultObject._ubos.contains(p_name))
		{
			GENERATE_ERROR("[Object] - Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] already exists in Pipeline.");
		}
		_defaultObject._ubos.emplace(p_name, std::move(p_ubo));
	}

	void Pipeline::addObjectShaderStorageBufferObject(const std::wstring &p_name, spk::OpenGL::ShaderStorageBufferObject &&p_ssbo)
	{
		if (_defaultObject._ssbos.contains(p_name))
		{
			GENERATE_ERROR("[Object] - Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] already exists in Pipeline.");
		}
		_defaultObject._ssbos.emplace(p_name, std::move(p_ssbo));
	}

	void Pipeline::addObjectSamplerObject(const std::wstring &p_name, spk::OpenGL::SamplerObject &&p_sampler)
	{
		if (_defaultObject._samplers.contains(p_name))
		{
			GENERATE_ERROR("[Object] - Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
		}
		_defaultObject._samplers.emplace(p_name, std::move(p_sampler));
	}

	void Pipeline::addObjectUniformObject(const std::wstring &p_name, spk::OpenGL::UniformObject &&p_uniform)
	{
		if (_defaultObject._uniforms.contains(p_name))
		{
			GENERATE_ERROR("[Object] - Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
		}
		_defaultObject._uniforms.emplace(p_name, std::move(p_uniform));
	}

	Pipeline::Object Pipeline::createObject()
	{
		Object result(this);

		result._bufferSet = _defaultObject._bufferSet;
		result._ubos = _defaultObject._ubos;
		result._ssbos = _defaultObject._ssbos;
		result._samplers = _defaultObject._samplers;
		result._uniforms = _defaultObject._uniforms;

		return result;
	}
}