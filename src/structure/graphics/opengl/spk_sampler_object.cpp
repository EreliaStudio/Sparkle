#include "structure/graphics/opengl/spk_sampler_object.hpp"

namespace spk::OpenGL
{
	void SamplerObject::Factory::setName(const std::string& name)
	{
		_name = name;
	}

	void SamplerObject::Factory::setDesignator(const std::string& designator)
	{
		_designator = designator;
	}
	
	void SamplerObject::Factory::setIndex(size_t index)
	{
		_index = index;
	}

	SamplerObject SamplerObject::Factory::construct() const
	{
		return SamplerObject(_name, _designator, _index);
	}

	SamplerObject::SamplerObject()
		: _name("unknow name"), _designator("unknow designator"), _index(-1), _texture(nullptr), _uniformDestination(-1)
	{
	}

	SamplerObject::SamplerObject(const std::string& p_name, const std::string& p_designator, size_t p_index)
		: _name(p_name), _designator(p_designator), _index(static_cast<GLint>(p_index)), _texture(nullptr), _uniformDestination(-1)
	{
	}

	SamplerObject::SamplerObject(const SamplerObject& p_other)
		: _name(p_other._name),
		_designator(p_other._designator),
		_index(p_other._index),
		_texture(p_other._texture),
		_uniformDestination(p_other._uniformDestination)
	{

	}

	SamplerObject& SamplerObject::operator=(const SamplerObject& p_other)
	{
		if (this != &p_other)
		{
			_name = p_other._name;
			_designator = p_other._designator;
			_index = p_other._index;
			_texture = p_other._texture;
			_uniformDestination = p_other._uniformDestination;
		}
		return *this;
	}

	SamplerObject::SamplerObject(SamplerObject&& p_other) noexcept
		: _name(std::move(p_other._name)), _designator(std::move(p_other._designator)), _index(p_other._index), _texture(p_other._texture), _uniformDestination(p_other._uniformDestination)
	{
		p_other._texture = nullptr;
		p_other._index = static_cast<GLint>(-1);
		p_other._uniformDestination = -1;
	}

	SamplerObject& SamplerObject::operator=(SamplerObject&& p_other) noexcept
	{
		if (this != &p_other)
		{
			_name = std::move(p_other._name);
			_designator = std::move(p_other._designator);
			_index = p_other._index;
			_texture = p_other._texture;
			_uniformDestination = p_other._uniformDestination;

			p_other._texture = nullptr;
			p_other._index = static_cast<GLint>(-1);
			p_other._uniformDestination = -1;
		}
		return *this;
	}

	void SamplerObject::bind(const spk::SafePointer<TextureObject>& p_texture)
	{
		_texture = p_texture;
	}

	void SamplerObject::activate()
	{
		if (_texture == nullptr)
		{
			throw std::runtime_error("Can't activate the SamplerObject [" + _name + "] without a linked TextureObject.");
		}

		if (_uniformDestination == -1)
		{
			GLint prog = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
			glUseProgram(prog);

			_uniformDestination = glGetUniformLocation(prog, _designator.c_str());
			glUniform1i(_uniformDestination, _index);
		}

		_texture->_upload();
		_texture->_setup();

		glActiveTexture(GL_TEXTURE0 + _index);

		glBindTexture(GL_TEXTURE_2D, _texture->_id);
	}
}
