#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "exception.hpp"
#include "graphics/image.hpp"
#include "graphics/opengl/index_buffer.hpp"
#include "graphics/opengl/sampler.hpp"
#include "graphics/opengl/texture.hpp"
#include "graphics/opengl/vertex_array.hpp"
#include "graphics/opengl/vertex_buffer.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	class TestTexture final : public spk::Texture
	{
	public:
		explicit TestTexture(Target target = Target::Texture2D) : Texture(target) {}

		using Texture::resizePixels;
		using Texture::setMipmap;
		using Texture::setPixels;
		using Texture::writePixels;
	};

	struct Position
	{
		float x;
		float y;
	};

	void configure(spk::VertexBuffer &vertices, spk::IndexBuffer &indices)
	{
		vertices.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 2);
		vertices.pushBack(Position{1.0f, 2.0f});
		indices.setType(spk::IndexBuffer::Type::UnsignedInt);
		indices.pushBack<std::uint32_t>(0);
		vertices.validate();
		indices.validate();
	}
}

TEST(TextureTest, FormatDescriptorsAndClassifiersCoverEveryPublicFormat)
{
	using Format = spk::Texture::Format;
	const std::array colorFormats{Format::RGB, Format::RGBA, Format::BGR, Format::BGRA, Format::GreyLevel, Format::DualChannel};
	for (const auto format : colorFormats)
	{
		const auto descriptor = spk::Texture::formatDescriptor(format);
		EXPECT_NE(descriptor.internalFormat, GL_NONE);
		EXPECT_NE(descriptor.externalFormat, GL_NONE);
		EXPECT_NE(descriptor.elementType, GL_NONE);
		EXPECT_TRUE(spk::Texture::isColorFormat(format));
		EXPECT_FALSE(spk::Texture::isDepthFormat(format));
	}
	EXPECT_TRUE(spk::Texture::isDepthFormat(Format::Depth24));
	EXPECT_TRUE(spk::Texture::isDepthFormat(Format::Depth32F));
	EXPECT_TRUE(spk::Texture::isDepthStencilFormat(Format::Depth24Stencil8));
	EXPECT_EQ(spk::Texture::formatDescriptor(Format::Error).internalFormat, GL_NONE);
}

TEST(TextureTest, PixelStorageResizePreservesOverlapAndZerosNewArea)
{
	TestTexture texture;
	const std::vector<std::uint8_t> pixels{1, 2, 3, 4, 5, 6};
	texture.setPixels(pixels, {2, 1}, spk::Texture::Format::RGB);
	EXPECT_EQ(texture.target(), spk::Texture::Target::Texture2D);
	EXPECT_EQ(texture.size(), spk::Vector2UInt(2, 1));
	EXPECT_EQ(texture.pixels(), pixels);
	texture.resizePixels({3, 2});
	EXPECT_EQ(texture.pixels(), (std::vector<std::uint8_t>{1, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
	texture.resizePixels({1, 1});
	EXPECT_EQ(texture.pixels(), (std::vector<std::uint8_t>{1, 2, 3}));
}

TEST(TextureTest, SubregionWritesSupportExactBoundaryAndRejectInvalidRanges)
{
	TestTexture texture;
	texture.setPixels(nullptr, {3, 2}, spk::Texture::Format::RGBA);
	const std::array<std::uint8_t, 8> replacement{1, 2, 3, 4, 5, 6, 7, 8};
	texture.writePixels(replacement.data(), {1, 1}, {2, 1});
	EXPECT_EQ(std::vector<std::uint8_t>(texture.pixels().end() - 8, texture.pixels().end()),
		(std::vector<std::uint8_t>{1, 2, 3, 4, 5, 6, 7, 8}));
	EXPECT_NO_THROW(texture.writePixels(nullptr, {3, 2}, {0, 0}));
	EXPECT_THROW(texture.writePixels(nullptr, {0, 0}, {1, 1}), std::invalid_argument);
	EXPECT_THROW(texture.writePixels(replacement.data(), {2, 0}, {2, 1}), std::out_of_range);
	EXPECT_THROW(texture.writePixels(replacement.data(), {0, 2}, {1, 1}), std::out_of_range);
}

TEST(TextureTest, InvalidFormatsSpansTargetsAndEmptyOperationsAreRejected)
{
	TestTexture texture;
	EXPECT_THROW(texture.setPixels(std::vector<std::uint8_t>{1}, {1, 1}, spk::Texture::Format::RGB), std::invalid_argument);
	EXPECT_THROW(texture.setPixels(nullptr, {1, 1}, spk::Texture::Format::Depth24), std::invalid_argument);
	EXPECT_THROW(texture.resizePixels({1, 1}), std::logic_error);
	EXPECT_THROW(texture.saveAsPng(std::filesystem::temp_directory_path() / "empty.png"), std::logic_error);
	TestTexture unsupported(spk::Texture::Target::Texture3D);
	EXPECT_THROW(unsupported.setPixels(nullptr, {1, 1}, spk::Texture::Format::RGBA), std::logic_error);
}

TEST(TextureTest, SynchronizeUploadsExactPixelsAndMipmapState)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	TestTexture texture;
	const std::vector<std::uint8_t> pixels{255, 0, 0, 255, 0, 255, 0, 128};
	texture.setPixels(pixels, {2, 1}, spk::Texture::Format::RGBA);
	texture.setMipmap(spk::Texture::Mipmap::Enable);
	texture.validate();
	texture.activate(openGL.renderContext());
	std::vector<std::uint8_t> retrieved(pixels.size());
	::glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, retrieved.data());
	EXPECT_EQ(retrieved, pixels);
	GLint maxLevel = -1;
	::glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &maxLevel);
	EXPECT_EQ(maxLevel, 1);
}

