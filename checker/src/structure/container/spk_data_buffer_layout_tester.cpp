#include "structure/container/spk_data_buffer_layout_tester.hpp"

namespace
{
	constexpr size_t positionOffset = offsetof(DataBufferLayoutTest::Vertex, position);
	constexpr size_t colorOffset = offsetof(DataBufferLayoutTest::Vertex, colors);
}

TEST_F(DataBufferLayoutTest, RootReflectsBufferSize)
{
	EXPECT_EQ(layout.root().size(), buffer.size()) << "Root element should cover the entire buffer";
	EXPECT_EQ(layout.root().buffer(), &buffer) << "Root element should reference the current buffer";
}

TEST_F(DataBufferLayoutTest, AddElementAndAssignValue)
{
	auto &positionElement = layout.addElement(L"position", positionOffset, sizeof(std::array<float, 3>));
	EXPECT_EQ(positionElement.isArray(), false) << "Position element should not be flagged as an array";
	EXPECT_EQ(layout.contains(L"position"), true) << "Layout should report that position is contained";

	std::array<float, 3> expected = {1.0F, 2.0F, 3.0F};
	positionElement = expected;
	const auto &stored = positionElement.get<std::array<float, 3>>();
	for (size_t index = 0; index < expected.size(); ++index)
	{
		EXPECT_FLOAT_EQ(stored[index], expected[index]) << "Stored component " << index << " should match";
	}

	auto &asArray = positionElement.as<std::array<float, 3>>();
	for (size_t index = 0; index < expected.size(); ++index)
	{
		EXPECT_FLOAT_EQ(asArray[index], expected[index]) << "Accessing through as<T> should expose buffer data";
	}
}

TEST_F(DataBufferLayoutTest, AddArrayElementAndRetrieve)
{
	auto &colorElement = layout.addElement(L"colors", colorOffset, 2U, sizeof(uint32_t), 0U);
	EXPECT_EQ(colorElement.isArray(), true) << "colors should be registered as an array";
	EXPECT_EQ(colorElement.nbElement(), 2U) << "Array should report the correct number of elements";

	colorElement[0U] = static_cast<uint32_t>(0x11223344U);
	colorElement[1U] = static_cast<uint32_t>(0x55667788U);

	const auto stored = colorElement.getArray<uint32_t>();
	EXPECT_EQ(stored.size(), 2U) << "Retrieved array should have two entries";
	EXPECT_EQ(stored[0U], 0x11223344U) << "First array entry should match the assigned value";
	EXPECT_EQ(stored[1U], 0x55667788U) << "Second array entry should match the assigned value";
}

TEST_F(DataBufferLayoutTest, ContainsAndRemoveElement)
{
	layout.addElement(L"position", positionOffset, sizeof(std::array<float, 3>));
	EXPECT_EQ(layout.contains(L"position"), true) << "Layout should report existing elements";

	layout.removeElement(L"position");
	EXPECT_EQ(layout.contains(L"position"), false) << "Removed element should no longer be reported";
	EXPECT_THROW(layout[L"position"], std::runtime_error) << "Accessing a removed element should throw";
}

TEST_F(DataBufferLayoutTest, DuplicateKeepsMetadata)
{
	auto &positionElement = layout.addElement(L"position", positionOffset, sizeof(std::array<float, 3>));
	auto duplicate = positionElement.duplicate(16U);
	EXPECT_EQ(duplicate.size(), positionElement.size()) << "Duplicated element should keep the same size";
	EXPECT_EQ(duplicate.buffer(), positionElement.buffer()) << "Duplicated element should point to the same buffer";
	EXPECT_EQ(duplicate.offset(), 16U) << "Duplicated element should apply the new offset";
}

