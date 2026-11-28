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
	spk::Buffer::Mutable makeMutable(std::size_t size = 0)
	{
		spk::Buffer::Builder builder;
		builder.resize(size);
		return std::move(builder).buildMutable();
	}

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

	class DerivedShaderStorageBuffer : public spk::ShaderStorageBuffer
	{
	public:
		struct Data
		{
			std::uint32_t value;
		};
		using DataView = spk::ShaderStorageBuffer::View<void, Data>;

	private:
		std::shared_ptr<spk::Buffer::Mutable> _cpu;
		DataView _view{};

	public:
		explicit DerivedShaderStorageBuffer(std::size_t nbElement) :
			ShaderStorageBuffer(0, 0, sizeof(Data)),
			_cpu(std::make_shared<spk::Buffer::Mutable>(makeMutable(nbElement * sizeof(Data))))
		{
			bind(_cpu.get());
			_view = {nbElement, _cpu->cast<Data>().data()};
		}

		[[nodiscard]] DataView &view() noexcept
		{
			return _view;
		}
		[[nodiscard]] const DataView &view() const noexcept
		{
			return _view;
		}
	};

	class DerivedUniformBuffer : public spk::UniformBuffer
	{
	public:
		struct Data
		{
			float value;
		};

	private:
		std::shared_ptr<spk::Buffer::Mutable> _cpu;
		Data *_view = nullptr;

	public:
		DerivedUniformBuffer() :
			UniformBuffer(0, sizeof(Data)),
			_cpu(std::make_shared<spk::Buffer::Mutable>(makeMutable(sizeof(Data)))),
			_view(_cpu->cast<Data>().data())
		{
			bind(_cpu.get());
		}

		[[nodiscard]] Data &view() noexcept
		{
			return *_view;
		}
		[[nodiscard]] const Data &view() const noexcept
		{
			return *_view;
		}
	};
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
	auto cpu = makeMutable();
	buffer.bind(&cpu);
	cpu.reserve<Vertex>(8);
	cpu.pushBack(Vertex{1.0f, 2.0f, 0x01020304u});
	const std::array<Vertex, 2> extra{{{3.0f, 4.0f, 5u}, {6.0f, 7.0f, 8u}}};
	cpu.insert(std::span<const Vertex>(extra));
	ASSERT_EQ(buffer.count(), 3u);
	EXPECT_FLOAT_EQ(buffer.cast<Vertex>()[1].x, 3.0f);
	cpu.cast<Vertex>()[2].y = 9.0f;
	EXPECT_FLOAT_EQ(std::as_const(buffer).cast<Vertex>()[2].y, 9.0f);

	cpu.resize<Vertex>(5);
	EXPECT_EQ(buffer.count(), 5u);
	EXPECT_EQ(buffer.cast<Vertex>()[4].color, 0u);
	cpu.clear();
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
	auto cpu = makeMutable(sizeof(Vertex));
	buffer.bind(&cpu);
	EXPECT_THROW(buffer.addPadding(1), std::logic_error);
	EXPECT_THROW(buffer.clearConfiguration(), std::logic_error);
}

TEST(VertexBufferTest, MissingOrWrongTypedLayoutAndSizeOverflowAreRejected)
{
	spk::VertexBuffer buffer;
	auto cpu = makeMutable(sizeof(Vertex));
	EXPECT_THROW(buffer.bind(&cpu), std::logic_error);
	buffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 1);
	buffer.bind(&cpu);
	EXPECT_THROW((void)buffer.cast<Vertex>(), std::logic_error);
	EXPECT_THROW(cpu.resize<float>(std::numeric_limits<std::size_t>::max()), spk::Exception);
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
	auto cpu = makeMutable();
	buffer.bind(&cpu);
	cpu.reserve<std::uint16_t>(8);
	cpu.pushBack<std::uint16_t>(0);
	const std::array<std::uint16_t, 2> extra{1, std::numeric_limits<std::uint16_t>::max()};
	cpu.insert(std::span<const std::uint16_t>(extra));
	ASSERT_EQ(buffer.count(), 3u);
	EXPECT_EQ(buffer.cast<std::uint16_t>()[2], std::numeric_limits<std::uint16_t>::max());
	cpu.resize<std::uint16_t>(4);
	EXPECT_EQ(buffer.cast<std::uint16_t>()[3], 0u);
	cpu.clear();
	buffer.setType(spk::IndexBuffer::Type::UnsignedByte);
	EXPECT_EQ(buffer.stride(), sizeof(std::uint8_t));
	buffer.setType(spk::IndexBuffer::Type::UnsignedInt);
	EXPECT_EQ(buffer.stride(), sizeof(std::uint32_t));
}

