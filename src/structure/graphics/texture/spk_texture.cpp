#include "structure/graphics/texture/spk_texture.hpp"

namespace spk
{

	Texture::ID Texture::takeID()
	{
		Texture::ID result;
		
		if (_availableIDs.empty() == false)
		{
			result = _availableIDs.back();

			_availableIDs.pop_back();
		}
		else
		{
			result = _nextID;
			_nextID++;
		}

		return (result);
	}
	
	void Texture::releaseID(const Texture::ID& p_id)
	{
		if (p_id == InvalidID)
			return;
		
    	_availableIDs.push_back(p_id);
	}

	Texture::Section::Section() :
		anchor(0, 0),
		size(0, 0)
	{

	}

	Texture::Section::Section(spk::Vector2 p_anchor, spk::Vector2 p_size) :
		anchor(p_anchor),
		size(p_size)
	{

	}

	size_t Texture::_getBytesPerPixel(const Format& format) const
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

	Texture::Texture() :
		_id(takeID()),
		_size{ 0, 0 },
		_format(Format::Error),
		_filtering(Filtering::Nearest),
		_wrap(Wrap::ClampToEdge),
		_mipmap(Mipmap::Enable)
	{
	}

	Texture::~Texture()
	{
		releaseID(_id);
	}

	Texture::Texture(Texture&& p_other) noexcept :
		_id(p_other._id),
		_datas(std::move(p_other._datas)),
		_size(p_other._size),
		_format(p_other._format),
		_filtering(p_other._filtering),
		_wrap(p_other._wrap),
		_mipmap(p_other._mipmap)
	{
		p_other._id = InvalidID;
	}

	Texture& Texture::operator=(Texture&& p_other) noexcept
	{
		if (this != &p_other)
		{
			_id = p_other._id;
			_datas = std::move(p_other._datas);
			_size = p_other._size;
			_format = p_other._format;
			_filtering = p_other._filtering;
			_wrap = p_other._wrap;
			_mipmap = p_other._mipmap;

			p_other._id = InvalidID;
		}
		return *this;
	}

	void Texture::setData(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_size,
		const Format& p_format, const Filtering& p_filtering,
		const Wrap& p_wrap, const Mipmap& p_mipmap)
	{
		_datas = p_data;
		_size = p_size;
		_format = p_format;
		_filtering = p_filtering;
		_wrap = p_wrap;
		_mipmap = p_mipmap;
	}

	void Texture::setData(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_size, const Format& p_format)
	{
		_datas = p_data;
		_size = p_size;
		_format = p_format;
	}

	void Texture::setData(const uint8_t* p_data, const spk::Vector2UInt& p_size,
		const Format& p_format, const Filtering& p_filtering,
		const Wrap& p_wrap, const Mipmap& p_mipmap)
	{
		_size = p_size;
		_format = p_format;
		_filtering = p_filtering;
		_wrap = p_wrap;
		_mipmap = p_mipmap;
		_datas.assign(p_data, p_data + (p_size.x * p_size.y * _getBytesPerPixel(p_format)));
	}

	void Texture::setData(const uint8_t* p_data, const spk::Vector2UInt& p_size, const Format& p_format)
	{
		_size = p_size;
		_format = p_format;
		_datas.assign(p_data, p_data + (p_size.x * p_size.y * _getBytesPerPixel(p_format)));
	}

	void Texture::setProperties(const Filtering& p_filtering, const Wrap& p_wrap, const Mipmap& p_mipmap)
	{
		_filtering = p_filtering;
		_wrap = p_wrap;
		_mipmap = p_mipmap;
	}

	const std::vector<uint8_t>& Texture::data() const
	{
		return _datas;
	}

	const spk::Vector2UInt& Texture::size() const
	{
		return _size;
	}

	Texture::Format Texture::format() const
	{
		return _format;
	}

	Texture::Filtering Texture::filtering() const
	{
		return _filtering;
	}

	Texture::Wrap Texture::wrap() const
	{
		return _wrap;
	}

	Texture::Mipmap Texture::mipmap() const
	{
		return _mipmap;
	}
}
