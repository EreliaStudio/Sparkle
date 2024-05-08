#include "graphics/pipeline/spk_pipeline.hpp"
#include "graphics/texture/spk_sprite_sheet.hpp"

namespace spk
{
	Pipeline::Texture::Texture(int p_programID, const Pipeline::Texture::Layout& p_layout) :
		_textureBindingPoint(p_layout.textureBindingPoint),
		_handle(glGetUniformLocation(p_programID, p_layout.name.c_str())),
		_activeTexture(nullptr)
	{
		_handle = _textureBindingPoint;
	}

	void Pipeline::Texture::attach(const spk::Texture* p_textureToSet)
	{
		if (p_textureToSet == _activeTexture)
			return ;

		_activeTexture = p_textureToSet;
	}

    void Pipeline::Texture::_activate()
	{
		if (_activeTexture == nullptr || _lastActiveTexture == _activeTexture)
			return ;
		
		_lastActiveTexture = _activeTexture;
		_activeTexture->_bind(_textureBindingPoint);
		_handle.activate();
	}

    std::map<std::string, Pipeline::Texture::Layout> Pipeline::_parseSamplerUniforms(const std::vector<OpenGL::ShaderInstruction>& p_shaderinstruction)
	{
		std::map<std::string, Pipeline::Texture::Layout> result;

		for (const auto& instruction : p_shaderinstruction)
		{
			if (instruction.type == OpenGL::ShaderInstruction::Type::Sampler)
			{
				Pipeline::Texture::Layout newLayout;

				newLayout.name = instruction.informations[OpenGL::ShaderInstruction::NameKey].as<std::string>();
				newLayout.textureBindingPoint = static_cast<int>(result.size());

				result.emplace(newLayout.name, newLayout);
			}
		}

		return (result);
	}
}