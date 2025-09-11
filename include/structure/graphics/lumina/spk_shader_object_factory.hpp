#pragma once

#include "structure/container/spk_json_object.hpp"
#include "structure/design_pattern/spk_singleton.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"
#include "structure/graphics/opengl/spk_shader_storage_buffer_object.hpp"
#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace spk::Lumina
{
	class ShaderObjectFactory : public spk::Singleton<ShaderObjectFactory>
	{
		friend class spk::Singleton<ShaderObjectFactory>;

	private:
		using Object = std::variant<
			std::shared_ptr<OpenGL::UniformBufferObject>,
			std::shared_ptr<OpenGL::ShaderStorageBufferObject>,
			std::shared_ptr<OpenGL::SamplerObject>>;

		std::unordered_map<std::wstring, Object> _objects;

		ShaderObjectFactory() = default;

		void _addUBOs(const spk::JSON::Object &p_ubos);
		void _addSSBOs(const spk::JSON::Object &p_ssbos);
		void _addSamplers(const spk::JSON::Object &p_samplers);

	public:
		void add(const spk::JSON::Object &p_desc);

		const OpenGL::UniformBufferObject &ubo(const std::wstring &p_name);
		const OpenGL::ShaderStorageBufferObject &ssbo(const std::wstring &p_name);
		const OpenGL::SamplerObject &sampler(const std::wstring &p_name);
	};
}
