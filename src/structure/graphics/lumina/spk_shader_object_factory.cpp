#include "structure/graphics/lumina/spk_shader_object_factory.hpp"
#include "utils/spk_string_utils.hpp"

namespace spk::Lumina
{
	void ShaderObjectFactory::_addUbos(const spk::JSON::Object& ubos)
	{
		for (size_t i = 0; i < ubos.size(); ++i)
		{
			const spk::JSON::Object &desc = ubos[i];
			const std::wstring name = desc[L"name"].as<std::wstring>();
			const spk::JSON::Object &data = desc[L"data"];

			spk::OpenGL::UniformBufferObject newUBO(data);

			auto it = _objects.find(name);
			if (it != _objects.end())
			{
				if (!_holds<spk::OpenGL::UniformBufferObject>(name))
				{
					GENERATE_ERROR(
						"Name conflict: '" + spk::StringUtils::wstringToString(name) +
						"' already exists with a different object type."
					);
				}

				auto &existing = std::get<spk::OpenGL::UniformBufferObject>(it->second);
				if (existing.size() != newUBO.size())
				{
					GENERATE_ERROR(
						"Uniform Buffer Object '" + spk::StringUtils::wstringToString(name) +
						"' already exists with a different size."
					);
				}
			}
			else
			{
				_objects.emplace(name, std::move(newUBO));
			}
		}
	}

	void ShaderObjectFactory::_addSsbos(const spk::JSON::Object& ssbos)
	{
		for (size_t i = 0; i < ssbos.size(); ++i)
		{
			const spk::JSON::Object &desc = ssbos[i];
			const std::wstring name = desc[L"name"].as<std::wstring>();
			const spk::JSON::Object &data = desc[L"data"];

			spk::OpenGL::ShaderStorageBufferObject newSSBO(data);

			auto it = _objects.find(name);
			if (it != _objects.end())
			{
				if (!_holds<spk::OpenGL::ShaderStorageBufferObject>(name))
				{
					GENERATE_ERROR(
						"Name conflict: '" + spk::StringUtils::wstringToString(name) +
						"' already exists with a different object type."
					);
				}

				auto &existing = std::get<spk::OpenGL::ShaderStorageBufferObject>(it->second);
				const bool sameFixed = (existing.fixedData().size() == newSSBO.fixedData().size());
				const bool sameDynElt = (existing.dynamicArray().elementSize() == newSSBO.dynamicArray().elementSize());

				if (!sameFixed || !sameDynElt)
				{
					GENERATE_ERROR(
						"Shader Storage Buffer Object '" + spk::StringUtils::wstringToString(name) +
						"' already exists with a different layout."
					);
				}
			}
			else
			{
				_objects.emplace(name, std::move(newSSBO));
			}
		}
	}

	void ShaderObjectFactory::_addSamplers(const spk::JSON::Object& samplers)
	{
		for (size_t i = 0; i < samplers.size(); ++i)
		{
			const spk::JSON::Object &desc = samplers[i];
			const std::wstring name = desc[L"name"].as<std::wstring>();
			const spk::JSON::Object &data = desc[L"data"];

			spk::OpenGL::SamplerObject newSampler(data);

			auto it = _objects.find(name);
			if (it != _objects.end())
			{
				if (!_holds<spk::OpenGL::SamplerObject>(name))
				{
					GENERATE_ERROR(
						"Name conflict: '" + spk::StringUtils::wstringToString(name) +
						"' already exists with a different object type."
					);
				}

				auto &existing = std::get<spk::OpenGL::SamplerObject>(it->second);
				const bool sameBinding = (existing.bindingPoint() == newSampler.bindingPoint());
				const bool sameType    = (existing.type() == newSampler.type());

				if (!sameBinding || !sameType)
				{
					GENERATE_ERROR(
						"Sampler '" + spk::StringUtils::wstringToString(name) +
						"' already exists with a different binding point or type."
					);
				}
			}
			else
			{
				_objects.emplace(name, std::move(newSampler));
			}
		}
	}
	void ShaderObjectFactory::add(const spk::JSON::Object &p_desc)
	{
		if (p_desc.contains(L"UBO"))
		{
			_addUbos(p_desc[L"UBO"]);
		}
		if (p_desc.contains(L"SSBO"))
		{
			_addSsbos(p_desc[L"SSBO"]);
		}
		if (p_desc.contains(L"Sampler"))
		{
			_addSamplers(p_desc[L"Sampler"]);
		}
	}

	spk::OpenGL::UniformBufferObject &ShaderObjectFactory::ubo(const std::wstring &p_name)
	{
		return std::get<spk::OpenGL::UniformBufferObject>(_objects.at(p_name));
	}

	spk::OpenGL::ShaderStorageBufferObject &ShaderObjectFactory::ssbo(const std::wstring &p_name)
	{
		return std::get<spk::OpenGL::ShaderStorageBufferObject>(_objects.at(p_name));
	}

	spk::OpenGL::SamplerObject &ShaderObjectFactory::sampler(const std::wstring &p_name)
	{
		return std::get<spk::OpenGL::SamplerObject>(_objects.at(p_name));
	}

}
