#pragma once

#include <filesystem>
#include <unordered_map>

#include "structure/graphics/opengl/spk_shader_parser.hpp"
#include "structure/graphics/opengl/spk_program.hpp"
#include "utils/spk_file_utils.hpp"

namespace spk
{
	class Pipeline
	{
	public:
		using Constant = spk::OpenGL::UniformBufferObject;

		class Object
		{
			friend class Pipeline;

		public:
			using Attribute = spk::OpenGL::UniformBufferObject;
			using Sampler2D = spk::OpenGL::SamplerObject;
			using FrameBuffer = spk::OpenGL::FrameBufferObject;

		private:
			Pipeline* _owner;
			OpenGL::BufferSet _bufferSet;
			FrameBuffer _frameBuffer;
			std::unordered_map<std::wstring, Attribute> _attributes;
			std::unordered_map<std::wstring, Sampler2D> _samplers;

			void _activate();
			void _deactivate();

		public:
			OpenGL::LayoutBufferObject& layout();
			OpenGL::IndexBufferObject& indexes();
			Attribute& attribute(const std::wstring& p_name);
			Sampler2D& sampler2D(const std::wstring& p_name);
			FrameBuffer& frameBuffer();
			void render(size_t p_nbInstance = 1);
		};

	private:
		ShaderParser _parser;
		spk::OpenGL::Program _program;
		static inline std::unordered_map<std::wstring, Constant> _constants;
		std::vector<Constant*> _activeConstants;

		void _render(Object& p_object, size_t p_nbInstance);

	public:
		Pipeline(const std::string& p_shaderContent);
		static Pipeline fromFile(const std::filesystem::path& p_path);

		Object createObject();

		static Constant& constants(const std::wstring& p_name);
	};
}
