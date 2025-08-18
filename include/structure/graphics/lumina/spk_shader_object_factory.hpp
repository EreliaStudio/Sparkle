#pragma once

#include "structure/container/spk_json_object.hpp"
#include "structure/design_pattern/spk_singleton.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"
#include "structure/graphics/opengl/spk_shader_storage_buffer_object.hpp"
#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"

#include <string>
#include <unordered_map>
#include <variant>

namespace spk::Lumina
{
	class ShaderObjectFactory : public spk::Singleton<ShaderObjectFactory>
	{
	private:
		friend class spk::Singleton<ShaderObjectFactory>;

		using Object = std::variant<OpenGL::UniformBufferObject,
									OpenGL::ShaderStorageBufferObject,
									OpenGL::SamplerObject>;

		std::unordered_map<std::wstring, Object> _objects;

		ShaderObjectFactory() = default;

		void _addUbos(const spk::JSON::Object& ubos);
		void _addSsbos(const spk::JSON::Object& ssbos);
		void _addSamplers(const spk::JSON::Object& samplers);

		template <typename TType>
		bool _holds(const std::wstring& name) const
		{
			auto it = _objects.find(name);
			if (it == _objects.end())
			{
				return false;
			}
			return std::holds_alternative<TType>(it->second);
		}

	public:
		void add(const spk::JSON::Object &p_desc);

		OpenGL::UniformBufferObject &ubo(const std::wstring &p_name);
		OpenGL::ShaderStorageBufferObject &ssbo(const std::wstring &p_name);
		OpenGL::SamplerObject &sampler(const std::wstring &p_name);
	};
}
