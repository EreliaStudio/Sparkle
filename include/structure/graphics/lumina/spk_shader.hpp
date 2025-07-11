#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "structure/graphics/opengl/spk_layout_buffer_object.hpp"

#include "structure/container/spk_json_object.hpp"

namespace spk::Lumina
{
class Shader
	{
	public:
		struct Layout
		{
			size_t bindingPoint{};
			std::wstring name;
			spk::OpenGL::LayoutBufferObject::Attribute::Type type{};
		};

		using ExceptionID = size_t;
		struct ExceptionDescriptor
		{
			std::wstring format;
			std::vector<spk::JSON::Object> parameters;
		};

	private:
		std::vector<Layout> _inputLayoutDescriptors;
		std::vector<Layout> _frameBufferLayoutDescriptors;
		std::unordered_map<ExceptionID, ExceptionDescriptor> _exceptionDescriptors;
		std::vector<spk::JSON::Object> _constantDescriptors;
		std::vector<spk::JSON::Object> _attributeDescriptors;
		std::unordered_map<std::wstring, std::wstring> _textureDescriptors;
		std::wstring _vertexShaderCode;
		std::wstring _fragmentShaderCode;

		static std::wstring _extractSection(const std::wstring &p_src, const std::wstring &p_sectionName);

		static spk::OpenGL::LayoutBufferObject::Attribute::Type _toAttrType(const std::wstring &p_input);

		static std::vector<Layout> _parseLayoutLines(const std::wstring &p_section);

		void _parseExceptionSection(const std::wstring &p_section);

		static std::vector<spk::JSON::Object> _splitIntoJsonObjects(const std::wstring &p_section);

		void _parseTextureSection(const std::wstring &p_section);

		explicit Shader(const std::wstring &p_source);

	public:
		Shader() = default;

		static Shader fromSource(const std::wstring &p_src);
		static Shader fromFile(const std::wstring &p_path);
		
		friend std::wostream& operator << (std::wostream& p_os, const Shader& p_shader);
		friend std::ostream& operator << (std::ostream& p_os, const Shader& p_shader);
	};
}