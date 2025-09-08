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
			auto newUBO = std::make_shared<OpenGL::UniformBufferObject>(data);

			if (_objects.contains(name))
			{
				auto existing = std::get<std::shared_ptr<OpenGL::UniformBufferObject>>(_objects[name]);
				if (existing->size() != newUBO->size())
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
			auto newSSBO = std::make_shared<spk::OpenGL::ShaderStorageBufferObject>(data);

			if (_objects.contains(name))
			{
				auto existing = std::get<std::shared_ptr<spk::OpenGL::ShaderStorageBufferObject>>(_objects[name]);
				if (existing->fixedData().size() != newSSBO->fixedData().size() ||
					existing->dynamicArray().elementSize() != newSSBO->dynamicArray().elementSize())
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
			auto newSampler = std::make_shared<spk::OpenGL::SamplerObject>(data);

			if (_objects.contains(alias))
			{
				auto existing = std::get<std::shared_ptr<spk::OpenGL::SamplerObject>>(_objects[alias]);
				if (existing->bindingPoint() != newSampler->bindingPoint() || existing->type() != newSampler->type())
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

	std::shared_ptr<spk::OpenGL::UniformBufferObject> ShaderObjectFactory::ubo(const std::wstring &p_name)
	{
		return (std::get<std::shared_ptr<spk::OpenGL::UniformBufferObject>>(_objects.at(p_name)));
	}

	std::shared_ptr<spk::OpenGL::ShaderStorageBufferObject> ShaderObjectFactory::ssbo(const std::wstring &p_name)
	{
		return (std::get<std::shared_ptr<spk::OpenGL::ShaderStorageBufferObject>>(_objects.at(p_name)));
	}

	std::shared_ptr<spk::OpenGL::SamplerObject> ShaderObjectFactory::sampler(const std::wstring &p_name)
	{
		return (std::get<std::shared_ptr<spk::OpenGL::SamplerObject>>(_objects.at(p_name)));
	}
}