TEST(IndexBufferTest, MissingWrongTypeLiveReconfigurationAndOverflowAreRejected)
{
	spk::IndexBuffer buffer;
	auto cpu = makeMutable(sizeof(std::uint32_t));
	EXPECT_THROW(buffer.bind(&cpu), std::logic_error);
	buffer.setType(spk::IndexBuffer::Type::UnsignedInt);
	cpu.resize<std::uint16_t>(1);
	EXPECT_THROW(buffer.bind(&cpu), std::logic_error);
	cpu.resize<std::uint32_t>(1);
	buffer.bind(&cpu);
	EXPECT_THROW(buffer.setType(spk::IndexBuffer::Type::UnsignedShort), std::logic_error);
	EXPECT_THROW(buffer.clearConfiguration(), std::logic_error);
	cpu.clear();
	EXPECT_THROW(cpu.resize<std::uint32_t>(std::numeric_limits<std::size_t>::max()), spk::Exception);
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
	auto cpu = makeMutable(sizeof(UniformData));
	cpu.cast<UniformData>()[0] = initial;
	cpu.cast<UniformData>()[0].values[2] = 9.0f;
	buffer.bind(&cpu);
	cpu.validate();
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
	auto emptyCpu = makeMutable();
	EXPECT_NO_THROW(empty.bind(&emptyCpu));
	spk::UniformBuffer buffer(2, 4);
	auto cpu = makeMutable(3);
	EXPECT_THROW(buffer.bind(&cpu), std::invalid_argument);
	cpu.resize(5);
	EXPECT_THROW(buffer.bind(&cpu), std::invalid_argument);
	cpu.resize<std::uint64_t>(1);
	EXPECT_THROW(buffer.bind(&cpu), std::invalid_argument);
	cpu.resize(4);
	buffer.bind(&cpu);
	EXPECT_THROW((void)buffer.cast<std::uint64_t>(), std::logic_error);
}

TEST(UniformBufferTest, DerivedResourceRetainsTypedViewAndSharesStateWhenCopied)
{
	DerivedUniformBuffer source;
	source.view().value = 12.0f;

	auto copy = source;
	DerivedUniformBuffer assigned;
	assigned = source;
	EXPECT_EQ(copy.identifier(), source.identifier());
	EXPECT_EQ(assigned.identifier(), source.identifier());
	EXPECT_EQ(&copy.view(), &source.view());
	EXPECT_EQ(&assigned.view(), &source.view());

	copy.view().value = 42.0f;
	EXPECT_FLOAT_EQ(source.view().value, 42.0f);
	EXPECT_FLOAT_EQ(assigned.view().value, 42.0f);
}

TEST(ShaderStorageBufferTest, FixedAndDynamicCpuViewsGpuRetrievalAndBindingRoundTrip)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::ShaderStorageBuffer buffer(4, sizeof(FixedData), sizeof(std::uint32_t));
	spk::Buffer::Builder builder;
	builder.pushBack(FixedData{3, 2.5f});
	const std::array<std::uint32_t, 3> values{10, 20, 30};
	builder.insert(std::span<const std::uint32_t>(values));
	auto cpu = std::move(builder).buildMutable();
	buffer.bind(&cpu);
	reinterpret_cast<FixedData *>(cpu.data())->scale = 4.0f;
	reinterpret_cast<std::uint32_t *>(cpu.data() + sizeof(FixedData))[1] = 99;
	cpu.validate();
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
	auto cpu = makeMutable();
	buffer.bind(&cpu);
	auto empty = buffer.cast<void, float>();
	EXPECT_EQ(empty.nbElement, 0u);
	const std::array<float, 2> values{1.5f, 2.5f};
	cpu.insert(std::span<const float>(values));
	const auto view = std::as_const(buffer).cast<void, float>();
	EXPECT_EQ(view.nbElement, 2u);
	EXPECT_FLOAT_EQ(view.dynamicArray[1], 2.5f);
}

TEST(ShaderStorageBufferTest, DerivedResourceRetainsTypedViewAndSharesStateWhenCopied)
{
	DerivedShaderStorageBuffer source(2);
	ASSERT_EQ(source.view().nbElement, 2u);
	source.view().dynamicArray[0].value = 10;
	source.view().dynamicArray[1].value = 20;

	auto copy = source;
	DerivedShaderStorageBuffer assigned(1);
	assigned = source;
	EXPECT_EQ(copy.identifier(), source.identifier());
	EXPECT_EQ(assigned.identifier(), source.identifier());
	EXPECT_EQ(copy.view().dynamicArray, source.view().dynamicArray);
	EXPECT_EQ(assigned.view().dynamicArray, source.view().dynamicArray);

	copy.view().dynamicArray[1].value = 99;
	EXPECT_EQ(source.view().dynamicArray[1].value, 99u);
	EXPECT_EQ(assigned.view().dynamicArray[1].value, 99u);
}