TEST(TextureTest, EmptyAndUnsupportedTexturesRejectSynchronization)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	TestTexture empty;
	empty.validate();
	EXPECT_THROW(empty.activate(openGL.renderContext()), spk::Exception);
	TestTexture unsupported(spk::Texture::Target::TextureCubeMap);
	unsupported.validate();
	EXPECT_THROW(unsupported.activate(openGL.renderContext()), spk::Exception);
}

TEST(TextureTest, MovePreservesIdentityAndPixelContent)
{
	TestTexture source;
	source.setPixels(std::vector<std::uint8_t>{1, 2, 3}, {1, 1}, spk::Texture::Format::RGB);
	const auto identifier = source.identifier();
	TestTexture moved(std::move(source));
	EXPECT_EQ(moved.identifier(), identifier);
	EXPECT_EQ(moved.pixels(), (std::vector<std::uint8_t>{1, 2, 3}));
}

TEST(TextureTest, DISABLED_RenderTargetContractNeedsPublicConstructionSeam)
{
	GTEST_SKIP() << "Texture render-target allocation is private and only framebuffer internals are friends; no public fixture can exercise it.";
}

TEST(TextureTest, DISABLED_ByteCountOverflowIsUnreachableWithPublic32BitDimensions)
{
	GTEST_SKIP() << "On this 64-bit build, two uint32 dimensions times four bytes cannot overflow size_t.";
}

TEST(TextureTest, DISABLED_OpenGLAndPngFailureInjectionNeedDedicatedSeams)
{
	GTEST_SKIP() << "The API cannot inject glGenTextures or stb_image_write failures deterministically.";
}

TEST(SamplerTest, PropertiesTextureAndBindingAreObservableInOpenGL)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	TestTexture texture;
	texture.setPixels(std::vector<std::uint8_t>{255, 255, 255, 255}, {1, 1}, spk::Texture::Format::RGBA);
	texture.validate();
	spk::Sampler sampler(2, spk::Sampler::Filtering::Nearest, spk::Sampler::Wrap::Repeat, spk::Sampler::MipmapFiltering::Nearest);
	sampler.setTexture(&texture);
	sampler.validate();
	sampler.activate(openGL.renderContext());

	EXPECT_EQ(sampler.bindingPoint(), 2u);
	EXPECT_EQ(sampler.texture(), &texture);
	GLint samplerIdentifier = 0;
	::glGetIntegeri_v(GL_SAMPLER_BINDING, 2, &samplerIdentifier);
	ASSERT_NE(samplerIdentifier, 0);
	GLint value = 0;
	::glGetSamplerParameteriv(static_cast<GLuint>(samplerIdentifier), GL_TEXTURE_MAG_FILTER, &value);
	EXPECT_EQ(value, GL_NEAREST);
	::glGetSamplerParameteriv(static_cast<GLuint>(samplerIdentifier), GL_TEXTURE_MIN_FILTER, &value);
	EXPECT_EQ(value, GL_NEAREST_MIPMAP_NEAREST);
	::glGetSamplerParameteriv(static_cast<GLuint>(samplerIdentifier), GL_TEXTURE_WRAP_S, &value);
	EXPECT_EQ(value, GL_REPEAT);
}

