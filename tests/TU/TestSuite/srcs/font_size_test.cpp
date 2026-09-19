#include <gtest/gtest.h>

#include <limits>
#include <map>
#include <type_traits>

#include "graphics/font.hpp"

namespace
{
	static_assert(std::is_trivially_copyable_v<spk::Font::Size>);
	static_assert(std::is_nothrow_copy_constructible_v<spk::Font::Size>);
	static_assert(std::is_nothrow_move_constructible_v<spk::Font::Size>);
	static_assert(spk::Font::Size{} == spk::Font::Size{0, 0});
	static_assert(spk::Font::Size{16} == spk::Font::Size{16, 0});
	static_assert(spk::Font::Size{16, 1} < spk::Font::Size{17, 0});
}

TEST(FontSizeTest, ConstructorsExposeGlyphAndOutlineValues)
{
	const spk::Font::Size defaultSize;
	const spk::Font::Size glyphOnly{18};
	const spk::Font::Size outlined{24, 3};
	const spk::Font::Size boundary{
		std::numeric_limits<std::size_t>::max(),
		std::numeric_limits<std::size_t>::max()};

	EXPECT_EQ(defaultSize.glyph, 0u);
	EXPECT_EQ(defaultSize.outline, 0u);
	EXPECT_EQ(glyphOnly.glyph, 18u);
	EXPECT_EQ(glyphOnly.outline, 0u);
	EXPECT_EQ(outlined.glyph, 24u);
	EXPECT_EQ(outlined.outline, 3u);
	EXPECT_EQ(boundary.glyph, std::numeric_limits<std::size_t>::max());
	EXPECT_EQ(boundary.outline, std::numeric_limits<std::size_t>::max());
}

TEST(FontSizeTest, EqualityAndOrderingUseGlyphThenOutline)
{
	const spk::Font::Size smallOutline{12, 1};
	const spk::Font::Size largeOutline{12, 2};
	const spk::Font::Size largerGlyph{13, 0};

	EXPECT_EQ(smallOutline, (spk::Font::Size{12, 1}));
	EXPECT_NE(smallOutline, largeOutline);
	EXPECT_LT(smallOutline, largeOutline);
	EXPECT_LT(largeOutline, largerGlyph);
	EXPECT_FALSE(largerGlyph < smallOutline);
	EXPECT_FALSE(smallOutline < smallOutline);

	std::map<spk::Font::Size, int> values;
	values.emplace(largerGlyph, 3);
	values.emplace(smallOutline, 1);
	values.emplace(largeOutline, 2);
	auto iterator = values.begin();
	EXPECT_EQ(iterator++->second, 1);
	EXPECT_EQ(iterator++->second, 2);
	EXPECT_EQ(iterator->second, 3);
}

TEST(FontSizeTest, CopyAndMovePreserveBothComponents)
{
	spk::Font::Size source{32, 4};
	spk::Font::Size copy = source;
	spk::Font::Size moved = std::move(source);

	EXPECT_EQ(copy, (spk::Font::Size{32, 4}));
	EXPECT_EQ(moved, (spk::Font::Size{32, 4}));

	copy = {9, 2};
	moved = std::move(copy);
	EXPECT_EQ(moved, (spk::Font::Size{9, 2}));
}