TEST(ShaderStorageBufferTest, InvalidSizesTypesCountsAlignmentAndOverflowAreRejected)
{
	EXPECT_THROW(spk::ShaderStorageBuffer(0, 0, 0), std::invalid_argument);
	spk::ShaderStorageBuffer buffer(0, 1, sizeof(std::uint32_t));
	auto cpu = makeMutable(2);
	EXPECT_THROW(buffer.bind(&cpu), std::invalid_argument);
	cpu.resize(4);
	EXPECT_THROW(buffer.bind(&cpu), std::invalid_argument);
	cpu.resize(9);
	buffer.bind(&cpu);
	EXPECT_THROW(((void)buffer.cast<std::uint32_t, std::uint32_t>()), std::logic_error);
	EXPECT_THROW(((void)buffer.cast<void, std::uint32_t>()), std::logic_error);
	EXPECT_THROW(((void)buffer.cast<std::uint8_t, std::uint32_t>()), std::logic_error);
	EXPECT_THROW(cpu.resize<std::uint32_t>(std::numeric_limits<std::size_t>::max()), spk::Exception);
}

TEST(LayoutBufferTest, OwnsLinkedBuffersAndSupportsConstAccessAndRepeatedValidation)
{
	static_assert(!std::is_copy_constructible_v<spk::LayoutBuffer>);
	static_assert(!std::is_move_constructible_v<spk::LayoutBuffer>);
	spk::LayoutBuffer layout;
	configureVertexBuffer(layout.vertexBuffer());
	layout.indexBuffer().setType(spk::IndexBuffer::Type::UnsignedInt);
	auto vertices = makeMutable(sizeof(Vertex));
	auto indices = makeMutable(sizeof(std::uint32_t));
	layout.vertexBuffer().bind(&vertices);
	layout.indexBuffer().bind(&indices);
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
	auto vertices = makeMutable(sizeof(Vertex));
	auto indices = makeMutable(sizeof(std::uint32_t));
	layout.vertexBuffer().bind(&vertices);
	layout.indexBuffer().bind(&indices);
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
	spk::Buffer::Builder builder;
	const std::array<std::uint32_t, 3> values{1, 2, 3};
	builder.insert(std::span<const std::uint32_t>(values));
	auto cpu = std::move(builder).buildMutable();
	buffer.bind(&cpu);
	const auto first = buffer.retrieve(openGL.renderContext());
	EXPECT_EQ(first.size(), sizeof(std::uint32_t) * 3);
	cpu.cast<std::uint32_t>()[1] = 42;
	cpu.validate();
	buffer.setUsage(spk::BufferGPUResource::Usage::StreamDraw);
	buffer.validate();
	const auto second = buffer.retrieve(openGL.renderContext());
	std::uint32_t middle = 0;
	std::memcpy(&middle, second.data() + sizeof(std::uint32_t), sizeof(middle));
	EXPECT_EQ(middle, 42u);
	EXPECT_EQ(buffer.usage(), spk::BufferGPUResource::Usage::StreamDraw);
}

TEST(BufferGPUResourceTest, ImmutableRebindingUploadsNewDataWithSameVersion)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Buffer::Builder firstBuilder;
	firstBuilder.pushBack(std::uint32_t{11});
	auto first = std::move(firstBuilder).build();
	spk::Buffer::Builder secondBuilder;
	secondBuilder.pushBack(std::uint32_t{22});
	auto second = std::move(secondBuilder).build();
	spk::UniformBuffer gpu;
	gpu.bind(&first);
	EXPECT_EQ(gpu.retrieve<std::uint32_t>(openGL.renderContext()), 11u);
	gpu.bind(&second);
	EXPECT_EQ(gpu.retrieve<std::uint32_t>(openGL.renderContext()), 22u);
}

TEST(BufferGPUResourceTest, MutableCpuBufferUpdatesUploadThroughExistingHandle)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Buffer::Builder builder;
	builder.pushBack(std::uint32_t{11});
	auto cpu = std::move(builder).buildMutable();
	spk::UniformBuffer gpu;
	gpu.bind(&cpu);
	auto handle = gpu.handle();
	EXPECT_EQ(gpu.retrieve<std::uint32_t>(openGL.renderContext()), 11u);
	cpu.cast<std::uint32_t>()[0] = 77;
	cpu.validate();
	handle.activate(openGL.renderContext());
	std::uint32_t result = 0;
	glGetBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(result), &result);
	EXPECT_EQ(result, 77u);
}

TEST(LayoutBufferTest, MutableCpuEditsUploadWithoutReconfiguringVertexArray)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::Buffer::Builder vertexBuilder;
	vertexBuilder.pushBack(Vertex{1.0f, 2.0f, 3u});
	auto vertices = std::move(vertexBuilder).buildMutable();
	spk::Buffer::Builder indexBuilder;
	indexBuilder.pushBack(std::uint32_t{0});
	auto indices = std::move(indexBuilder).build();
	spk::LayoutBuffer layout;
	configureVertexBuffer(layout.vertexBuffer());
	layout.indexBuffer().setType(spk::IndexBuffer::Type::UnsignedInt);
	layout.vertexBuffer().bind(&vertices);
	layout.indexBuffer().bind(&indices);
	layout.validate();
	layout.activate(openGL.renderContext());
	vertices.cast<Vertex>()[0].color = 42u;
	vertices.validate();
	layout.activate(openGL.renderContext());
	Vertex actual{};
	glGetBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(actual), &actual);
	EXPECT_EQ(actual.color, 42u);
}