TEST(SamplerTest, AllSettersAndCombinedPropertiesUpdateAccessors)
{
	spk::Sampler sampler(7);
	EXPECT_EQ(sampler.filtering(), spk::Sampler::Filtering::Linear);
	EXPECT_EQ(sampler.wrap(), spk::Sampler::Wrap::ClampToEdge);
	EXPECT_EQ(sampler.mipmapFiltering(), spk::Sampler::MipmapFiltering::Disabled);
	sampler.setFiltering(spk::Sampler::Filtering::Nearest);
	sampler.setWrap(spk::Sampler::Wrap::MirroredRepeat);
	sampler.setMipmapFiltering(spk::Sampler::MipmapFiltering::Linear);
	EXPECT_EQ(sampler.filtering(), spk::Sampler::Filtering::Nearest);
	EXPECT_EQ(sampler.wrap(), spk::Sampler::Wrap::MirroredRepeat);
	EXPECT_EQ(sampler.mipmapFiltering(), spk::Sampler::MipmapFiltering::Linear);
	sampler.setProperties(spk::Sampler::Filtering::Linear, spk::Sampler::Wrap::ClampToBorder, spk::Sampler::MipmapFiltering::Nearest);
	EXPECT_EQ(sampler.wrap(), spk::Sampler::Wrap::ClampToBorder);
}

TEST(SamplerTest, ActivationWithoutTextureIsRejected)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Sampler sampler(0);
	sampler.validate();
	EXPECT_THROW(sampler.activate(openGL.renderContext()), spk::Exception);
}

TEST(SamplerTest, DISABLED_CreationFailureNeedsOpenGLInjectionSeam)
{
	GTEST_SKIP() << "The public API cannot force glGenSamplers to return zero.";
}

TEST(VertexArrayTest, MissingEitherBufferIsRejected)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::VertexArray array;
	array.validate();
	EXPECT_THROW(array.activate(openGL.renderContext()), spk::Exception);
	spk::VertexBuffer vertices;
	spk::IndexBuffer indices;
	configure(vertices, indices);
	spk::VertexArray onlyVertices;
	onlyVertices.setVertexBuffer(vertices);
	onlyVertices.validate();
	EXPECT_THROW(onlyVertices.activate(openGL.renderContext()), spk::Exception);
}

TEST(VertexArrayTest, ConfiguresAttributesAndReconfiguresAfterBufferReplacement)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::VertexBuffer firstVertices;
	spk::IndexBuffer firstIndices;
	configure(firstVertices, firstIndices);
	spk::VertexArray array;
	array.setVertexBuffer(firstVertices);
	array.setIndexBuffer(firstIndices);
	array.validate();
	array.activate(openGL.renderContext());
	GLint enabled = 0;
	::glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
	EXPECT_EQ(enabled, GL_TRUE);

	spk::VertexBuffer secondVertices;
	spk::IndexBuffer secondIndices;
	configure(secondVertices, secondIndices);
	array.setVertexBuffer(secondVertices);
	array.setIndexBuffer(secondIndices);
	EXPECT_NO_THROW(array.activate(openGL.renderContext()));
}

TEST(VertexArrayTest, DISABLED_OversizedStrideNeedsDeterministicConfigurationSeam)
{
	GTEST_SKIP() << "Constructing a stride above GLsizei max through padding would require a multi-gigabyte logical layout; no synthetic seam exists.";
}

TEST(VertexArrayTest, DISABLED_CreationFailureNeedsOpenGLInjectionSeam)
{
	GTEST_SKIP() << "The public API cannot force glGenVertexArrays to return zero.";
}
