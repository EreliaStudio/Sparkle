#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <type_traits>
#include <vector>

#include "graphics/opengl/index_buffer.hpp"
#include "graphics/opengl/layout_buffer.hpp"
#include "graphics/opengl/shader_storage_buffer.hpp"
#include "graphics/opengl/uniform_buffer.hpp"
#include "graphics/opengl/vertex_buffer.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	struct Vertex
	{
		float x;
		float y;
		std::uint32_t color;
	};

	struct UniformData
	{
		float values[4];
	};

	struct FixedData
	{
		std::uint32_t count;
		float scale;
	};

	void configureVertexBuffer(spk::VertexBuffer &buffer)
	{
		buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 2);
		buffer.addAttribute(
			1,
			spk::VertexBuffer::Attribute::Type::UnsignedByte,
			4,
			spk::VertexBuffer::Interpretation::Floating,
			true);
	}
}

TEST(VertexBufferTest, MixedLayoutResolvesOffsetsStrideAndConfigurationGeneration)
{
	spk::VertexBuffer buffer;
	const auto initialGeneration = buffer.configurationGeneration();
	configureVertexBuffer(buffer);

	ASSERT_EQ(buffer.attributes().size(), 2u);
	EXPECT_EQ(buffer.attributes()[0].offset, 0u);
	EXPECT_EQ(buffer.attributes()[1].offset, sizeof(float) * 2);
	EXPECT_EQ(buffer.stride(), sizeof(Vertex));
	EXPECT_GT(buffer.configurationGeneration(), initialGeneration);
	EXPECT_EQ(spk::VertexBuffer::openGLType(spk::VertexBuffer::Attribute::Type::Float), GL_FLOAT);
	EXPECT_EQ(spk::VertexBuffer::openGLType(spk::VertexBuffer::Attribute::Type::UnsignedByte), GL_UNSIGNED_BYTE);
}

TEST(VertexBufferTest, PushInsertResizeReserveCastAndClearPreserveTypedVertices)
{
	spk::VertexBuffer buffer;
	configureVertexBuffer(buffer);
	buffer.reserve<Vertex>(8);
	buffer.pushBack(Vertex{1.0f, 2.0f, 0x01020304u});
	const std::array<Vertex, 2> extra{{{3.0f, 4.0f, 5u}, {6.0f, 7.0f, 8u}}};
	buffer.insert<Vertex>(extra);
	ASSERT_EQ(buffer.count(), 3u);
	EXPECT_FLOAT_EQ(buffer.cast<Vertex>()[1].x, 3.0f);
	buffer.cast<Vertex>()[2].y = 9.0f;
	EXPECT_FLOAT_EQ(std::as_const(buffer).cast<Vertex>()[2].y, 9.0f);

	buffer.resize<Vertex>(5);
	EXPECT_EQ(buffer.count(), 5u);
	EXPECT_EQ(buffer.cast<Vertex>()[4].color, 0u);
	buffer.clear();
	EXPECT_EQ(buffer.count(), 0u);
	EXPECT_EQ(buffer.size(), 0u);
	buffer.clearConfiguration();
	EXPECT_EQ(buffer.stride(), 0u);
}

TEST(VertexBufferTest, InvalidAttributesDuplicateLocationsAndLiveLayoutEditsAreRejected)
{
	spk::VertexBuffer buffer;
	EXPECT_THROW(buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 0), std::invalid_argument);
	EXPECT_THROW(buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 5), std::invalid_argument);
	EXPECT_THROW(buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Double, 1), std::invalid_argument);
	EXPECT_THROW(
		buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 1, spk::VertexBuffer::Interpretation::Integer),
		std::invalid_argument);
	EXPECT_THROW(
		buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Int, 1, spk::VertexBuffer::Interpretation::Integer, true),
		std::invalid_argument);
	EXPECT_THROW(
		buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Int, 1, spk::VertexBuffer::Interpretation::Double),
		std::invalid_argument);

	configureVertexBuffer(buffer);
	EXPECT_THROW(buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 1), std::invalid_argument);
	buffer.pushBack(Vertex{});
	EXPECT_THROW(buffer.addPadding(1), std::logic_error);
	EXPECT_THROW(buffer.clearConfiguration(), std::logic_error);
}

