#include "structure/graphics/opengl/spk_sampler_object.hpp"

namespace spk::OpenGL
{
	SamplerObject::SamplerObject() : _designator(""), _index(-1), _texture(nullptr), _uniformDestination(-1) {}

	SamplerObject::SamplerObject(const SamplerObject& p_other)
		: _designator(p_other._designator),
		  _index(p_other._index),
		  _texture(p_other._texture),
		  _uniformDestination(p_other._uniformDestination) {}

	SamplerObject& SamplerObject::operator=(const SamplerObject& p_other)
	{
		if (this != &p_other)
		{
			_designator = p_other._designator;
			_index = p_other._index;
			_texture = p_other._texture;
			_uniformDestination = p_other._uniformDestination;
		}
		return *this;
	}

	SamplerObject::SamplerObject(SamplerObject&& p_other) noexcept
		: _designator(std::move(p_other._designator)),
		  _index(p_other._index),
		  _texture(p_other._texture),
		  _uniformDestination(p_other._uniformDestination)
	{
		p_other._texture = nullptr;
		p_other._index = -1;
		p_other._uniformDestination = -1;
	}

	SamplerObject& SamplerObject::operator=(SamplerObject&& p_other) noexcept
	{
		if (this != &p_other)
		{
			_designator = std::move(p_other._designator);
			_index = p_other._index;
			_texture = p_other._texture;
			_uniformDestination = p_other._uniformDestination;

			p_other._texture = nullptr;
			p_other._index = -1;
			p_other._uniformDestination = -1;
		}
		return *this;
	}

	void SamplerObject::setDesignator(const std::string& designator)
	{
		_designator = designator;
		_uniformDestination = -1;
	}

	void SamplerObject::setIndex(size_t index)
	{
		_index = static_cast<GLint>(index);
	}

	void SamplerObject::bind(const spk::SafePointer<TextureObject>& p_texture)
	{
		_texture = p_texture;
	}

	void SamplerObject::activate()
	{
		if (_uniformDestination == -1)
		{
			GLint prog = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
			glUseProgram(prog);

			_uniformDestination = glGetUniformLocation(prog, _designator.c_str());
			glUniform1i(_uniformDestination, _index);
		}

		glActiveTexture(GL_TEXTURE0 + _index);

		if (_texture == nullptr)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			return;
		}

		_texture->_upload();
		_texture->_setup();
		glBindTexture(GL_TEXTURE_2D, _texture->_id);
	}
}
