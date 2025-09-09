#pragma once

#include <GL/glew.h>

#include <GL/gl.h>

#include "structure/graphics/spk_geometry_2d.hpp"
#include <cstring>
#include <deque>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace spk
{
	namespace OpenGL
	{
		class SamplerObject;
		class TextureCollection;
	}

	class Texture
	{
		friend class OpenGL::SamplerObject;
		friend class OpenGL::TextureCollection;

	public:
		using ID = long;

		struct Section
		{
			union {
				spk::Vector2 anchor;
				struct
				{
					float x;
					float y;
				};
			};

			union {
				spk::Vector2 size;
				struct
				{
					float width;
					float height;
				};
			};

			Section();
			Section(spk::Vector2 p_anchor, spk::Vector2 p_size);

			bool operator==(const Section &p_other) const noexcept;
			bool operator!=(const Section &p_other) const noexcept;
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
		static std::deque<ID> &_availableIDs()
		{
			static std::deque<ID> ids;
			return ids;
		}
		static inline ID _nextID = 0;
		static inline ID InvalidID = -1;

		static ID _takeId();
		static void _releaseId(const Texture::ID &p_id);

		ID _id;

		mutable bool _needUpdate = false;
		mutable bool _needSettings = false;
		std::vector<uint8_t> _pixels;
		spk::Vector2UInt _size;
		Format _format;
		Filtering _filtering;
		Wrap _wrap;
		Mipmap _mipmap;

		size_t _getBytesPerPixel(const Format &p_format) const;

	public:
		Texture();
		virtual ~Texture();

		Texture(const Texture &p_other) = delete;
		Texture &operator=(const Texture &p_other) = delete;

		Texture(Texture &&p_other) noexcept;
		Texture &operator=(Texture &&p_other) noexcept;

		void setPixels(
			const std::vector<uint8_t> &p_data,
			const spk::Vector2UInt &p_size,
			const Format &p_format,
			const Filtering &p_filtering,
			const Wrap &p_wrap,
			const Mipmap &p_mipmap);

		void setPixels(const std::vector<uint8_t> &p_data, const spk::Vector2UInt &p_size, const Format &p_format);

		void setPixels(
			const uint8_t *p_data,
			const spk::Vector2UInt &p_size,
			const Format &p_format,
			const Filtering &p_filtering,
			const Wrap &p_wrap,
			const Mipmap &p_mipmap);

		void setPixels(const uint8_t *p_data, const spk::Vector2UInt &p_size, const Format &p_format);
		void setProperties(const Filtering &p_filtering, const Wrap &p_wrap, const Mipmap &p_mipmap);

		const std::vector<uint8_t> &pixels() const;
		const spk::Vector2UInt &size() const;
		Format format() const;
		Filtering filtering() const;
		Wrap wrap() const;
		Mipmap mipmap() const;

		void saveAsPng(const std::filesystem::path &p_path) const;
	};
}
