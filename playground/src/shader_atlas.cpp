#include "shader_atlas.hpp"

ShaderAtlas::ShaderAtlas()
{
	_shaderObjectFactoryInstanciator->add(spk::JSON::File("playground/resources/shaders/shader_object_descriptors.json").root());		

	addShader(L"MapPainterShader", createMapPainterShader());
}

spk::Lumina::Shader ShaderAtlas::createMapPainterShader()
{
	spk::Lumina::Shader result = spk::Lumina::Shader(
			spk::FileUtils::readFileAsString("playground/resources/shaders/map_painter.vert"),
			spk::FileUtils::readFileAsString("playground/resources/shaders/map_painter.frag")
		);

	result.addAttribute(0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2);

	result.addUBO(L"CameraUBO", _shaderObjectFactoryInstanciator->ubo(L"CameraUBO"), spk::Lumina::Shader::Mode::Constant);
	result.addUBO(L"TimeUBO", _shaderObjectFactoryInstanciator->ubo(L"TimeUBO"), spk::Lumina::Shader::Mode::Constant);
	result.addUBO(L"ChunkDataUBO", _shaderObjectFactoryInstanciator->ubo(L"ChunkDataUBO"), spk::Lumina::Shader::Mode::Attribute);
	result.addSSBO(L"TilesetSSBO", _shaderObjectFactoryInstanciator->ssbo(L"TilesetSSBO"), spk::Lumina::Shader::Mode::Constant);

	return (result);
}

void ShaderAtlas::addShader(const std::wstring& p_name, const spk::Lumina::Shader& p_shader)
{
	if (_shaders.contains(p_name) == true)
	{
		GENERATE_ERROR("Shader [" + spk::StringUtils::wstringToString(p_name) + "] already created");
	}

	_shaders[p_name] = p_shader;
}

spk::Lumina::Shader& ShaderAtlas::shader(const std::wstring& p_name)
{
	if (_shaders.contains(p_name) == false)
	{
		GENERATE_ERROR("Shader [" + spk::StringUtils::wstringToString(p_name) + "] not loaded.");
	}

	return (_shaders.at(p_name));
}

spk::OpenGL::SamplerObject &ShaderAtlas::sampler(const std::wstring &p_name)
{
	return spk::Lumina::Shader::Constants::sampler(p_name);
}

spk::OpenGL::UniformBufferObject &ShaderAtlas::ubo(const std::wstring &p_name)
{
	return spk::Lumina::Shader::Constants::ubo(p_name);
}

spk::OpenGL::ShaderStorageBufferObject &ShaderAtlas::ssbo(const std::wstring &p_name)
{
	return spk::Lumina::Shader::Constants::ssbo(p_name);
}