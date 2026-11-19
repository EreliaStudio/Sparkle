#pragma once

#include <cstdint>

#include "graphics/opengl/texture.hpp"

namespace spk
{
	class CubeMap final
	{
	public:
		enum class Face : std::uint8_t
		{
			PositiveX,
			NegativeX,
			PositiveY,
			NegativeY,
			PositiveZ,
			NegativeZ
		};

		using Handle = Texture::Handle;

	private:
		Texture _texture;

	public:
		explicit CubeMap(std::uint32_t faceSize, Texture::Format format = Texture::Format::Depth32F) :
			_texture(Texture::_createRenderAttachment(Texture::Target::TextureCubeMap, {faceSize, faceSize}, format))
		{
		}

		[[nodiscard]] Handle handle() const
		{
			return _texture.handle();
		}

		[[nodiscard]] std::uint32_t faceSize() const noexcept
		{
			return _texture.size().x;
		}

		[[nodiscard]] Texture::Format format() const noexcept
		{
			return _texture.format();
		}

		void activate(RenderContext &context) const
		{
			_texture.activate(context);
		}
	};
}
