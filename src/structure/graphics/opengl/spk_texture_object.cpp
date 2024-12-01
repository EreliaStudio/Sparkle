#include "structure/graphics/opengl/spk_texture_object.hpp"

#include "spk_debug_macro.hpp"

namespace spk::OpenGL
{
	void TextureObject::_upload()
	{
		if (_needUpload == true)
		{
			if (_id == 0)
			{
				glGenTextures(1, &_id);
			}

			_needSetup = true;

			_setup();

			glBindTexture(GL_TEXTURE_2D, _id);

			glTexImage2D(
				GL_TEXTURE_2D, 0, static_cast<GLint>(_format),
				static_cast<GLsizei>(_size.x), static_cast<GLsizei>(_size.y),
				0, static_cast<GLenum>(_format), GL_UNSIGNED_BYTE, _datas.data());

			glBindTexture(GL_TEXTURE_2D, 0);

			_ownTexture = true;
			_needUpload = false;
		}
	}
	
	void TextureObject::_setup()
	{
		if (_needSetup == true)
		{
			glBindTexture(GL_TEXTURE_2D, _id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(_filtering));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(_filtering));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(_wrap));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(_wrap));

			if (_mipmap == Mipmap::Enable)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

			if (_mipmap == Mipmap::Enable)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
			}

			glBindTexture(GL_TEXTURE_2D, 0);

			_needSetup = false;
		}
	}

	size_t TextureObject::_getBytesPerPixel(const Format& format) const
	{
		switch (format)
		{
		case Format::GreyLevel: return 1;
		case Format::DualChannel: return 2;
		case Format::RGB: return 3;
		case Format::RGBA: return 4;
		default: return 0; // Error or unsupported format
		}
	}

	TextureObject::TextureObject() :
		_id(0),
		_ownTexture(true),
		_size{ 0, 0 },
		_format(Format::Error),
		_filtering(Filtering::Nearest),
		_wrap(Wrap::Repeat),
		_mipmap(Mipmap::Disable)
	{
	}

	TextureObject::TextureObject(const TextureObject& other) :
		_id(0),
		_ownTexture(true),
		_needUpload(true),
		_needSetup(true),
		_datas(other._datas),
		_size(other._size),
		_format(other._format),
		_filtering(other._filtering),
		_wrap(other._wrap),
		_mipmap(other._mipmap)
	{

	}
	TextureObject& TextureObject::operator=(const TextureObject& other)
	{
		if (this != &other)
		{
			_ownTexture = true;
			_needUpload = true;
			_needSetup = true;
			_datas = other._datas;
			_size = other._size;
			_format = other._format;
			_filtering = other._filtering;
			_wrap = other._wrap;
			_mipmap = other._mipmap;

		}
		return *this;
	}


	TextureObject::~TextureObject()
	{
		if (_ownTexture == true && _id != 0)
		{
			glDeleteTextures(1, &_id);
		}
	}

	TextureObject::TextureObject(TextureObject&& p_other) noexcept :
		_id(p_other._id),
		_ownTexture(p_other._ownTexture),
		_needUpload(p_other._needUpload),
		_datas(std::move(p_other._datas)),
		_size(p_other._size),
		_format(p_other._format),
		_filtering(p_other._filtering),
		_wrap(p_other._wrap),
		_mipmap(p_other._mipmap)
	{
		p_other._id = 0;
	}

	TextureObject& TextureObject::operator=(TextureObject&& p_other) noexcept
	{
		if (this != &p_other)
		{
			if (_id != 0)
			{
				glDeleteTextures(1, &_id);
			}

			_id = p_other._id;
			_ownTexture = p_other._ownTexture;
			_needUpload = p_other._needUpload;
			_datas = std::move(p_other._datas);
			_size = p_other._size;
			_format = p_other._format;
			_filtering = p_other._filtering;
			_wrap = p_other._wrap;
			_mipmap = p_other._mipmap;

			p_other._id = 0;
		}
		return *this;
	}

	void TextureObject::setData(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_size,
		const Format& p_format, const Filtering& p_filtering,
		const Wrap& p_wrap, const Mipmap& p_mipmap)
	{
		if (_ownTexture == false)
		{
			throw std::runtime_error("Can't set data on non-owned texture.");
		}
		_datas = p_data;
		_size = p_size;
		_format = p_format;
		_filtering = p_filtering;
		_wrap = p_wrap;
		_mipmap = p_mipmap;
		_needUpload = true;
	}

	void TextureObject::setData(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_size, const Format& p_format)
	{
		if (_ownTexture == false)
		{
			throw std::runtime_error("Can't set data on non-owned texture.");
		}
		_datas = p_data;
		_size = p_size;
		_format = p_format;
		_needUpload = true;
	}

	void TextureObject::setData(const uint8_t* p_data, const spk::Vector2UInt& p_size,
		const Format& p_format, const Filtering& p_filtering,
		const Wrap& p_wrap, const Mipmap& p_mipmap)
	{			
		if (_ownTexture == false)
		{
			throw std::runtime_error("Can't set data on non-owned texture.");
		}
		
		_size = p_size;
		_format = p_format;
		_filtering = p_filtering;
		_wrap = p_wrap;
		_mipmap = p_mipmap;
		_datas.assign(p_data, p_data + (p_size.x * p_size.y * _getBytesPerPixel(p_format)));
		_needUpload = true;
	}

	void TextureObject::setData(const uint8_t* p_data, const spk::Vector2UInt& p_size, const Format& p_format)
	{
		if (_ownTexture == false)
		{
			throw std::runtime_error("Can't set data on non-owned texture.");
		}
		_size = p_size;
		_format = p_format;
		_datas.assign(p_data, p_data + (p_size.x * p_size.y * _getBytesPerPixel(p_format)));
		_needUpload = true;
	}

	void TextureObject::setProperties(const Filtering& p_filtering, const Wrap& p_wrap, const Mipmap& p_mipmap)
	{
		_filtering = p_filtering;
		_wrap = p_wrap;
		_mipmap = p_mipmap;
		_needSetup = true;
	}

	const std::vector<uint8_t>& TextureObject::data() const
	{
		return _datas;
	}

	spk::Vector2UInt TextureObject::size() const
	{
		return _size;
	}

	TextureObject::Format TextureObject::format() const
	{
		return _format;
	}

	TextureObject::Filtering TextureObject::filtering() const
	{
		return _filtering;
	}

	TextureObject::Wrap TextureObject::wrap() const
	{
		return _wrap;
	}

	TextureObject::Mipmap TextureObject::mipmap() const
	{
		return _mipmap;
	}
}