TEST(VertexBufferTest, MissingOrWrongTypedLayoutAndSizeOverflowAreRejected)
{
	spk::VertexBuffer buffer;
	EXPECT_THROW(buffer.pushBack(Vertex{}), std::logic_error);
	buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 1);
	EXPECT_THROW(buffer.pushBack(Vertex{}), std::logic_error);
	EXPECT_THROW(buffer.resize<float>(std::numeric_limits<std::size_t>::max()), std::overflow_error);
	EXPECT_THROW((void)spk::VertexBuffer::openGLType(static_cast<spk::VertexBuffer::Attribute::Type>(255)), std::invalid_argument);
}

TEST(VertexBufferTest, EveryAttributeTypeAndValidInterpretationCanBeConfigured)
{
	spk::VertexBuffer buffer;
	using Type = spk::VertexBuffer::Attribute::Type;
	buffer.addAttribute(0, Type::Byte, 1, spk::VertexBuffer::Interpretation::Integer);
	buffer.addAttribute(1, Type::UnsignedByte, 2, spk::VertexBuffer::Interpretation::Floating, true);
	buffer.addAttribute(2, Type::Short, 3);
	buffer.addAttribute(3, Type::UnsignedShort, 4);
	buffer.addAttribute(4, Type::Int, 1);
	buffer.addAttribute(5, Type::UnsignedInt, 1, spk::VertexBuffer::Interpretation::Integer);
	buffer.addAttribute(6, Type::HalfFloat, 2);
	buffer.addAttribute(7, Type::Float, 3);
	buffer.addAttribute(8, Type::Double, 4, spk::VertexBuffer::Interpretation::Double);
	EXPECT_EQ(buffer.attributes().size(), 9u);
}

TEST(IndexBufferTest, ConfiguresEveryTypeAndSupportsTypedStorageOperations)
{
	spk::IndexBuffer buffer;
	EXPECT_FALSE(buffer.isConfigured());
	EXPECT_EQ(buffer.count(), 0u);
	buffer.setType(spk::IndexBuffer::Type::UnsignedShort);
	buffer.reserve<std::uint16_t>(8);
	buffer.pushBack<std::uint16_t>(0);
	buffer.insert<std::uint16_t>({1, std::numeric_limits<std::uint16_t>::max()});
	ASSERT_EQ(buffer.count(), 3u);
	EXPECT_EQ(buffer.cast<std::uint16_t>()[2], std::numeric_limits<std::uint16_t>::max());
	buffer.resize<std::uint16_t>(4);
	EXPECT_EQ(buffer.cast<std::uint16_t>()[3], 0u);
	buffer.clear();
	buffer.setType(spk::IndexBuffer::Type::UnsignedByte);
	EXPECT_EQ(buffer.stride(), sizeof(std::uint8_t));
	buffer.setType(spk::IndexBuffer::Type::UnsignedInt);
	EXPECT_EQ(buffer.stride(), sizeof(std::uint32_t));
}

TEST(IndexBufferTest, MissingWrongTypeLiveReconfigurationAndOverflowAreRejected)
{
	spk::IndexBuffer buffer;
	EXPECT_THROW(buffer.pushBack<std::uint32_t>(0), std::logic_error);
	buffer.setType(spk::IndexBuffer::Type::UnsignedInt);
	EXPECT_THROW(buffer.pushBack<std::uint16_t>(0), std::logic_error);
	buffer.pushBack<std::uint32_t>(0);
	EXPECT_THROW(buffer.setType(spk::IndexBuffer::Type::UnsignedShort), std::logic_error);
	EXPECT_THROW(buffer.clearConfiguration(), std::logic_error);
	buffer.clear();
	EXPECT_THROW(buffer.resize<std::uint32_t>(std::numeric_limits<std::size_t>::max()), std::overflow_error);
}

