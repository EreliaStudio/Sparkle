#include "structure/graphics/lumina/spk_shader_object_factory.hpp"

namespace spk::Lumina
{
	void ShaderObjectFactory::add(const spk::JSON::Object &p_desc)
	{
		_add(p_desc);
	}

	void ShaderObjectFactory::_add(const spk::JSON::Object &p_desc)
	{
		if (p_desc.contains(L"UBO") == true)
		{
			const spk::JSON::Object &ubos = p_desc[L"UBO"];
			for (size_t index = 0; index < ubos.size(); ++index)
			{
				const spk::JSON::Object &desc = ubos[index];
				std::wstring name = desc[L"name"].as<std::wstring>();
				const spk::JSON::Object &data = desc[L"data"];
				_objects[name] = spk::OpenGL::UniformBufferObject(data);
			}
		}

		if (p_desc.contains(L"SSBO") == true)
		{
			const spk::JSON::Object &ssbos = p_desc[L"SSBO"];
			for (size_t index = 0; index < ssbos.size(); ++index)
			{
				const spk::JSON::Object &desc = ssbos[index];
				std::wstring name = desc[L"name"].as<std::wstring>();
				const spk::JSON::Object &data = desc[L"data"];
				_objects[name] = spk::OpenGL::ShaderStorageBufferObject(data);
			}
		}

		if (p_desc.contains(L"Sampler") == true)
		{
			const spk::JSON::Object &samplers = p_desc[L"Sampler"];
			for (size_t index = 0; index < samplers.size(); ++index)
			{
				const spk::JSON::Object &desc = samplers[index];
				std::wstring alias = desc[L"name"].as<std::wstring>();
				const spk::JSON::Object &data = desc[L"data"];
				_objects[alias] = spk::OpenGL::SamplerObject(data);
			}
		}
	}

	spk::OpenGL::UniformBufferObject &ShaderObjectFactory::ubo(const std::wstring &p_name)
	{
		return (std::get<spk::OpenGL::UniformBufferObject>(_objects.at(p_name)));
	}

	spk::OpenGL::ShaderStorageBufferObject &ShaderObjectFactory::ssbo(const std::wstring &p_name)
	{
		return (std::get<spk::OpenGL::ShaderStorageBufferObject>(_objects.at(p_name)));
	}

	spk::OpenGL::SamplerObject &ShaderObjectFactory::sampler(const std::wstring &p_name)
	{
		return (std::get<spk::OpenGL::SamplerObject>(_objects.at(p_name)));
	}
}
