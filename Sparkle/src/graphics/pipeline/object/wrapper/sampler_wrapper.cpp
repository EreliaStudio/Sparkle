#include "graphics/pipeline/spk_pipeline.hpp"
#include "graphics/texture/spk_sprite_sheet.hpp"

namespace spk
{
	Pipeline::Texture::Texture(int p_programID, const Pipeline::Texture::Layout& p_layout, Pipeline::Constant& p_constant) :
		_textureBindingPoint(p_layout.textureBindingPoint),
		_handle(glGetUniformLocation(p_programID, p_layout.name.c_str())),
		_constant(p_constant),
        _unitElement(_constant["unit"]),
		_activeTexture(nullptr)
	{
		_handle = _textureBindingPoint;
	}

	void Pipeline::Texture::attach(const spk::Texture* p_textureToSet)
	{
		_activeTexture = p_textureToSet;
		const spk::SpriteSheet* spriteSheet = dynamic_cast<const spk::SpriteSheet*>(p_textureToSet);
        if (spriteSheet != nullptr)
        {
            _unitElement = spriteSheet->unit();
        }
        else
        {
            _unitElement = spk::Vector2(1, 1);
        }
		_constant.update();
	}

    void Pipeline::Texture::activate()
	{
		if (_activeTexture == nullptr)
			return ;
			
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
				newLayout.textureBindingPoint = result.size();

				result.emplace(newLayout.name, newLayout);
			}
		}

		return (result);
	}
}