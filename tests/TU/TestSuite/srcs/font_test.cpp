#include <gtest/gtest.h>

#include <Windows.h>
#include <fstream>
#include <type_traits>

#include "graphics/font.hpp"
#include "sparkle_test.hpp"

namespace
{
	static_assert(!std::is_copy_constructible_v<spk::Font::Atlas>);
	static_assert(!std::is_move_constructible_v<spk::Font::Atlas>);
	static_assert(std::is_move_constructible_v<spk::Font>);
	spk::Font::Data fontBytes()
	{
		const auto path = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path().parent_path() / "resources/fonts/arial.ttf";
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			throw std::runtime_error("Cannot open bundled font fixture: " + path.string());
		}
		return spk::Font::Data(std::istreambuf_iterator<char>(file), {});
	}

	template <typename Callback>
	void expectFontError(Callback callback, const std::string &message)
	{
		try
		{
			callback();
			FAIL() << "Expected runtime_error";
		} catch (const std::runtime_error &error)
		{
			EXPECT_EQ(error.what(), message);
		}
	}
}

TEST(FontTest, EmbeddedAndFileFontsHaveRepeatableMetricsAndIndependentAtlases)
{
	sparkle_test::TemporaryDirectory directory;
	const auto bytes = fontBytes();
	const auto path = directory.file("fixture.ttf");
	{
		std::ofstream file(path, std::ios::binary);
		file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
	}
	spk::Font fileFont(path);
	auto memoryFont = spk::Font::fromRawData(bytes);
	for (const spk::Font::Size size : {spk::Font::Size{16}, {32}, {24, 2}})
	{
		auto &atlas = memoryFont.atlas(size);
		EXPECT_EQ(&atlas, &memoryFont.atlas(size));
		EXPECT_NE(&atlas, &fileFont.atlas(size));
		for (const auto text : {U"AV", U"Hello", U"gyp", U"\u00e9\u03a9", U""})
		{
			EXPECT_EQ(memoryFont.computeStringSize(text, size), fileFont.computeStringSize(text, size));
			EXPECT_EQ(memoryFont.computeStringBaselineOffset(text, size), fileFont.computeStringBaselineOffset(text, size));
		}
		EXPECT_EQ(memoryFont.computeStringSize(U"", size), spk::Vector2UInt(0, 0));
		EXPECT_GT(memoryFont.computeStringSize(U"Hello", size).y, 0u);
		// The public measurement API uses advances, with no pair-kerning or line-height API.
		EXPECT_GE(memoryFont.computeStringSize(U"AV", size).x, static_cast<unsigned>(atlas[U'A'].step.x + atlas[U'V'].step.x));
	}
}

