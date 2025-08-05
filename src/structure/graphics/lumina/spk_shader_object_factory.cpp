#include "structure/graphics/lumina/spk_shader_object_factory.hpp"

namespace spk::Lumina
{
	void ShaderObjectFactory::add(const spk::JSON::Object &p_desc)
	{
		DEBUG_LINE();
		if (p_desc.contains(L"UBO") == true)
		{
			const spk::JSON::Object &ubos = p_desc[L"UBO"];
			for (size_t index = 0; index < ubos.size(); ++index)
			{
				const spk::JSON::Object &desc = ubos[index];
				std::wstring name = desc[L"name"].as<std::wstring>();
				const spk::JSON::Object &data = desc[L"data"];
				spk::OpenGL::UniformBufferObject newUBO = spk::OpenGL::UniformBufferObject(data);

				if (_objects.contains(name) == true)
				{
					spk::OpenGL::UniformBufferObject& existingUBO = std::get<spk::OpenGL::UniformBufferObject>(_objects[name]);
					if (existingUBO.size() != newUBO.size())
					{
						GENERATE_ERROR("Uniform Buffer Object '" + spk::StringUtils::wstringToString(name) + "' already exists with a different size.");
					}
				}
				else
				{
					_objects[name] = std::move(newUBO);
				}
			}
		}
		DEBUG_LINE();

		if (p_desc.contains(L"SSBO") == true)
		{
			const spk::JSON::Object &ssbos = p_desc[L"SSBO"];
			for (size_t index = 0; index < ssbos.size(); ++index)
			{
				const spk::JSON::Object &desc = ssbos[index];
				std::wstring name = desc[L"name"].as<std::wstring>();
				const spk::JSON::Object &data = desc[L"data"];

				spk::OpenGL::ShaderStorageBufferObject newSSBO = spk::OpenGL::ShaderStorageBufferObject(data);

				if (_objects.contains(name) == true)
				{
					spk::OpenGL::ShaderStorageBufferObject& existingSSBO = std::get<spk::OpenGL::ShaderStorageBufferObject>(_objects[name]);
					if (existingSSBO.fixedData().size() != newSSBO.fixedData().size() ||
						existingSSBO.dynamicArray().elementSize() != newSSBO.dynamicArray().elementSize())
					{
						GENERATE_ERROR("Shader Storage Buffer Object '" + spk::StringUtils::wstringToString(name) + "' already exists with a different size.");
					}
				}
				else
				{
					_objects[name] = std::move(newSSBO);
				}
			}
		}

		DEBUG_LINE();
		if (p_desc.contains(L"Sampler") == true)
		{
			const spk::JSON::Object &samplers = p_desc[L"Sampler"];
			for (size_t index = 0; index < samplers.size(); ++index)
			{
				const spk::JSON::Object &desc = samplers[index];
				std::wstring alias = desc[L"name"].as<std::wstring>();
				const spk::JSON::Object &data = desc[L"data"];

				spk::OpenGL::SamplerObject newSampler = spk::OpenGL::SamplerObject(data);

				if (_objects.contains(alias) == true)
				{
					spk::OpenGL::SamplerObject& existingSampler = std::get<spk::OpenGL::SamplerObject>(_objects[alias]);
					if (existingSampler.bindingPoint() != newSampler.bindingPoint() ||
						existingSampler.type() != newSampler.type())
					{
						GENERATE_ERROR("Shader Storage Buffer Object '" + spk::StringUtils::wstringToString(alias) + "' already exists with a different binding point or type.");
					}
				}
				else
				{
					_objects[alias] = std::move(newSampler);
				}
			}
		}
		DEBUG_LINE();
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
