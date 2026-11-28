#include <gtest/gtest.h>

#include "container/byte_stream.hpp"
#include "exception.hpp"
#include "network/message.hpp"
#include "voxel/volume.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace
{
	using Volume = spk::Voxel::Volume;
	using Definition = spk::Voxel::Definition;
	using Registry = Definition::Registry;
	using Shape = spk::Voxel::Shape;
	using Position = spk::Vector3UInt;
	using Horizontal = spk::Voxel::Cell::HorizontalOrientation;
	using Vertical = spk::Voxel::Cell::VerticalOrientation;

	static_assert(std::is_same_v<spk::Message::Reader, spk::ByteStream::Reader>);

	[[nodiscard]] Registry registry()
	{
		Shape::Builder shape;
		shape.addSlot("surface");
		Registry::Builder builder;
		builder.add(7, Definition(shape.build(), {{"surface", {"stone"}}}));
		return builder.build();
	}

	[[nodiscard]] Volume populated(const Registry &definitions)
	{
		Volume::Builder builder({4, 2, 1});
		builder.bind(definitions);
		constexpr std::array horizontals{
			Horizontal::XPositive,
			Horizontal::ZNegative,
			Horizontal::XNegative,
			Horizontal::ZPositive};
		for (std::uint32_t i = 0; i < 8; ++i)
		{
			builder.set({i % 4, i / 4, 0}, 7, horizontals[i % 4], i < 4 ? Vertical::YPositive : Vertical::YNegative);
		}
		return builder.build();
	}

	[[nodiscard]] spk::ByteStream encode(const Volume &volume)
	{
		spk::ByteStream::Writer writer;
		writer << volume;
		return std::move(writer).build();
	}

	[[nodiscard]] spk::ByteStream altered(
		const spk::ByteStream &original,
		std::size_t offset,
		std::uint32_t value)
	{
		spk::ByteStream::Writer writer;
		writer.append(original.data().data(), original.size());
		const std::array<std::byte, 4> bytes{
			std::byte(value & 0xFFU),
			std::byte((value >> 8) & 0xFFU),
			std::byte((value >> 16) & 0xFFU),
			std::byte((value >> 24) & 0xFFU)};
		writer.edit(offset, bytes.data(), bytes.size());
		return std::move(writer).build();
	}

	[[nodiscard]] std::uint32_t word(std::span<const std::byte> bytes, std::size_t offset)
	{
		std::uint32_t result = 0;
		for (std::size_t i = 0; i < 4; ++i)
		{
			result |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + i])) << (8 * i);
		}
		return result;
	}

	void expectSame(const Volume &expected, const Volume &actual)
	{
		ASSERT_EQ(expected.dimensions(), actual.dimensions());
		const auto dimensions = expected.dimensions();
		for (std::uint32_t z = 0; z < dimensions.z; ++z)
		{
			for (std::uint32_t y = 0; y < dimensions.y; ++y)
			{
				for (std::uint32_t x = 0; x < dimensions.x; ++x)
				{
					EXPECT_EQ(expected.atRaw({x, y, z}), actual.atRaw({x, y, z}));
				}
			}
		}
	}
}

TEST(VoxelVolumeStreamTest, ByteStreamRoundTripPreservesPackedIDsAndAllEightOrientations)
{
	const auto definitions = registry();
	const auto original = populated(definitions);
	const auto stream = encode(original);
	auto reader = stream.reader();

	Volume::Builder builder(original.dimensions());
	builder.bind(definitions);
	reader >> builder;
	const auto restored = builder.build();

	EXPECT_EQ(reader.remaining(), 0u);
	expectSame(original, restored);
	EXPECT_TRUE(restored.at({0, 0, 0}).definition() == original.at({0, 0, 0}).definition());
}

TEST(VoxelVolumeStreamTest, MessageWriterAndReaderUseTheSameVoxelCodec)
{
	const auto definitions = registry();
	const auto original = populated(definitions);
	spk::Message::Writer writer(271);
	writer.setRequestID(42);
	writer << original;
	const auto message = std::move(writer).build();
	EXPECT_EQ(message.type(), 271u);
	EXPECT_EQ(message.requestID(), 42u);

	Volume::Builder builder(original.dimensions());
	builder.bind(definitions);
	auto reader = message.reader();
	reader >> builder;
	expectSame(original, builder.build());
	EXPECT_EQ(reader.remaining(), 0u);
}

TEST(VoxelVolumeStreamTest, SerializedLayoutIsLittleEndianAndIndependentOfCppStructs)
{
	const auto definitions = registry();
	Volume::Builder builder({2, 1, 1});
	builder.bind(definitions);
	builder.set({1, 0, 0}, 7, Horizontal::XNegative, Vertical::YNegative);
	const auto original = builder.build();
	const auto stream = encode(original);
	const auto bytes = stream.data();

	ASSERT_EQ(bytes.size(), 28u);
	EXPECT_EQ(std::to_integer<unsigned char>(bytes[0]), 'S');
	EXPECT_EQ(std::to_integer<unsigned char>(bytes[1]), 'P');
	EXPECT_EQ(std::to_integer<unsigned char>(bytes[2]), 'K');
	EXPECT_EQ(std::to_integer<unsigned char>(bytes[3]), 'V');
	EXPECT_EQ(word(bytes, 4), 1u);
	EXPECT_EQ(word(bytes, 8), 2u);
	EXPECT_EQ(word(bytes, 12), 1u);
	EXPECT_EQ(word(bytes, 16), 1u);
	EXPECT_EQ(word(bytes, 20), 0u);
	EXPECT_EQ(word(bytes, 24), Volume::PackedCell::encode(7, Horizontal::XNegative, Vertical::YNegative));
}

