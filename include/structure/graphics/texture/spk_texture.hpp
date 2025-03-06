#pragma once

#include <GL/glew.h>
#include <GL/gl.h>
#include <vector>
#include <unordered_map>
#include <deque>
#include <cstring>
#include "structure/graphics/spk_geometry_2D.hpp"

namespace spk
{
	class Texture
	{
	public:
		struct Section
		{
			union
			{
				spk::Vector2 anchor;
				struct
				{
					float x;
					float y;
				};
			};

			union
			{
				spk::Vector2 size;
				struct
				{
					float width;
					float height;
				};
			};

			Section();
			Section(spk::Vector2 p_anchor, spk::Vector2 p_size);
		};

		enum class Format
		{
			RGB = GL_RGB,
			RGBA = GL_RGBA,
			BGR = GL_BGR,
			BGRA = GL_BGRA,
			GreyLevel = GL_RED,
			DualChannel = GL_RG,
			Error = GL_NONE
		};

		enum class Filtering
		{
			Nearest = GL_NEAREST,
			Linear = GL_LINEAR
		};

		enum class Wrap
		{
			Repeat = GL_REPEAT,
			ClampToEdge = GL_CLAMP_TO_EDGE,
			ClampToBorder = GL_CLAMP_TO_BORDER
		};

		enum class Mipmap
		{
			Disable,
			Enable
		};

	private:
		using ID = long;

		static inline std::deque<ID> _availableIDs;
		static inline ID _nextID = 0;
		static inline ID InvalidID = -1;

		static ID takeID();
		static void releaseID(const Texture::ID& p_id);

		ID _id;

		std::vector<uint8_t> _datas;
		spk::Vector2UInt _size;
		Format _format;
		Filtering _filtering;
		Wrap _wrap;
		Mipmap _mipmap;

		size_t _getBytesPerPixel(const Format& format) const;

	public:
		Texture();
		~Texture();
		
		Texture(const Texture& p_other) = delete;
		Texture& operator=(const Texture& p_other) = delete;

		Texture(Texture&& p_other) noexcept;
		Texture& operator=(Texture&& p_other) noexcept;

		void setData(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_size,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		void setData(const std::vector<uint8_t>& p_data, const spk::Vector2UInt& p_size, const Format& p_format);

		void setData(const uint8_t* p_data, const spk::Vector2UInt& p_size,
			const Format& p_format, const Filtering& p_filtering,
			const Wrap& p_wrap, const Mipmap& p_mipmap);

		void setData(const uint8_t* p_data, const spk::Vector2UInt& p_size, const Format& p_format);
		void setProperties(const Filtering& p_filtering, const Wrap& p_wrap, const Mipmap& p_mipmap);

		const std::vector<uint8_t>& data() const;
		const spk::Vector2UInt& size() const;
		Format format() const;
		Filtering filtering() const;
		Wrap wrap() const;
		Mipmap mipmap() const;
	};
}
