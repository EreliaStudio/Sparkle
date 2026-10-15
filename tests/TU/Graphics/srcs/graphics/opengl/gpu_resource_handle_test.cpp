#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <concepts>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "graphics/opengl/framebuffer.hpp"
#include "graphics/opengl/index_buffer.hpp"
#include "graphics/opengl/program.hpp"
#include "graphics/opengl/sampler.hpp"
#include "graphics/opengl/shader_storage_buffer.hpp"
#include "graphics/opengl/texture.hpp"
#include "graphics/opengl/uniform_buffer.hpp"
#include "graphics/opengl/vertex_array.hpp"
#include "graphics/opengl/vertex_buffer.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	class TestTexture final : public spk::Texture
	{
	public:
		TestTexture() :
			Texture(Target::Texture2D)
		{
		}

		using Texture::setPixels;
		using Texture::writePixels;
	};

	const std::string VertexShader = R"(#version 430 core
void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }
)";

	const std::string FragmentShader = R"(#version 430 core
layout(location = 0) out vec4 outputColor;
void main() { outputColor = vec4(1.0); }
)";

	struct Vertex
	{
		float x;
		float y;
	};

	template <typename THandle>
	constexpr bool ValidHandleTraits =
		std::is_default_constructible_v<THandle> &&
		std::is_nothrow_copy_constructible_v<THandle> &&
		std::is_nothrow_move_constructible_v<THandle> &&
		std::is_nothrow_copy_assignable_v<THandle> &&
		std::is_nothrow_move_assignable_v<THandle>;

	static_assert(ValidHandleTraits<spk::Texture::Handle>);
	static_assert(ValidHandleTraits<spk::VertexBuffer::Handle>);
	static_assert(ValidHandleTraits<spk::IndexBuffer::Handle>);
	static_assert(ValidHandleTraits<spk::UniformBuffer::Handle>);
	static_assert(ValidHandleTraits<spk::ShaderStorageBuffer::Handle>);
	static_assert(ValidHandleTraits<spk::VertexArray::Handle>);
	static_assert(ValidHandleTraits<spk::Program::Handle>);
	static_assert(ValidHandleTraits<spk::Sampler::Handle>);
	static_assert(ValidHandleTraits<spk::Framebuffer::Handle>);

	static_assert(std::same_as<decltype(*std::declval<const spk::Texture::Handle &>()), const spk::Texture::State &>);
	static_assert(std::same_as<decltype(std::declval<const spk::Texture::Handle &>().operator->()), const spk::Texture::State *>);
}

TEST(GPUResourceHandleTest, EmptyHandleHasNoIdentityAndRejectsAccessAndActivation)
{
	spk::Texture::Handle handle;
	EXPECT_FALSE(handle.valid());
	EXPECT_FALSE(static_cast<bool>(handle));
	EXPECT_EQ(handle.identifier(), 0u);
	EXPECT_EQ(handle.generation(), 0u);
	EXPECT_THROW((void)handle.state(), std::logic_error);
	EXPECT_THROW(handle.activate(sparkle_test::OpenGLTestContext::instance().renderContext()), std::logic_error);
}

TEST(GPUResourceHandleTest, CopiesAndMovesShareIdentityWithoutCopyingState)
{
	TestTexture texture;
	texture.setPixels(std::array<std::uint8_t, 4>{1, 2, 3, 4}, {1, 1}, spk::Texture::Format::RGBA);
	texture.validate();

	auto first = texture.handle();
	auto copy = first;
	auto moved = std::move(copy);

	EXPECT_EQ(first.identifier(), texture.identifier());
	EXPECT_EQ(moved.identifier(), texture.identifier());
	EXPECT_EQ(std::addressof(first.state()), std::addressof(moved.state()));
	EXPECT_FALSE(copy);
}

TEST(GPUResourceHandleTest, HandleObservesLiveGenerationAndTypedTextureMetadata)
{
	TestTexture texture;
	texture.setPixels(std::array<std::uint8_t, 4>{1, 2, 3, 4}, {1, 1}, spk::Texture::Format::RGBA);
	texture.validate();
	auto handle = texture.handle();
	const auto generation = handle.generation();

	texture.writePixels(std::array<std::uint8_t, 4>{5, 6, 7, 8}.data(), {0, 0}, {1, 1});
	texture.validate();

	EXPECT_GT(handle.generation(), generation);
	EXPECT_EQ(handle.generation(), texture.generation());
	EXPECT_EQ(handle->size(), spk::Vector2UInt(1, 1));
	EXPECT_EQ(handle->format(), spk::Texture::Format::RGBA);
}

