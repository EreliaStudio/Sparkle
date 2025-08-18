#include "structure/graphics/opengl/spk_sampler_object.hpp"

#include "structure/graphics/opengl/spk_texture_collection.hpp"
#include "utils/spk_string_utils.hpp"

namespace spk::OpenGL
{
	SamplerObject::SamplerObject() :
		_texture(nullptr),
		_type(Type::Texture2D)
	{
	}

	SamplerObject::SamplerObject(const std::string &p_name, Type p_type, BindingPoint p_bindingPoint) :
		_designator(p_name),
		_bindingPoint(static_cast<GLint>(p_bindingPoint)),
		_texture(nullptr),
		_type(p_type)
	{
	}

	SamplerObject::SamplerObject(const spk::JSON::Object &p_desc) :
		SamplerObject(
			spk::StringUtils::wstringToString(p_desc[L"Name"].as<std::wstring>()),
			[&]()
			{
				std::wstring typeStr = p_desc[L"Type"].as<std::wstring>();
				if (typeStr == L"Texture1D")
				{
					return Type::Texture1D;
				}
				if (typeStr == L"Texture3D")
				{
					return Type::Texture3D;
				}
				if (typeStr == L"TextureCubeMap")
				{
					return Type::TextureCubeMap;
				}
				return Type::Texture2D;
			}(),
			static_cast<BindingPoint>(p_desc[L"BindingPoint"].as<int32_t>()))
	{
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
		return (_texture);
	}

	const spk::SafePointer<const Texture> &SamplerObject::texture() const
	{
		return (_texture);
	}

	SamplerObject::BindingPoint SamplerObject::bindingPoint() const
	{
		return (_bindingPoint);
	}

	void SamplerObject::setBindingPoint(BindingPoint p_bindingPoint)
	{
		_bindingPoint = p_bindingPoint;
	}

	SamplerObject::Type SamplerObject::type() const
	{
		return (_type);
	}
	
	void SamplerObject::setType(SamplerObject::Type p_type)
	{
		_type = p_type;
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

		if (gpuTexture != nullptr)
		{
			if (_texture->_needUpdate == true)
			{
				gpuTexture->upload(_texture);

				_texture->_needUpdate = false;
			}

			else if (_texture->_needSettings == true)
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