TEST_F(DataBufferLayoutTest, SetBufferPropagatesToChildren)
{
	auto &positionElement = layout.addElement(L"position", positionOffset, sizeof(std::array<float, 3>));
	auto &colorsElement = layout.addElement(L"colors", colorOffset, 2U, sizeof(uint32_t), 0U);

	spk::DataBuffer otherBuffer(sizeof(Vertex));
	layout.setBuffer(&otherBuffer);

	EXPECT_EQ(positionElement.buffer(), &otherBuffer) << "setBuffer should update child elements";
	EXPECT_EQ(colorsElement.buffer(), &otherBuffer) << "setBuffer should update nested arrays";
}

TEST_F(DataBufferLayoutTest, ResizeArrayRebuildsChildren)
{
	auto &colorsElement = layout.addElement(L"colors", colorOffset, 2U, sizeof(uint32_t), 0U);
	buffer.resize(sizeof(Vertex) + sizeof(uint32_t));
	layout.updateRootSize();

	colorsElement.resize(3U, sizeof(uint32_t), 0U);
	EXPECT_EQ(colorsElement.nbElement(), 3U) << "Resizing should update the number of children";
	EXPECT_EQ(colorsElement.size(), 3U * sizeof(uint32_t)) << "Resized array should report the expected size in bytes";
}

TEST_F(DataBufferLayoutTest, LayoutAsProvidesStructuredView)
{
	layout.addElement(L"position", positionOffset, sizeof(std::array<float, 3>));
	layout.addElement(L"colors", colorOffset, 2U, sizeof(uint32_t), 0U);

	auto *vertexPtr = layout.as<Vertex>();
	vertexPtr->position = {9.0F, 8.0F, 7.0F};
	vertexPtr->colors[0] = 11U;
	vertexPtr->colors[1] = 22U;

	const auto &positionElement = layout[L"position"].get<std::array<float, 3>>();
	EXPECT_FLOAT_EQ(positionElement[0], 9.0F) << "Structured view should modify the buffer position x";
	EXPECT_FLOAT_EQ(positionElement[1], 8.0F) << "Structured view should modify the buffer position y";
	EXPECT_FLOAT_EQ(positionElement[2], 7.0F) << "Structured view should modify the buffer position z";

	const auto colors = layout[L"colors"].getArray<uint32_t>();
	EXPECT_EQ(colors[0], 11U) << "Structured view should modify the first color";
	EXPECT_EQ(colors[1], 22U) << "Structured view should modify the second color";
}

TEST_F(DataBufferLayoutTest, ElementAccessorsValidateTypes)
{
	auto &positionElement = layout.addElement(L"position", positionOffset, sizeof(std::array<float, 3>));
	positionElement = std::array<float, 3>{1.0F, 2.0F, 3.0F};

	EXPECT_THROW(positionElement.get<uint32_t>(), std::runtime_error) << "Requesting a mismatched type should trigger an exception";
	EXPECT_THROW(positionElement.getArray<uint32_t>(), std::runtime_error)
		<< "Requesting an array from a non-array element should trigger an exception";
}

TEST_F(DataBufferLayoutTest, OperatorIndexingOnArraysAndStructures)
{
	auto &colorsElement = layout.addElement(L"colors", colorOffset, 2U, sizeof(uint32_t), 0U);
	colorsElement[0U] = 5U;
	colorsElement[1U] = 6U;
	EXPECT_EQ(colorsElement[0U].get<uint32_t>(), 5U) << "Indexing should return the first element";
	EXPECT_EQ(colorsElement[1U].get<uint32_t>(), 6U) << "Indexing should return the second element";
	EXPECT_THROW(colorsElement[2U], std::runtime_error) << "Out of bounds access should throw";

	layout.addElement(L"position", positionOffset, sizeof(std::array<float, 3>));
	EXPECT_EQ(layout[L"position"].size(), sizeof(std::array<float, 3>)) << "Structure indexing should access named children";
	EXPECT_THROW(layout[L"missing"], std::runtime_error) << "Missing keys should throw when accessed";
}