TEST(FontTest, MissingEmptyMalformedAndLockedFilesReportExactDiagnostics)
{
	sparkle_test::TemporaryDirectory directory;
	const auto missing = directory.file("missing.ttf");
	expectFontError([&] {
		spk::Font font(missing);
	},
					"Font: failed to open file: " + missing.string());
	directory.write("empty.ttf", "");
	expectFontError([&] {
		spk::Font font(directory.file("empty.ttf"));
	},
					"Font can't be initialized from empty data");
	expectFontError([] {
		(void)spk::Font::fromRawData({});
	},
					"Font can't be initialized from empty data");
	// A full invalid header avoids making the unbounded STB reader inspect a truncated allocation.
	directory.write("invalid.ttf", std::string(256, '\0'));
	expectFontError([&] {
		spk::Font font(directory.file("invalid.ttf"));
	},
					"Font data is not a valid TrueType font");
	expectFontError([] {
		(void)spk::Font::fromRawData(spk::Font::Data(256, 0));
	},
					"Font data is not a valid TrueType font");
	const auto locked = directory.file("locked.ttf");
	directory.write("locked.ttf", std::string(256, 'x'));
	const HANDLE handle = CreateFileW(locked.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	ASSERT_NE(handle, INVALID_HANDLE_VALUE);
	expectFontError([&] {
		spk::Font font(locked);
	},
					"Font: failed to open file: " + locked.string());
	EXPECT_TRUE(CloseHandle(handle));
	spk::Font empty;
	EXPECT_THROW((void)empty.atlas({16}), std::logic_error);
}

TEST(FontAtlasTest, GlyphMetricsCachingUnicodeAndMissingGlyphsAreDeterministic)
{
	auto font = spk::Font::fromRawData(fontBytes());
	auto other = spk::Font::fromRawData(fontBytes());
	auto &atlas = font.atlas({32});
	int editions = 0;
	auto contract = atlas.subscribe([&] {
		++editions;
	});
	const auto &glyph = atlas.glyph(U'A');
	EXPECT_EQ(glyph.step, spk::Vector2Int(22, 0));
	EXPECT_EQ(glyph.size, spk::Vector2UInt(23, 23));
	EXPECT_EQ(glyph.baselineOffset, spk::Vector2Int(1, 23));
	EXPECT_GT(glyph.step.x, 0);
	EXPECT_EQ(glyph.step.y, 0);
	EXPECT_GT(glyph.size.x, 0u);
	EXPECT_GT(glyph.size.y, 0u);
	EXPECT_EQ(glyph.positions[3] - glyph.positions[0], static_cast<spk::Vector2Int>(glyph.size));
	EXPECT_EQ(glyph.baselineOffset, spk::Vector2Int(0, 0) - glyph.positions[0]);
	EXPECT_EQ(&glyph, &atlas[U'A']);
	EXPECT_EQ(editions, 1);
	atlas.loadGlyphs("A\xc3\xa9");
	atlas.loadGlyphs(U"A\u00e9");
	EXPECT_EQ(editions, 2);
	EXPECT_EQ(font.computeStringSize("A\xc3\xa9", {32}), font.computeStringSize(U"A\u00e9", {32}));
	EXPECT_EQ(atlas[U' '].size, spk::Vector2UInt(0, 0));
	EXPECT_EQ(atlas[U' '].step, spk::Vector2Int(16, 0));
	// Unsupported codepoints use the font's .notdef glyph, identically to codepoint zero.
	EXPECT_EQ(atlas[0x10ffff].size, atlas[0].size);
	EXPECT_EQ(atlas[0x10ffff].step, atlas[0].step);
	other.atlas({32}).loadGlyphs(U"A\u00e9\U0010ffff\0");
	EXPECT_EQ(atlas[U'A'].uvs, other.atlas({32})[U'A'].uvs);
	EXPECT_EQ(atlas[U'\u00e9'].uvs, other.atlas({32})[U'\u00e9'].uvs);
}

TEST(FontAtlasTest, GrowthPreservesExistingPixelCoordinatesAndNotifiesOncePerBatch)
{
	auto font = spk::Font::fromRawData(fontBytes());
	auto &atlas = font.atlas({48, 2});
	const auto glyph = atlas[U'A'];
	const auto beforeSize = atlas.size();
	const auto beforePixels = atlas.pixels();
	int editions = 0;
	auto contract = atlas.subscribe([&] {
		++editions;
	});
	atlas.loadAllRenderableGlyphs();
	ASSERT_GT(atlas.size().x, beforeSize.x);
	EXPECT_EQ(editions, 1);
	EXPECT_EQ(atlas[U'A'].size, glyph.size);
	EXPECT_EQ(atlas[U'A'].positions, glyph.positions);
	for (std::size_t i = 0; i < 4; ++i)
	{
		EXPECT_NEAR(atlas[U'A'].uvs[i].x * atlas.size().x, glyph.uvs[i].x * beforeSize.x, 0.001f);
		EXPECT_NEAR(atlas[U'A'].uvs[i].y * atlas.size().y, glyph.uvs[i].y * beforeSize.y, 0.001f);
	}
	for (unsigned y = 0; y < beforeSize.y; ++y)
	{
		for (unsigned x = 0; x < beforeSize.x; ++x)
		{
			if (beforePixels[y * beforeSize.x + x] != 0)
			{
				EXPECT_EQ(atlas.pixels()[y * atlas.size().x + x], beforePixels[y * beforeSize.x + x]);
			}
		}
	}
	atlas.loadAllRenderableGlyphs();
	EXPECT_EQ(editions, 1);
}

TEST(FontTest, MovesPreserveAtlasAddressesSubscriptionsAndGpuRealization)
{
	auto font = spk::Font::fromRawData(fontBytes());
	auto *atlas = &font.atlas({20});
	const auto *glyph = &atlas->glyph(U'A');
	int editions = 0;
	auto contract = atlas->subscribe([&] {
		++editions;
	});
	auto moved = std::move(font);
	EXPECT_EQ(&moved.atlas({20}), atlas);
	EXPECT_EQ(&atlas->glyph(U'A'), glyph);
	EXPECT_THROW((void)font.atlas({20}), std::logic_error);
	spk::Font assigned;
	assigned = std::move(moved);
	EXPECT_EQ(&assigned.atlas({20}), atlas);
	atlas->loadGlyphs(U"B");
	EXPECT_EQ(editions, 1);
	auto &context = sparkle_test::OpenGLTestContext::instance();
	context.makeCurrent();
	EXPECT_NO_THROW(atlas->activate(context.renderContext()));
	assigned = spk::Font{};
	EXPECT_NO_THROW(context.surface()._gpuResources().reclaimReleased());
}
