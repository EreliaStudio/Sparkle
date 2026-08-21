#include "texture.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>
#include <stdexcept>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "render_context.hpp"

namespace spk
{
	class Texture::Instance final : public GPUResource::Instance
	{
	public:
		GLuint identifier = 0;
		GLenum target = 0;
		Vector2UInt size{0, 0};
		GLint internalFormat = GL_NONE;

		Instance()
		{
			glGenTextures(1, &identifier);
			if (identifier == 0)
			{
				throw std::runtime_error("Failed to create OpenGL texture");
			}
		}

		~Instance() override
		{
			if (identifier != 0)
			{
				glDeleteTextures(1, &identifier);
			}
		}
	};

	const Texture::Section Texture::Section::whole({0.0f, 0.0f}, {1.0f, 1.0f});

	Texture::Section::Section() :
		anchor(0.0f, 0.0f),
		size(0.0f, 0.0f)
	{
	}

	Texture::Section::Section(Vector2 anchor, Vector2 size) :
		anchor(anchor),
		size(size)
	{
	}

	bool Texture::Section::operator==(const Section &other) const noexcept
	{
		return anchor == other.anchor && size == other.size;
	}

	bool Texture::Section::operator!=(const Section &other) const noexcept
	{
		return (*this == other) == false;
	}

	GPUResource::Kind Texture::_kind() const noexcept
	{
		return GPUResource::Kind::Texture;
	}

	GPUResource::RecyclingScore Texture::_recyclingScore(const GPUResource::Instance &base) const noexcept
	{
		const auto &instance = static_cast<const Instance &>(base);
		const GLenum target = _openGLTarget(_textureTarget);

		if (instance.target != target)
		{
			return 0;
		}

		const TextureFormat descriptor = formatDescriptor(_format);
		if (instance.size == _size && instance.internalFormat == descriptor.internalFormat)
		{
			return 2;
		}

		return 1;
	}

	GLenum Texture::_openGLTarget(Target target) noexcept
	{
		switch (target)
		{
		case Target::Texture1D:
			return GL_TEXTURE_1D;
		case Target::Texture2D:
			return GL_TEXTURE_2D;
		case Target::Texture3D:
			return GL_TEXTURE_3D;
		case Target::TextureCubeMap:
			return GL_TEXTURE_CUBE_MAP;
		}
		return GL_TEXTURE_2D;
	}

	std::size_t Texture::_bytesPerPixel(Format format)
	{
		switch (format)
		{
		case Format::GreyLevel:
			return 1;
		case Format::DualChannel:
			return 2;
		case Format::RGB:
		case Format::BGR:
			return 3;
		case Format::RGBA:
		case Format::BGRA:
			return 4;
		case Format::Depth24:
		case Format::Depth32F:
		case Format::Depth24Stencil8:
		case Format::Error:
			break;
		}

		throw std::invalid_argument("Texture format cannot contain CPU pixel data");
	}

	std::size_t Texture::_checkedByteCount(const Vector2UInt &size, Format format)
	{
		const std::size_t width = static_cast<std::size_t>(size.x);
		const std::size_t height = static_cast<std::size_t>(size.y);
		const std::size_t bytesPerPixel = _bytesPerPixel(format);

		if (width != 0 && height > std::numeric_limits<std::size_t>::max() / width)
		{
			throw std::overflow_error("Texture size overflow");
		}

		const std::size_t pixelCount = width * height;
		if (pixelCount != 0 && bytesPerPixel > std::numeric_limits<std::size_t>::max() / pixelCount)
		{
			throw std::overflow_error("Texture size overflow");
		}

		return pixelCount * bytesPerPixel;
	}