TEST(GPUResourceHandleTest, TextureHandleCreatesInstanceAfterOwnerWasDestroyed)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Texture::Handle handle;
	{
		TestTexture texture;
		texture.setPixels(std::array<std::uint8_t, 4>{10, 20, 30, 255}, {1, 1}, spk::Texture::Format::RGBA);
		texture.validate();
		handle = texture.handle();
	}

	EXPECT_NO_THROW(handle.activate(openGL.renderContext()));
	GLint identifier = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &identifier);
	EXPECT_NE(identifier, 0);
}

TEST(GPUResourceHandleTest, BufferHandlesRetainTheirLiveDataAndBindings)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();

	spk::VertexBuffer::Handle vertices;
	{
		spk::VertexBuffer buffer;
		buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 2);
		buffer.pushBack(Vertex{2.0f, 3.0f});
		buffer.validate();
		vertices = buffer.handle();
	}
	EXPECT_EQ(vertices->size(), sizeof(Vertex));
	EXPECT_EQ(vertices->stride(), sizeof(Vertex));
	EXPECT_NO_THROW(vertices.activate(openGL.renderContext()));
	GLint vertexIdentifier = 0;
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vertexIdentifier);
	EXPECT_NE(vertexIdentifier, 0);

	spk::UniformBuffer::Handle uniform;
	{
		spk::UniformBuffer buffer(3, sizeof(std::uint32_t));
		buffer.setData(std::uint32_t{42});
		buffer.validate();
		uniform = buffer.handle();
	}
	EXPECT_NO_THROW(uniform.activate(openGL.renderContext()));
	GLint uniformIdentifier = 0;
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 3, &uniformIdentifier);
	EXPECT_NE(uniformIdentifier, 0);

	spk::ShaderStorageBuffer::Handle storage;
	{
		spk::ShaderStorageBuffer buffer(4, 0, sizeof(std::uint32_t));
		buffer.resize(1);
		const std::array<std::uint32_t, 1> value{77};
		buffer.setDynamicData<std::uint32_t>(value);
		buffer.validate();
		storage = buffer.handle();
	}
	EXPECT_NO_THROW(storage.activate(openGL.renderContext()));
	GLint storageIdentifier = 0;
	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 4, &storageIdentifier);
	EXPECT_NE(storageIdentifier, 0);
}

TEST(GPUResourceHandleTest, VertexArrayStateRetainsBothBufferDependencies)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::VertexArray::Handle arrayHandle;
	{
		spk::VertexBuffer vertices;
		vertices.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 2);
		vertices.pushBack(Vertex{0.0f, 0.0f});
		vertices.validate();

		spk::IndexBuffer indices;
		indices.setType(spk::IndexBuffer::Type::UnsignedInt);
		indices.pushBack(std::uint32_t{0});
		indices.validate();

		spk::VertexArray array;
		array.setVertexBuffer(vertices);
		array.setIndexBuffer(indices);
		array.validate();
		arrayHandle = array.handle();
	}

	EXPECT_NO_THROW(arrayHandle.activate(openGL.renderContext()));
	GLint identifier = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &identifier);
	EXPECT_NE(identifier, 0);
}

TEST(GPUResourceHandleTest, ProgramHandleCreatesAndBindsProgramAfterOwnerDestruction)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Program::Handle handle;
	{
		spk::Program program(VertexShader, FragmentShader);
		handle = program.handle();
	}

	EXPECT_NO_THROW(handle.activate(openGL.renderContext()));
	GLint identifier = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &identifier);
	EXPECT_NE(identifier, 0);
}

TEST(GPUResourceHandleTest, SamplerStateRetainsTextureDependencyAfterBothOwnersAreDestroyed)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Sampler::Handle handle;
	{
		TestTexture texture;
		texture.setPixels(std::array<std::uint8_t, 4>{255, 255, 255, 255}, {1, 1}, spk::Texture::Format::RGBA);
		texture.validate();

		spk::Sampler sampler(5);
		sampler.setTexture(&texture);
		sampler.validate();
		handle = sampler.handle();
	}

	EXPECT_NO_THROW(handle.activate(openGL.renderContext()));
	GLint samplerIdentifier = 0;
	GLint textureIdentifier = 0;
	glGetIntegeri_v(GL_SAMPLER_BINDING, 5, &samplerIdentifier);
	glActiveTexture(GL_TEXTURE0 + 5);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureIdentifier);
	EXPECT_NE(samplerIdentifier, 0);
	EXPECT_NE(textureIdentifier, 0);
}

TEST(GPUResourceHandleTest, FramebufferHandleRetainsAttachmentsAfterOwnerDestruction)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Framebuffer::Handle handle;
	{
		spk::Framebuffer framebuffer({8, 8});
		handle = framebuffer.handle();
	}

	EXPECT_NO_THROW(handle.activate(openGL.renderContext()));
	GLint identifier = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &identifier);
	EXPECT_NE(identifier, 0);
	EXPECT_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
}