TEST(VoxelVolumeStreamTest, MultipleVolumesCanBeChainedAfterOtherPayloadData)
{
	const auto definitions = registry();
	const auto original = populated(definitions);
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{99};
	writer << original << original;
	const auto stream = std::move(writer).build();
	auto reader = stream.reader();
	EXPECT_EQ(reader.get<std::uint32_t>(), 99u);

	Volume::Builder first(original.dimensions());
	Volume::Builder second(original.dimensions());
	first.bind(definitions);
	second.bind(definitions);
	reader >> first >> second;
	expectSame(original, first.build());
	expectSame(original, second.build());
	EXPECT_EQ(reader.remaining(), 0u);
}

TEST(VoxelVolumeStreamTest, AllTruncationsPreserveBuilderAndReader)
{
	const auto definitions = registry();
	const auto original = populated(definitions);
	const auto stream = encode(original);
	for (std::size_t length = 0; length < stream.size(); ++length)
	{
		auto reader = stream.reader(0, length);
		Volume::Builder builder(original.dimensions());
		builder.bind(definitions);
		builder.set({0, 0, 0}, 7);
		EXPECT_THROW(builder.load(reader), spk::Exception) << "length " << length;
		EXPECT_EQ(reader.readOffset(), 0u);
		EXPECT_EQ(Volume::Builder(builder).build().atRaw({0, 0, 0}), original.atRaw({0, 0, 0}));
	}
}

TEST(VoxelVolumeStreamTest, RejectsUnknownMagicVersionAndDimensionsAtomically)
{
	const auto definitions = registry();
	const auto original = populated(definitions);
	const auto stream = encode(original);
	const std::array corruptions{
		std::pair{std::size_t{0}, std::uint32_t{0}},
		std::pair{std::size_t{4}, std::uint32_t{99}},
		std::pair{std::size_t{8}, std::uint32_t{100}},
		std::pair{std::size_t{12}, std::uint32_t{0}},
		std::pair{std::size_t{16}, std::uint32_t{9}}};
	for (const auto &[offset, value] : corruptions)
	{
		auto reader = altered(stream, offset, value).reader();
		Volume::Builder builder(original.dimensions());
		builder.bind(definitions);
		EXPECT_THROW(reader >> builder, spk::Exception);
		EXPECT_EQ(reader.readOffset(), 0u);
		EXPECT_EQ(Volume::Builder(builder).build().atRaw({0, 0, 0}), 0u);
	}
}

TEST(VoxelVolumeStreamTest, MissingDefinitionIdRejectsEntirePayloadWithoutChanges)
{
	const auto definitions = registry();
	const auto original = populated(definitions);
	const auto stream = altered(encode(original), 20, Volume::PackedCell::encode(123, Horizontal::XPositive, Vertical::YPositive));
	auto reader = stream.reader();

	Volume::Builder builder(original.dimensions());
	builder.bind(definitions);
	builder.set({1, 0, 0}, 7);
	EXPECT_THROW(reader >> builder, spk::Exception);
	EXPECT_EQ(reader.readOffset(), 0u);
	const auto preserved = builder.build();
	EXPECT_EQ(preserved.atRaw({0, 0, 0}), 0u);
	EXPECT_EQ(preserved.atRaw({1, 0, 0}), Volume::PackedCell::encode(7, Horizontal::XPositive, Vertical::YPositive));
}

TEST(VoxelVolumeStreamTest, EmptyRegistryRejectsNonAirPayloadAndAcceptsAirOnly)
{
	const auto definitions = registry();
	const auto full = encode(populated(definitions));
	Volume::Builder missing({4, 2, 1});
	auto reader = full.reader();
	EXPECT_THROW(missing.load(reader), spk::Exception);
	EXPECT_EQ(reader.readOffset(), 0u);

	Volume::Builder airBuilder({4, 2, 1});
	const auto air = airBuilder.build();
	const auto serializedAir = encode(air);
	Volume::Builder receiver({4, 2, 1});
	auto airReader = serializedAir.reader();
	EXPECT_NO_THROW(receiver.load(airReader));
	expectSame(air, receiver.build());
}

TEST(VoxelVolumeStreamTest, AlreadyConsumedBuilderRejectsStreamLoad)
{
	const auto definitions = registry();
	const auto original = populated(definitions);
	auto reader = encode(original).reader();
	Volume::Builder builder(original.dimensions());
	builder.bind(definitions);
	(void)builder.build();
	EXPECT_THROW(builder.load(reader), spk::Exception);
	EXPECT_EQ(reader.readOffset(), 0u);
}

TEST(VoxelVolumeStreamTest, ReaderPreservesAlreadyConsumedPrefixOnFailure)
{
	const auto definitions = registry();
	const auto original = populated(definitions);
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{0x12345678};
	writer << original;
	const auto stream = std::move(writer).build();
	auto reader = stream.reader();
	EXPECT_EQ(reader.get<std::uint32_t>(), 0x12345678u);
	const auto offset = reader.readOffset();

	Volume::Builder wrongDimensions({1, 1, 1});
	wrongDimensions.bind(definitions);
	EXPECT_THROW(reader >> wrongDimensions, spk::Exception);
	EXPECT_EQ(reader.readOffset(), offset);

	Volume::Builder correct(original.dimensions());
	correct.bind(definitions);
	reader >> correct;
	expectSame(original, correct.build());
	EXPECT_EQ(reader.remaining(), 0u);
}
