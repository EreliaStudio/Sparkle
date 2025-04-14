#include "structure/graphics/spk_pipeline.hpp"
#include "utils/spk_string_utils.hpp"

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
			throw std::runtime_error("[Object] - Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Object.");
		}
		return _ubos.at(p_name);
	}

	spk::OpenGL::ShaderStorageBufferObject &Pipeline::Object::ssbo(const std::wstring &p_name)
	{
		if (_ssbos.find(p_name) == _ssbos.end())
		{
			throw std::runtime_error("[Object] - Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] not found in Object.");
		}
		return _ssbos.at(p_name);
	}

	spk::OpenGL::SamplerObject &Pipeline::Object::sampler(const std::wstring &p_name)
	{
		if (_samplers.find(p_name) == _samplers.end())
		{
			throw std::runtime_error("[Object] - Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Object.");
		}
		return _samplers.at(p_name);
	}

	spk::OpenGL::UniformObject &Pipeline::Object::uniform(const std::wstring &p_name)
	{
		if (_uniforms.find(p_name) == _uniforms.end())
		{
			throw std::runtime_error("[Object] - Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Object.");
		}
		return _uniforms.at(p_name);
	}

	const spk::OpenGL::UniformBufferObject &Pipeline::Object::ubo(const std::wstring &p_name) const
	{
		if (_ubos.find(p_name) == _ubos.end())
		{
			throw std::runtime_error("[Object] - Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _ubos.at(p_name);
	}

	const spk::OpenGL::ShaderStorageBufferObject &Pipeline::Object::ssbo(const std::wstring &p_name) const
	{
		if (_ssbos.find(p_name) == _ssbos.end())
		{
			throw std::runtime_error("[Object] - Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] not found in Pipeline.");
		}
		return _ssbos.at(p_name);
	}

	const spk::OpenGL::SamplerObject &Pipeline::Object::sampler(const std::wstring &p_name) const
	{
		if (_samplers.find(p_name) == _samplers.end())
		{
			throw std::runtime_error("[Object] - Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _samplers.at(p_name);
	}

	const spk::OpenGL::UniformObject &Pipeline::Object::uniform(const std::wstring &p_name) const
	{
		if (_uniforms.find(p_name) == _uniforms.end())
		{
			throw std::runtime_error("[Object] - Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _uniforms.at(p_name);
	}

	void Pipeline::Object::render()
	{
		if (_owner == nullptr)
		{
			throw std::runtime_error("[Object] - Pipeline owner is null.");
		}

		std::lock_guard<std::mutex> lock(_owner->_mutex);

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
		if (p_nbIndexes == 0)
		{
			return;
		}

		if (p_nbInstance > 0)
		{
			glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>(p_nbIndexes), static_cast<GLsizei>(p_nbInstance));
		}
		else
		{
			glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(p_nbIndexes));
		}
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
			throw std::runtime_error("[Pipeline] - Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] already exists in Pipeline.");
		}
		_ubos.at(p_name) = p_ubo;
	}

	void Pipeline::addShaderStorageBufferObject(const std::wstring &p_name, spk::OpenGL::ShaderStorageBufferObject &&p_ssbo)
	{
		if (_ssbos.contains(p_name))
		{
			throw std::runtime_error("[Pipeline] - Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] already exists in Pipeline.");
		}
		_ssbos.at(p_name) = p_ssbo;
	}

	void Pipeline::addSamplerObject(const std::wstring &p_name, spk::OpenGL::SamplerObject &&p_sampler)
	{
		if (_samplers.contains(p_name))
		{
			throw std::runtime_error("[Pipeline] - Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
		}
		_samplers.at(p_name) = p_sampler;
	}

	void Pipeline::addUniformObject(const std::wstring &p_name, spk::OpenGL::UniformObject &&p_uniform)
	{
		if (_uniforms.contains(p_name))
		{
			throw std::runtime_error("[Pipeline] - Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
		}
		_uniforms.at(p_name) = p_uniform;
	}

	spk::OpenGL::UniformBufferObject &Pipeline::ubo(const std::wstring &p_name)
	{
		if (_ubos.find(p_name) == _ubos.end())
		{
			throw std::runtime_error("[Pipeline] - Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _ubos.at(p_name);
	}

	spk::OpenGL::ShaderStorageBufferObject &Pipeline::ssbo(const std::wstring &p_name)
	{
		if (_ssbos.find(p_name) == _ssbos.end())
		{
			throw std::runtime_error("[Pipeline] - Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] not found in Pipeline.");
		}
		return _ssbos.at(p_name);
	}

	spk::OpenGL::SamplerObject &Pipeline::sampler(const std::wstring &p_name)
	{
		if (_samplers.find(p_name) == _samplers.end())
		{
			throw std::runtime_error("[Pipeline] - Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _samplers.at(p_name);
	}

	spk::OpenGL::UniformObject &Pipeline::uniform(const std::wstring &p_name)
	{
		if (_uniforms.find(p_name) == _uniforms.end())
		{
			throw std::runtime_error("[Pipeline] - Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _uniforms.at(p_name);
	}

	const spk::OpenGL::UniformBufferObject &Pipeline::ubo(const std::wstring &p_name) const
	{
		if (_ubos.find(p_name) == _ubos.end())
		{
			throw std::runtime_error("[Pipeline] - Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _ubos.at(p_name);
	}

	const spk::OpenGL::ShaderStorageBufferObject &Pipeline::ssbo(const std::wstring &p_name) const
	{
		if (_ssbos.find(p_name) == _ssbos.end())
		{
			throw std::runtime_error("[Pipeline] - Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] not found in Pipeline.");
		}
		return _ssbos.at(p_name);
	}

	const spk::OpenGL::SamplerObject &Pipeline::sampler(const std::wstring &p_name) const
	{
		if (_samplers.find(p_name) == _samplers.end())
		{
			throw std::runtime_error("[Pipeline] - Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
		}
		return _samplers.at(p_name);
	}

	const spk::OpenGL::UniformObject &Pipeline::uniform(const std::wstring &p_name) const
	{
		if (_uniforms.find(p_name) == _uniforms.end())
		{
			throw std::runtime_error("[Pipeline] - Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] not found in Pipeline.");
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
			throw std::runtime_error("[Object] - Attribute location " + std::to_string(p_index) + " has already been defined.");
		}
		_defaultObject._bufferSet.layout().addAttribute(p_index, p_type);
	}

	void Pipeline::addObjectUniformBufferObject(const std::wstring &p_name, spk::OpenGL::UniformBufferObject &&p_ubo)
	{
		if (_defaultObject._ubos.contains(p_name))
		{
			throw std::runtime_error("[Object] - Uniform Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] already exists in Pipeline.");
		}
		_defaultObject._ubos.at(p_name) = p_ubo;
	}

	void Pipeline::addObjectShaderStorageBufferObject(const std::wstring &p_name, spk::OpenGL::ShaderStorageBufferObject &&p_ssbo)
	{
		if (_defaultObject._ssbos.contains(p_name))
		{
			throw std::runtime_error("[Object] - Shader Storage Buffer Object [" + spk::StringUtils::wstringToString(p_name) +
									 "] already exists in Pipeline.");
		}
		_defaultObject._ssbos.at(p_name) = p_ssbo;
	}

	void Pipeline::addObjectSamplerObject(const std::wstring &p_name, spk::OpenGL::SamplerObject &&p_sampler)
	{
		if (_defaultObject._samplers.contains(p_name))
		{
			throw std::runtime_error("[Object] - Sampler Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
		}
		_defaultObject._samplers.at(p_name) = p_sampler;
	}

	void Pipeline::addObjectUniformObject(const std::wstring &p_name, spk::OpenGL::UniformObject &&p_uniform)
	{
		if (_defaultObject._uniforms.contains(p_name))
		{
			throw std::runtime_error("[Object] - Uniform Object [" + spk::StringUtils::wstringToString(p_name) + "] already exists in Pipeline.");
		}
		_defaultObject._uniforms.at(p_name) = p_uniform;
	}

	Pipeline::Object Pipeline::createObject()
	{
		std::lock_guard<std::mutex> lock(_mutex);

		Object result(this);

		result._bufferSet = _defaultObject._bufferSet;
		result._ubos = _defaultObject._ubos;
		result._ssbos = _defaultObject._ssbos;
		result._samplers = _defaultObject._samplers;
		result._uniforms = _defaultObject._uniforms;

		return result;
	}
}