TEST(IndexBufferTest, EmptyConfigurationCanBeResetAndReused)
{
	spk::IndexBuffer buffer;
	buffer.setType(spk::IndexBuffer::Type::UnsignedByte);
	buffer.clearConfiguration();
	EXPECT_FALSE(buffer.isConfigured());
	EXPECT_EQ(buffer.stride(), 0u);
	buffer.setType(spk::IndexBuffer::Type::UnsignedShort);
	EXPECT_TRUE(buffer.isConfigured());
}

TEST(UniformBufferTest, CpuDataMutableCastGpuRetrievalAndBindingRoundTrip)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::UniformBuffer buffer(3, sizeof(UniformData));
	const UniformData initial{{1.0f, 2.0f, 3.0f, 4.0f}};
	buffer.setData(initial);
	buffer.cast<UniformData>().values[2] = 9.0f;
	buffer.validate();
	buffer.activate(openGL.renderContext());
	const auto retrieved = buffer.retrieve<UniformData>(openGL.renderContext());
	EXPECT_FLOAT_EQ(retrieved.values[2], 9.0f);
	GLint binding = 0;
	::glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 3, &binding);
	EXPECT_NE(binding, 0);
}

TEST(UniformBufferTest, DefaultZeroSizeAndWrongSizesHaveDefinedBehavior)
{
	spk::UniformBuffer empty;
	EXPECT_EQ(empty.bindingPoint(), 0u);
	EXPECT_EQ(empty.size(), 0u);
	EXPECT_NO_THROW(empty.setData(nullptr, 0));
	spk::UniformBuffer buffer(2, 4);
	EXPECT_THROW(buffer.setData(nullptr, 3), std::invalid_argument);
	EXPECT_THROW(buffer.setData(std::vector<std::byte>(5)), std::invalid_argument);
	EXPECT_THROW(buffer.setData(std::uint64_t{7}), std::logic_error);
	EXPECT_THROW((void)buffer.cast<std::uint64_t>(), std::logic_error);
}

TEST(ShaderStorageBufferTest, FixedAndDynamicCpuViewsGpuRetrievalAndBindingRoundTrip)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::ShaderStorageBuffer buffer(4, sizeof(FixedData), sizeof(std::uint32_t));
	buffer.resize(3);
	buffer.setFixedData(FixedData{3, 2.5f});
	const std::array<std::uint32_t, 3> values{10, 20, 30};
	buffer.setDynamicData<std::uint32_t>(values);
	auto view = buffer.cast<FixedData, std::uint32_t>();
	view.fixedPart.scale = 4.0f;
	view.dynamicArray[1] = 99;
	buffer.validate();
	buffer.activate(openGL.renderContext());
	const auto gpu = buffer.retrieve<FixedData, std::uint32_t>(openGL.renderContext());
	EXPECT_EQ(gpu.fixedPart.count, 3u);
	EXPECT_FLOAT_EQ(gpu.fixedPart.scale, 4.0f);
	EXPECT_EQ(gpu.dynamicArray, (std::vector<std::uint32_t>{10, 99, 30}));
	GLint binding = 0;
	::glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 4, &binding);
	EXPECT_NE(binding, 0);
}

TEST(ShaderStorageBufferTest, DynamicOnlyZeroAndMultipleElementCountsAreSupported)
{
	spk::ShaderStorageBuffer buffer(1, 0, sizeof(float));
	EXPECT_EQ(buffer.dynamicElementCount(), 0u);
	auto empty = buffer.cast<void, float>();
	EXPECT_EQ(empty.nbElement, 0u);
	buffer.resize(2);
	const std::array<float, 2> values{1.5f, 2.5f};
	buffer.setDynamicData<float>(values);
	const auto view = std::as_const(buffer).cast<void, float>();
	EXPECT_EQ(view.nbElement, 2u);
	EXPECT_FLOAT_EQ(view.dynamicArray[1], 2.5f);
}

