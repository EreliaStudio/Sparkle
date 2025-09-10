#include "structure/graphics/lumina/spk_shader_object_factory.hpp"

namespace spk::Lumina
{
	void ShaderObjectFactory::add(const spk::JSON::Object &p_desc)
	{
		if (p_desc.contains(L"UBO"))
		{
			_addUBOs(p_desc[L"UBO"]);
		}

		if (p_desc.contains(L"SSBO"))
		{
			_addSSBOs(p_desc[L"SSBO"]);
		}

		if (p_desc.contains(L"Sampler"))
		{
			_addSamplers(p_desc[L"Sampler"]);
		}
	}

	void ShaderObjectFactory::_addUBOs(const spk::JSON::Object &p_ubos)
	{
		for (size_t index = 0; index < p_ubos.size(); ++index)
		{
			const JSON::Object &desc = p_ubos[index];
			std::wstring name = desc[L"name"].as<std::wstring>();
			const JSON::Object &data = desc[L"data"];
			OpenGL::UniformBufferObject newUBO(data);

			if (_objects.contains(name))
			{
				auto &existing = std::get<OpenGL::UniformBufferObject>(_objects[name]);
				if (existing.size() != newUBO.size())
				{
					GENERATE_ERROR("UBO '" + StringUtils::wstringToString(name) + "' already exists with a different size.");
				}
			}
			else
			{
				_objects[name] = newUBO;
			}
		}
	}

	void ShaderObjectFactory::_addSSBOs(const spk::JSON::Object &p_ssbos)
	{
		for (size_t index = 0; index < p_ssbos.size(); ++index)
		{
			const spk::JSON::Object &desc = p_ssbos[index];
			std::wstring name = desc[L"name"].as<std::wstring>();
			const spk::JSON::Object &data = desc[L"data"];
			OpenGL::ShaderStorageBufferObject newSSBO(data);

			if (_objects.contains(name))
			{
				auto &existing = std::get<spk::OpenGL::ShaderStorageBufferObject>(_objects[name]);
				if (existing.fixedData().size() != newSSBO.fixedData().size() ||
					existing.dynamicArray().elementSize() != newSSBO.dynamicArray().elementSize())
				{
					GENERATE_ERROR("SSBO '" + spk::StringUtils::wstringToString(name) + "' already exists with a different size.");
				}
			}
			else
			{
				_objects[name] = newSSBO;
			}
		}
	}

	void ShaderObjectFactory::_addSamplers(const spk::JSON::Object &p_samplers)
	{
		for (size_t index = 0; index < p_samplers.size(); ++index)
		{
			const spk::JSON::Object &desc = p_samplers[index];
			std::wstring alias = desc[L"name"].as<std::wstring>();
			const spk::JSON::Object &data = desc[L"data"];
			OpenGL::SamplerObject newSampler(data);

			if (_objects.contains(alias))
			{
				auto &existing = std::get<spk::OpenGL::SamplerObject>(_objects[alias]);
				if (existing.bindingPoint() != newSampler.bindingPoint() || existing.type() != newSampler.type())
				{
					GENERATE_ERROR(
						"Sampler '" + spk::StringUtils::wstringToString(alias) + "' already exists with a different binding point or type.");
				}
			}
			else
			{
				_objects[alias] = newSampler;
			}
		}
	}

	const OpenGL::UniformBufferObject &ShaderObjectFactory::ubo(const std::wstring &p_name)
	{
		return std::get<spk::OpenGL::UniformBufferObject>(_objects.at(p_name));
	}

	const OpenGL::ShaderStorageBufferObject &ShaderObjectFactory::ssbo(const std::wstring &p_name)
	{
		return std::get<spk::OpenGL::ShaderStorageBufferObject>(_objects.at(p_name));
	}

	const OpenGL::SamplerObject &ShaderObjectFactory::sampler(const std::wstring &p_name)
	{
		return std::get<spk::OpenGL::SamplerObject>(_objects.at(p_name));
	}
}
