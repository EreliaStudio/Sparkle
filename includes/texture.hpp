#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

#include "gpu_resource.hpp"
#include "vector2.hpp"

namespace spk
{
	struct TextureFormat
	{
		GLint internalFormat = GL_NONE;
		GLenum externalFormat = GL_NONE;
		GLenum elementType = GL_NONE;
		bool depth = false;
		bool stencil = false;
	};

	class FrameBufferObject;
	class RenderContext;
	class Sampler;

	class Texture : public GPUResource
	{
		friend class FrameBufferObject;
		friend class Sampler;

	public:
		enum class Target
		{
			Texture1D,
			Texture2D,
			Texture3D,
			TextureCubeMap
		};

		enum class Format
		{
			RGB,
			RGBA,
			BGR,
			BGRA,
			GreyLevel,
			DualChannel,
			Depth24,
			Depth32F,
			Depth24Stencil8,
			Error
		};

		enum class ContentSource
		{
			PixelData,
			RenderTarget
		};

		enum class Mipmap
		{
			Disable,
			Enable
		};

		struct Section
		{
			Vector2 anchor;
			Vector2 size;

			static const Section whole;

			Section();
			Section(Vector2 anchor, Vector2 size);

			[[nodiscard]] bool operator==(const Section &other) const noexcept;
			[[nodiscard]] bool operator!=(const Section &other) const noexcept;
		};

	protected:
		class Instance;

	private:
		Target _textureTarget = Target::Texture2D;
		std::vector<std::uint8_t> _pixels;
		Vector2UInt _size{0, 0};
		Format _format = Format::Error;
		ContentSource _contentSource = ContentSource::PixelData;
		Mipmap _mipmap = Mipmap::Disable;

		[[nodiscard]] Kind _kind() const noexcept override;
		[[nodiscard]] RecyclingScore _recyclingScore(const GPUResource::Instance &instance) const noexcept override;
		[[nodiscard]] static GLenum _openGLTarget(Target target) noexcept;
		[[nodiscard]] static std::size_t _bytesPerPixel(Format format);
		[[nodiscard]] static std::size_t _checkedByteCount(const Vector2UInt &size, Format format);

		void _bindToUnit(std::size_t bindingPoint, RenderContext &context) const;
		void _allocateRenderTarget(const Vector2UInt &size, Format format);

	protected:
		explicit Texture(Target target = Target::Texture2D);

		void setPixels(const std::uint8_t *data, const Vector2UInt &size, Format format);
		void setPixels(std::span<const std::uint8_t> data, const Vector2UInt &size, Format format);
		void resizePixels(const Vector2UInt &size);
		void writePixels(const std::uint8_t *data, const Vector2UInt &position, const Vector2UInt &size);
		void setMipmap(Mipmap mipmap) noexcept;

		[[nodiscard]] std::unique_ptr<GPUResource::Instance> _create(RenderContext &context) const override;
		void _synchronize(GPUResource::Instance &instance, RenderContext &context) const override;
		void _bind(GPUResource::Instance &instance, RenderContext &context) const override;

	public:
		[[nodiscard]] static TextureFormat formatDescriptor(Format format) noexcept;
		[[nodiscard]] static bool isColorFormat(Format format) noexcept;
		[[nodiscard]] static bool isDepthFormat(Format format) noexcept;
		[[nodiscard]] static bool isDepthStencilFormat(Format format) noexcept;

		[[nodiscard]] Target target() const noexcept;
		[[nodiscard]] const Vector2UInt &size() const noexcept;
		[[nodiscard]] Format format() const noexcept;
		[[nodiscard]] ContentSource contentSource() const noexcept;
		[[nodiscard]] bool isRenderTarget() const noexcept;
		[[nodiscard]] Mipmap mipmap() const noexcept;
		[[nodiscard]] const std::vector<std::uint8_t> &pixels() const;

		void saveAsPng(const std::filesystem::path &path) const;
	};
}