TEST(ShaderStorageBufferTest, InvalidSizesTypesCountsAlignmentAndOverflowAreRejected)
{
	EXPECT_THROW(spk::ShaderStorageBuffer(0, 0, 0), std::invalid_argument);
	spk::ShaderStorageBuffer buffer(0, 1, sizeof(std::uint32_t));
	EXPECT_THROW(buffer.setFixedData(nullptr, 2), std::invalid_argument);
	EXPECT_THROW(buffer.setFixedData(std::uint32_t{1}), std::invalid_argument);
	buffer.resize(2);
	EXPECT_THROW(buffer.setDynamicData(nullptr, 1), std::invalid_argument);
	EXPECT_THROW(buffer.setDynamicData<std::uint16_t>(std::array<std::uint16_t, 2>{}), std::invalid_argument);
	EXPECT_THROW(((void)buffer.cast<std::uint32_t, std::uint32_t>()), std::logic_error);
	EXPECT_THROW(((void)buffer.cast<void, std::uint32_t>()), std::logic_error);
	EXPECT_THROW(buffer.resize(std::numeric_limits<std::size_t>::max()), std::overflow_error);
}

TEST(LayoutBufferTest, OwnsLinkedBuffersAndSupportsConstAccessAndRepeatedValidation)
{
	static_assert(!std::is_copy_constructible_v<spk::LayoutBuffer>);
	static_assert(!std::is_move_constructible_v<spk::LayoutBuffer>);
	spk::LayoutBuffer layout;
	configureVertexBuffer(layout.vertexBuffer());
	layout.indexBuffer().setType(spk::IndexBuffer::Type::UnsignedInt);
	layout.vertexBuffer().pushBack(Vertex{});
	layout.indexBuffer().pushBack<std::uint32_t>(0);
	EXPECT_NO_THROW(layout.validate());
	EXPECT_NO_THROW(layout.validate());
	const auto &constant = std::as_const(layout);
	EXPECT_EQ(constant.vertexBuffer().count(), 1u);
	EXPECT_EQ(constant.indexBuffer().count(), 1u);
}

TEST(LayoutBufferTest, ConfiguredLayoutActivatesVertexArrayAndBothGpuBuffers)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::LayoutBuffer layout;
	configureVertexBuffer(layout.vertexBuffer());
	layout.indexBuffer().setType(spk::IndexBuffer::Type::UnsignedInt);
	layout.vertexBuffer().pushBack(Vertex{});
	layout.indexBuffer().pushBack<std::uint32_t>(0);
	layout.validate();
	layout.activate(openGL.renderContext());

	GLint vertexArray = 0;
	GLint arrayBuffer = 0;
	GLint elementBuffer = 0;
	::glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertexArray);
	::glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
	::glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elementBuffer);
	EXPECT_NE(vertexArray, 0);
	EXPECT_NE(arrayBuffer, 0);
	EXPECT_NE(elementBuffer, 0);
}

TEST(BufferGPUResourceTest, UsageAndGpuRetrievalTrackCpuEditsAcrossGenerations)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::IndexBuffer buffer;
	buffer.setType(spk::IndexBuffer::Type::UnsignedInt);
	buffer.setUsage(spk::BufferGPUResource::Usage::DynamicDraw);
	buffer.insert<std::uint32_t>({1, 2, 3});
	buffer.validate();
	const auto first = buffer.retrieve(openGL.renderContext());
	EXPECT_EQ(first.size(), sizeof(std::uint32_t) * 3);
	buffer.cast<std::uint32_t>()[1] = 42;
	buffer.setUsage(spk::BufferGPUResource::Usage::StreamDraw);
	buffer.validate();
	const auto second = buffer.retrieve(openGL.renderContext());
	std::uint32_t middle = 0;
	std::memcpy(&middle, second.data() + sizeof(std::uint32_t), sizeof(middle));
	EXPECT_EQ(middle, 42u);
	EXPECT_EQ(buffer.usage(), spk::BufferGPUResource::Usage::StreamDraw);
}

TEST(BufferGPUResourceTest, DISABLED_OpenGLCreationFailureNeedsDriverInjectionSeam)
{
	GTEST_SKIP() << "The public buffer API has no injectable glGenBuffers failure seam.";
}
