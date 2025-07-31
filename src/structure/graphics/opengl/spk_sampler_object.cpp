#include "structure/graphics/opengl/spk_sampler_object.hpp"

#include "structure/graphics/opengl/spk_texture_collection.hpp"

namespace spk::OpenGL
{
	SamplerObject::SamplerObject() :
		_designator(""),
		_bindingPoint(-1),
		_texture(nullptr),
		_uniformDestination(-1),
		_type(Type::Texture2D)
	{
	}

	SamplerObject::SamplerObject(const std::string &p_name, Type p_type, BindingPoint p_bindingPoint) :
		_designator(p_name),
		_bindingPoint(static_cast<GLint>(p_bindingPoint)),
		_texture(nullptr),
		_uniformDestination(-1),
		_type(p_type)
	{
	}

	SamplerObject::SamplerObject(const SamplerObject &p_other) :
		_designator(p_other._designator),
		_bindingPoint(p_other._bindingPoint),
		_texture(p_other._texture),
		_type(p_other._type),
		_uniformDestination(p_other._uniformDestination)
	{
	}

	SamplerObject &SamplerObject::operator=(const SamplerObject &p_other)
	{
		if (this != &p_other)
		{
			_designator = p_other._designator;
			_bindingPoint = p_other._bindingPoint;
			_texture = p_other._texture;
			_type = p_other._type;
			_uniformDestination = p_other._uniformDestination;
		}
		return *this;
	}

	SamplerObject::SamplerObject(SamplerObject &&p_other) noexcept :
		_designator(std::move(p_other._designator)),
		_bindingPoint(p_other._bindingPoint),
		_texture(p_other._texture),
		_type(p_other._type),
		_uniformDestination(p_other._uniformDestination)
	{
		p_other._texture = nullptr;
		p_other._bindingPoint = -1;
		p_other._uniformDestination = -1;
	}

	SamplerObject &SamplerObject::operator=(SamplerObject &&p_other) noexcept
	{
		if (this != &p_other)
		{
			_designator = std::move(p_other._designator);
			_bindingPoint = p_other._bindingPoint;
			_texture = p_other._texture;
			_type = p_other._type;
			_uniformDestination = p_other._uniformDestination;

			p_other._texture = nullptr;
			p_other._bindingPoint = -1;
			p_other._uniformDestination = -1;
		}
		return *this;
	}

	void SamplerObject::bind(const spk::SafePointer<const Texture> &p_texture)
	{
		_texture = p_texture;
	}

	spk::SafePointer<const Texture> &SamplerObject::texture()
	{
		return _texture;
	}

	const spk::SafePointer<const Texture> &SamplerObject::texture() const
	{
		return _texture;
	}

	SamplerObject::BindingPoint SamplerObject::bindingPoint() const
	{
		return _bindingPoint;
	}

	void SamplerObject::setBindingPoint(BindingPoint p_bindingPoint)
	{
		_bindingPoint = p_bindingPoint;
	}

	void SamplerObject::activate()
	{
		if (_uniformDestination == -1)
		{
			GLint prog = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
			glUseProgram(prog);

			_uniformDestination = glGetUniformLocation(prog, _designator.c_str());
			glUniform1i(_uniformDestination, _bindingPoint);
		}

		glActiveTexture(GL_TEXTURE0 + _bindingPoint);

		if (_texture == nullptr)
		{
			glBindTexture(static_cast<GLenum>(_type), 0);
			return;
		}

		auto cpuID = _texture->_id;
		if (cpuID < 0)
		{
			glBindTexture(static_cast<GLenum>(_type), 0);
			return;
		}

		spk::SafePointer<OpenGL::TextureObject> gpuTexture = TextureCollection::textureObject(_texture);

		if (gpuTexture)
		{
			if (_texture->_needUpdate)
			{
				gpuTexture->upload(_texture);

				_texture->_needUpdate = false;
			}

			else if (_texture->_needSettings)
			{
				gpuTexture->updateSettings(_texture);
				_texture->_needSettings = false;
			}

			glBindTexture(static_cast<GLenum>(_type), gpuTexture->id());
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	void SamplerObject::deactivate()
	{
		glBindTexture(static_cast<GLenum>(_type), 0);
	}
}
