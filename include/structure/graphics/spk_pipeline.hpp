#pragma once

#include <filesystem>
#include <unordered_map>

#include "structure/graphics/opengl/spk_program.hpp"
#include "structure/graphics/opengl/spk_uniform_buffer_object.hpp"
#include "structure/graphics/opengl/spk_buffer_set.hpp"
#include "structure/graphics/opengl/spk_sampler_object.hpp"
#include "structure/graphics/opengl/spk_frame_buffer_object.hpp"
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
		class ShaderParser
		{
		private:
			struct ConstantInformation
			{
				spk::OpenGL::UniformBufferObject::Factory factory;
				int bindingPoint = -1;
				size_t size = 0;

				ConstantInformation() = default;
				ConstantInformation(const spk::OpenGL::UniformBufferObject::Factory& factory, int bindingPoint, size_t size);
			};

			const std::string LAYOUTS_DELIMITER = "## LAYOUTS DEFINITION ##";
			const std::string CONSTANTS_DELIMITER = "## CONSTANTS DEFINITION ##";
			const std::string FRAMEBUFFER_DELIMITER = "## FRAMEBUFFER DEFINITION ##";
			const std::string ATTRIBUTES_DELIMITER = "## ATTRIBUTES DEFINITION ##";
			const std::string TEXTURES_DELIMITER = "## TEXTURES DEFINITION ##";
			const std::string VERTEX_DELIMITER = "## VERTEX SHADER CODE ##";
			const std::string FRAGMENT_DELIMITER = "## FRAGMENT SHADER CODE ##";

			std::string _vertexShaderCode;
			std::string _fragmentShaderCode;

			static inline std::unordered_map<std::wstring, ConstantInformation> _constantInfoMap;
			std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject::Factory> _currentConstantFactories;
			std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject::Factory> _attributeFactories;
			std::unordered_map<std::wstring, spk::OpenGL::SamplerObject::Factory> _samplerFactories;
			spk::OpenGL::FrameBufferObject::Factory _frameBufferFactory;
			spk::OpenGL::BufferSet::Factory _objectBufferFactory;

			std::string _getFileSection(const std::string& p_inputCode, const std::string& p_delimiter);
			std::string _cleanAndCondenseWhitespace(const std::string& input);
			void _parseUniformBufferLayout(spk::OpenGL::UniformBufferObject::Layout& p_layoutToFeed, const std::vector<std::string>& p_words, size_t& p_index);
			std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject::Factory> _parseUniformDescriptors(const std::string& p_descriptors);
			void _assignBindingPointsToConstants();
			void _assignBindingPointsToAttributes();
			void _applyBindingPoints(const std::string& type, const std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject::Factory>& factories);
			spk::OpenGL::BufferSet::Factory _parseLayoutDescriptors(const std::string& p_layoutDescriptors);
			std::unordered_map<std::wstring, spk::OpenGL::SamplerObject::Factory> _parseSamplerDescriptors(const std::string& p_textureDescriptors);

			spk::OpenGL::FrameBufferObject::Factory parseFramebufferDescriptors(const std::string& descriptors);

		public:
			ShaderParser(const std::string& p_fileContent);

			static const std::unordered_map<std::wstring, ConstantInformation>& getConstantInfoMap();
			const std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject::Factory>& getCurrentConstantFactories() const;
			const std::unordered_map<std::wstring, spk::OpenGL::UniformBufferObject::Factory>& getAttributeFactories() const;
			const spk::OpenGL::FrameBufferObject::Factory& getFrameBufferFactory() const;
			const spk::OpenGL::BufferSet::Factory& getLayoutFactory() const;
			const std::unordered_map<std::wstring, spk::OpenGL::SamplerObject::Factory>& getSamplerFactories() const;
			const std::string& getVertexShaderCode() const;
			const std::string& getFragmentShaderCode() const;
		};

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
