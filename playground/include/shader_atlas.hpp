#pragma once

#include <sparkle.hpp>

class ShaderAtlas : public spk::Singleton<ShaderAtlas>
{
	friend class spk::Singleton<ShaderAtlas>;

private:
	ShaderAtlas();

	spk::Lumina::ShaderObjectFactory::Instanciator _shaderObjectFactoryInstanciator;
	std::unordered_map<std::wstring, spk::Lumina::Shader> _shaders;

	spk::Lumina::Shader createMapPainterShader();

public:
	void addShader(const std::wstring& p_name, const spk::Lumina::Shader& p_shader);

	spk::Lumina::Shader& shader(const std::wstring& p_name);
	
	static spk::OpenGL::SamplerObject &sampler(const std::wstring &p_name);
	static spk::OpenGL::UniformBufferObject &ubo(const std::wstring &p_name);
	static spk::OpenGL::ShaderStorageBufferObject &ssbo(const std::wstring &p_name);
};