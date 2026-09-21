#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <string>

#include "type/uuid.hpp"

namespace
{
	TEST(UUIDTest, StandardUsageGeneratesStringifiesParsesInspectsAndRoundTrips)
	{
		const spk::UUID generated = spk::UUID::generate();
		const std::string text = generated.toString();
		const spk::UUID parsed = spk::UUID::fromString(text);

		EXPECT_FALSE(generated.isNull());
		EXPECT_EQ(generated.version(), 4u);
		EXPECT_TRUE(generated.hasRFCVariant());
		EXPECT_EQ(text.size(), 36u);
		EXPECT_EQ(text[8], '-');
		EXPECT_EQ(text[13], '-');
		EXPECT_EQ(text[18], '-');
		EXPECT_EQ(text[23], '-');
		EXPECT_EQ(parsed, generated);
		ASSERT_TRUE(spk::UUID::tryParse(text).has_value());
		EXPECT_EQ(*spk::UUID::tryParse(text), generated);
	}

	TEST(UUIDTest, NullUUIDContainsOnlyZeroBytes)
	{
		const spk::UUID null = spk::UUID::null();

		EXPECT_TRUE(null.isNull());
		EXPECT_TRUE(std::all_of(null.bytes().begin(), null.bytes().end(), [](std::uint8_t byte) { return byte == 0u; }));
		EXPECT_EQ(null.version(), 0u);
		EXPECT_FALSE(null.hasRFCVariant());
		EXPECT_EQ(null.toString(), "00000000-0000-0000-0000-000000000000");
	}

	TEST(UUIDTest, ExplicitStorageIsExposedWithoutModification)
	{
		const spk::UUID::Storage storage{
			0x00u, 0x11u, 0x22u, 0x33u,
			0x44u, 0x55u,
			0x46u, 0x77u,
			0x88u, 0x99u,
			0xAAu, 0xBBu, 0xCCu, 0xDDu, 0xEEu, 0xFFu};
		const spk::UUID value(storage);

		EXPECT_EQ(value.bytes(), storage);
		EXPECT_EQ(value.version(), 4u);
		EXPECT_TRUE(value.hasRFCVariant());
		EXPECT_EQ(value.toString(), "00112233-4455-4677-8899-aabbccddeeff");
	}

	TEST(UUIDTest, MaximumByteStorageReportsVersionAndVariantFromRawBits)
	{
		spk::UUID::Storage storage{};
		storage.fill(0xFFu);
		const spk::UUID value(storage);

		EXPECT_FALSE(value.isNull());
		EXPECT_EQ(value.version(), 0x0Fu);
		EXPECT_FALSE(value.hasRFCVariant());
		EXPECT_EQ(value.toString(), "ffffffff-ffff-ffff-ffff-ffffffffffff");
	}

	TEST(UUIDTest, GeneratedSampleContainsNoDuplicates)
	{
		std::set<std::string> values;
		constexpr std::size_t SampleSize = 128;

		for (std::size_t index = 0; index < SampleSize; ++index)
		{
			values.insert(spk::UUID::generate().toString());
		}

		EXPECT_EQ(values.size(), SampleSize);
	}

	TEST(UUIDTest, ParsingAcceptsUppercaseAndStringificationUsesCanonicalLowercase)
	{
		const std::string uppercase = "00112233-4455-4677-8899-AABBCCDDEEFF";
		const spk::UUID value = spk::UUID::fromString(uppercase);

		EXPECT_EQ(value.toString(), "00112233-4455-4677-8899-aabbccddeeff");
	}

	TEST(UUIDTest, ComparisonOrdersUUIDsLexicographicallyByStorage)
	{
		spk::UUID::Storage lowerStorage{};
		spk::UUID::Storage higherStorage{};
		higherStorage.back() = 1u;

		const spk::UUID lower(lowerStorage);
		const spk::UUID higher(higherStorage);

		EXPECT_LT(lower, higher);
		EXPECT_GT(higher, lower);
		EXPECT_EQ(lower, spk::UUID(lowerStorage));
	}

	TEST(UUIDTest, FromStringRejectsMalformedLength)
	{
		EXPECT_THROW((void)spk::UUID::fromString(""), std::invalid_argument);
		EXPECT_THROW((void)spk::UUID::fromString("00112233-4455-4677-8899-aabbccddeef"), std::invalid_argument);
		EXPECT_THROW((void)spk::UUID::fromString("00112233-4455-4677-8899-aabbccddeeff0"), std::invalid_argument);
	}

	TEST(UUIDTest, FromStringRejectsMalformedHyphenPlacement)
	{
		EXPECT_THROW((void)spk::UUID::fromString("001122334455-4677-8899-aabbccddeeff"), std::invalid_argument);
		EXPECT_THROW((void)spk::UUID::fromString("00112233_4455-4677-8899-aabbccddeeff"), std::invalid_argument);
		EXPECT_THROW((void)spk::UUID::fromString("00112233-4455-4677-8899a-abbccddeeff"), std::invalid_argument);
	}

	TEST(UUIDTest, FromStringRejectsNonHexadecimalCharacters)
	{
		EXPECT_THROW((void)spk::UUID::fromString("g0112233-4455-4677-8899-aabbccddeeff"), std::invalid_argument);
		EXPECT_THROW((void)spk::UUID::fromString("00112233-4455-4677-8899-aabbccddeefz"), std::invalid_argument);
	}

	TEST(UUIDTest, FromStringRejectsLeadingTrailingAndEmbeddedWhitespace)
	{
		EXPECT_THROW((void)spk::UUID::fromString(" 00112233-4455-4677-8899-aabbccddeeff"), std::invalid_argument);
		EXPECT_THROW((void)spk::UUID::fromString("00112233-4455-4677-8899-aabbccddeeff "), std::invalid_argument);
		EXPECT_THROW((void)spk::UUID::fromString("00112233-4455-4677-8899-aabbccd eeff"), std::invalid_argument);
	}

	TEST(UUIDTest, TryParseNeverThrowsAndReturnsNulloptForEveryMalformedCategory)
	{
		for (const std::string &text : {
				 std::string{},
				 std::string{"00112233-4455-4677-8899-aabbccddeef"},
				 std::string{"001122334455-4677-8899-aabbccddeeff"},
				 std::string{"00112233-4455-4677-8899-aabbccddeefz"},
				 std::string{" 00112233-4455-4677-8899-aabbccddeeff"}})
		{
			std::optional<spk::UUID> parsed;
			EXPECT_NO_THROW(parsed = spk::UUID::tryParse(text));
			EXPECT_FALSE(parsed.has_value());
		}
	}
}
