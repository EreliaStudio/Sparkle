#pragma once

#include "structure/graphics/opengl/spk_uniform_object.hpp"
#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"
#include "structure/graphics/opengl/spk_shader_storage_buffer_object.hpp"

namespace spk
{
	class Pipeline
	{
	public:
		class Object
		{
		private:
			using Attribute = std::variant<spk::OpenGL::UniformBufferObject, spk::OpenGL::ShaderStorageBufferObject>;
			
			std::unordered_map<std::wstring, Attribute> _ubos;

		public:
			Object()
			{

			}

			spk::OpenGL::UniformBufferObject& ubo(const std::wstring& p_name)
			{
				
			}

			spk::OpenGL::ShaderStorageBufferObject& ssbo(const std::wstring& p_name)
			{

			}
		};

	private:

	public:
		Pipeline()
		{

		}
	};
}