	void Texture::_bindToUnit(std::size_t bindingPoint, RenderContext &context) const
	{
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(bindingPoint));
		activate(context);
	}

	void Texture::_allocateRenderTarget(const Vector2UInt &size, Format format)
	{
		if (_textureTarget != Target::Texture2D)
		{
			throw std::logic_error("Render targets currently support Texture2D only");
		}
		if (size.x == 0 || size.y == 0)
		{
			throw std::invalid_argument("Render target size cannot be zero");
		}
		if (format == Format::Error)
		{
			throw std::invalid_argument("Render target format cannot be invalid");
		}

		_pixels.clear();
		_size = size;
		_format = format;
		_contentSource = ContentSource::RenderTarget;
		_mipmap = Mipmap::Disable;
	}

	Texture::Texture(Target target) :
		_textureTarget(target)
	{
	}

	void Texture::setPixels(const std::uint8_t *data, const Vector2UInt &size, Format format)
	{
		if (_textureTarget != Target::Texture2D)
		{
			throw std::logic_error("CPU pixel storage currently supports Texture2D only");
		}
		if (isColorFormat(format) == false)
		{
			throw std::invalid_argument("Depth formats cannot contain CPU pixel data");
		}

		const std::size_t byteCount = _checkedByteCount(size, format);
		_pixels.resize(byteCount);

		if (data == nullptr)
		{
			std::fill(_pixels.begin(), _pixels.end(), 0);
		}
		else
		{
			std::memcpy(_pixels.data(), data, byteCount);
		}

		_size = size;
		_format = format;
		_contentSource = ContentSource::PixelData;
	}

	void Texture::setPixels(std::span<const std::uint8_t> data, const Vector2UInt &size, Format format)
	{
		const std::size_t expectedSize = _checkedByteCount(size, format);
		if (data.size() != expectedSize)
		{
			throw std::invalid_argument("Texture pixel data size does not match dimensions and format");
		}

		setPixels(data.data(), size, format);
	}

	void Texture::resizePixels(const Vector2UInt &size)
	{
		if (_contentSource != ContentSource::PixelData)
		{
			throw std::logic_error("Cannot resize CPU pixels of a render-target Texture");
		}
		if (_format == Format::Error)
		{
			throw std::logic_error("Cannot resize a Texture without a configured format");
		}

		const std::size_t bytesPerPixel = _bytesPerPixel(_format);
		std::vector<std::uint8_t> result(_checkedByteCount(size, _format), 0);
		const std::size_t copyWidth = std::min<std::size_t>(_size.x, size.x);
		const std::size_t copyHeight = std::min<std::size_t>(_size.y, size.y);

		for (std::size_t y = 0; y < copyHeight; ++y)
		{
			const std::size_t sourceOffset = y * static_cast<std::size_t>(_size.x) * bytesPerPixel;
			const std::size_t destinationOffset = y * static_cast<std::size_t>(size.x) * bytesPerPixel;
			std::memcpy(result.data() + destinationOffset, _pixels.data() + sourceOffset, copyWidth * bytesPerPixel);
		}

		_pixels.swap(result);
		_size = size;
	}

	void Texture::writePixels(const std::uint8_t *data, const Vector2UInt &position, const Vector2UInt &size)
	{
		if (_contentSource != ContentSource::PixelData)
		{
			throw std::logic_error("Cannot write CPU pixels into a render-target Texture");
		}
		if (data == nullptr && size.x != 0 && size.y != 0)
		{
			throw std::invalid_argument("Texture pixel source cannot be null");
		}
		if (position.x > _size.x || size.x > _size.x - position.x ||
			position.y > _size.y || size.y > _size.y - position.y)
		{
			throw std::out_of_range("Texture pixel write exceeds texture bounds");
		}

		const std::size_t bytesPerPixel = _bytesPerPixel(_format);
		for (std::size_t y = 0; y < static_cast<std::size_t>(size.y); ++y)
		{
			const std::size_t sourceOffset = y * static_cast<std::size_t>(size.x) * bytesPerPixel;
			const std::size_t destinationPixel =
				static_cast<std::size_t>(position.x) +
				(static_cast<std::size_t>(position.y) + y) * static_cast<std::size_t>(_size.x);

			std::memcpy(
				_pixels.data() + destinationPixel * bytesPerPixel,
				data + sourceOffset,
				static_cast<std::size_t>(size.x) * bytesPerPixel);
		}
	}

	void Texture::setMipmap(Mipmap mipmap) noexcept
	{
		if (_mipmap == mipmap)
		{
			return;
		}

		_mipmap = mipmap;
	}

	std::unique_ptr<GPUResource::Instance> Texture::_create(RenderContext &) const
	{
		return std::make_unique<Instance>();
	}

	void Texture::_synchronize(GPUResource::Instance &base, RenderContext &) const
	{
		if (_textureTarget != Target::Texture2D)
		{
			throw std::logic_error("Texture synchronization currently supports Texture2D only");
		}
		if (_size.x == 0 || _size.y == 0 || _format == Format::Error)
		{
			throw std::logic_error("Cannot synchronize an empty Texture");
		}

		const TextureFormat descriptor = formatDescriptor(_format);
		if (descriptor.internalFormat == GL_NONE || descriptor.externalFormat == GL_NONE || descriptor.elementType == GL_NONE)
		{
			throw std::logic_error("Texture format cannot be synchronized");
		}

		auto &instance = static_cast<Instance &>(base);
		const GLenum target = _openGLTarget(_textureTarget);

		if (instance.target != 0 && instance.target != target)
		{
			throw std::logic_error("Texture received an incompatible GPU instance");
		}

		glBindTexture(target, instance.identifier);

		const bool storageMatches =
			instance.target == target &&
			instance.size == _size &&
			instance.internalFormat == descriptor.internalFormat;

		GLint previousAlignment = 0;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		const void *pixels = _contentSource == ContentSource::PixelData && _pixels.empty() == false
								 ? _pixels.data()
								 : nullptr;

		if (storageMatches)
		{
			if (pixels != nullptr)
			{
				glTexSubImage2D(
					target,
					0,
					0,
					0,
					static_cast<GLsizei>(_size.x),
					static_cast<GLsizei>(_size.y),
					descriptor.externalFormat,
					descriptor.elementType,
					pixels);
			}
		}
		else
		{
			glTexImage2D(
				target,
				0,
				descriptor.internalFormat,
				static_cast<GLsizei>(_size.x),
				static_cast<GLsizei>(_size.y),
				0,
				descriptor.externalFormat,
				descriptor.elementType,
				pixels);

			instance.size = _size;
			instance.internalFormat = descriptor.internalFormat;
			instance.target = target;
		}

		glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
		glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);

		if (_mipmap == Mipmap::Enable && _contentSource == ContentSource::PixelData)
		{
			const unsigned int maximumDimension = std::max(_size.x, _size.y);
			glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(std::bit_width(maximumDimension) - 1));
			glGenerateMipmap(target);
		}
		else
		{
			glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, 0);
		}
	}

	void Texture::_bind(GPUResource::Instance &base, RenderContext &) const
	{
		auto &instance = static_cast<Instance &>(base);
		glBindTexture(_openGLTarget(_textureTarget), instance.identifier);
	}

	TextureFormat Texture::formatDescriptor(Format format) noexcept
	{
		switch (format)
		{
		case Format::RGB:
			return {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, false, false};
		case Format::RGBA:
			return {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, false, false};
		case Format::BGR:
			return {GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE, false, false};
		case Format::BGRA:
			return {GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE, false, false};
		case Format::GreyLevel:
			return {GL_R8, GL_RED, GL_UNSIGNED_BYTE, false, false};
		case Format::DualChannel:
			return {GL_RG8, GL_RG, GL_UNSIGNED_BYTE, false, false};
		case Format::Depth24:
			return {GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, true, false};
		case Format::Depth32F:
			return {GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, true, false};
		case Format::Depth24Stencil8:
			return {GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, true, true};
		case Format::Error:
			return {};
		}
		return {};
	}

	bool Texture::isColorFormat(Format format) noexcept
	{
		switch (format)
		{
		case Format::RGB:
		case Format::RGBA:
		case Format::BGR:
		case Format::BGRA:
		case Format::GreyLevel:
		case Format::DualChannel:
			return true;
		default:
			return false;
		}
	}

	bool Texture::isDepthFormat(Format format) noexcept
	{
		return format == Format::Depth24 || format == Format::Depth32F;
	}

	bool Texture::isDepthStencilFormat(Format format) noexcept
	{
		return format == Format::Depth24Stencil8;
	}

	Texture::Target Texture::target() const noexcept
	{
		return _textureTarget;
	}

	const Vector2UInt &Texture::size() const noexcept
	{
		return _size;
	}

	Texture::Format Texture::format() const noexcept
	{
		return _format;
	}

	Texture::ContentSource Texture::contentSource() const noexcept
	{
		return _contentSource;
	}

	bool Texture::isRenderTarget() const noexcept
	{
		return _contentSource == ContentSource::RenderTarget;
	}

	Texture::Mipmap Texture::mipmap() const noexcept
	{
		return _mipmap;
	}

	const std::vector<std::uint8_t> &Texture::pixels() const
	{
		if (_contentSource == ContentSource::RenderTarget)
		{
			throw std::logic_error("Render-target textures do not expose CPU pixel data");
		}

		return _pixels;
	}

	void Texture::saveAsPng(const std::filesystem::path &path) const
	{
		if (_contentSource == ContentSource::RenderTarget || isColorFormat(_format) == false)
		{
			throw std::logic_error("Only CPU color textures can be exported as PNG files");
		}
		if (_pixels.empty() || _size.x == 0 || _size.y == 0)
		{
			throw std::logic_error("Cannot save an empty Texture");
		}

		const int channels = static_cast<int>(_bytesPerPixel(_format));
		const std::uint8_t *data = _pixels.data();
		std::vector<std::uint8_t> converted;

		if (_format == Format::BGR || _format == Format::BGRA)
		{
			converted = _pixels;
			for (std::size_t index = 0; index < converted.size(); index += static_cast<std::size_t>(channels))
			{
				std::swap(converted[index], converted[index + 2]);
			}
			data = converted.data();
		}

		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}

		const int stride = static_cast<int>(_size.x) * channels;
		if (stbi_write_png(
				path.string().c_str(),
				static_cast<int>(_size.x),
				static_cast<int>(_size.y),
				channels,
				data,
				stride) == 0)
		{
			throw std::runtime_error("Failed to write PNG file: " + path.string());
		}
	}
}
