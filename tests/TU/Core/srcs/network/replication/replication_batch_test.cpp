#include "container/byte_stream.hpp"
#include "exception.hpp"
#include "network/message.hpp"
#include "network/replication/replication_batch.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <limits>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
	using Batch = spk::Network::ReplicationBatch;

	[[nodiscard]] Batch::Component makeComponent(std::string value, std::uint64_t revision = 1)
	{
		spk::ByteStream::Writer writer;
		writer << value;
		return {spk::UUID::generate(), revision, std::move(writer).build()};
	}

	[[nodiscard]] spk::ByteStream overwrite(
		const spk::ByteStream &stream, std::size_t offset, const void *value, std::size_t size)
	{
		std::vector<std::byte> buffer(stream.data().begin(), stream.data().end());
		if (offset > buffer.size() || size > buffer.size() - offset)
		{
			throw spk::Exception("Test patch exceeds buffer.");
		}
		std::memcpy(buffer.data() + offset, value, size);
		spk::ByteStream::Writer writer;
		writer.append(buffer.data(), buffer.size());
		return std::move(writer).build();
	}
}

TEST(ReplicationBatch, EmptyBatchContainsNoSections)
{
	const std::vector<Batch::Component> components;
	auto batch = Batch::decode(Batch::encode(components, 16));
	EXPECT_EQ(batch.componentCount(), 0u);
	EXPECT_EQ(batch.sectionCount(), 0u);
	EXPECT_THROW(batch.section(0), spk::Exception);
}

TEST(ReplicationBatch, OneComponentCreatesOneSection)
{
	const std::vector<Batch::Component> components{makeComponent("health")};
	auto batch = Batch::decode(Batch::encode(components, 16));
	ASSERT_EQ(batch.sectionCount(), 1u);
	auto records = Batch::decodeSection(batch.section(0), batch.count(0));
	ASSERT_EQ(records.size(), 1u);
	EXPECT_EQ(records[0].identifier, components[0].identifier);
	EXPECT_EQ(records[0].revision, 1u);
	std::string decoded;
	records[0].payload >> decoded;
	EXPECT_EQ(decoded, "health");
}

TEST(ReplicationBatch, OneComponentPerSection)
{
	const std::vector<Batch::Component> components{
		makeComponent("a"), makeComponent("bb"), makeComponent("ccc")};
	auto batch = Batch::decode(Batch::encode(components, 1));
	ASSERT_EQ(batch.sectionCount(), 3u);
	for (std::size_t index = 0; index < batch.sectionCount(); ++index)
	{
		auto records = Batch::decodeSection(batch.section(index), batch.count(index));
		ASSERT_EQ(records.size(), 1u);
		EXPECT_EQ(records[0].identifier, components[index].identifier);
	}
}

TEST(ReplicationBatch, PartialLastSectionAndVariablePayloadSizes)
{
	std::vector<Batch::Component> components;
	for (std::size_t index = 0; index < 7; ++index)
	{
		components.push_back(makeComponent(std::string(index * 113 + 1, 'x'), index));
	}
	auto batch = Batch::decode(Batch::encode(components, 3));
	ASSERT_EQ(batch.sectionCount(), 3u);
	EXPECT_EQ(batch.count(0), 3u);
	EXPECT_EQ(batch.count(1), 3u);
	EXPECT_EQ(batch.count(2), 1u);
	std::size_t current = 0;
	for (std::size_t section = 0; section < batch.sectionCount(); ++section)
	{
		for (auto &record : Batch::decodeSection(batch.section(section), batch.count(section)))
		{
			EXPECT_EQ(record.identifier, components[current].identifier);
			EXPECT_EQ(record.revision, current);
			std::string value;
			record.payload >> value;
			EXPECT_EQ(value, std::string(current * 113 + 1, 'x'));
			++current;
		}
	}
	EXPECT_EQ(current, components.size());
}

TEST(ReplicationBatch, SectionsRetainStorageAfterOriginalMessageDies)
{
	auto section = [] {
		auto component = makeComponent("retained");
		return Batch::decode(Batch::encode(std::span<const Batch::Component>(&component, 1), 1)).section(0);
	}();
	auto records = Batch::decodeSection(section, 1);
	std::string value;
	records[0].payload >> value;
	EXPECT_EQ(value, "retained");
}

TEST(ReplicationBatch, IndependentSectionReadersCanBeUsedConcurrently)
{
	std::vector<Batch::Component> components;
	for (std::size_t index = 0; index < 4; ++index)
	{
		components.push_back(makeComponent(std::to_string(index)));
	}
	auto batch = Batch::decode(Batch::encode(components, 1));
	std::vector<std::string> values(4);
	std::vector<std::thread> workers;
	for (std::size_t index = 0; index < 4; ++index)
	{
		workers.emplace_back([&, index] {
			auto records = Batch::decodeSection(batch.section(index), 1);
			records[0].payload >> values[index];
		});
	}
	for (auto &worker : workers)
	{
		worker.join();
	}
	for (std::size_t index = 0; index < 4; ++index)
	{
		EXPECT_EQ(values[index], std::to_string(index));
	}
}

TEST(ReplicationBatch, RejectsZeroSectionCapacity)
{
	const std::vector<Batch::Component> components{makeComponent("x")};
	EXPECT_THROW((void)Batch::encode(components, 0), spk::Exception);
}

TEST(ReplicationBatch, RejectsTruncatedHeader)
{
	spk::ByteStream::Writer writer;
	writer << std::uint32_t{Batch::Magic};
	EXPECT_THROW((void)Batch::decode(std::move(writer).build()), spk::Exception);
}

TEST(ReplicationBatch, RejectsInvalidMagic)
{
	const std::vector<Batch::Component> components{makeComponent("x")};
	auto batch = Batch::encode(components, 1);
	const std::uint32_t invalid = 0;
	EXPECT_THROW((void)Batch::decode(overwrite(batch, 0, &invalid, sizeof(invalid))), spk::Exception);
}

TEST(ReplicationBatch, RejectsIncorrectSectionCount)
{
	const std::vector<Batch::Component> components{makeComponent("a"), makeComponent("b")};
	auto batch = Batch::encode(components, 1);
	const std::uint32_t invalid = 1;
	EXPECT_THROW((void)Batch::decode(overwrite(batch, 12, &invalid, sizeof(invalid))), spk::Exception);
}

TEST(ReplicationBatch, RejectsOverlappingSections)
{
	const std::vector<Batch::Component> components{makeComponent("a"), makeComponent("b")};
	auto batch = Batch::encode(components, 1);
	const std::uint64_t invalid = 0;
	EXPECT_THROW((void)Batch::decode(overwrite(batch, 24, &invalid, sizeof(invalid))), spk::Exception);
}

TEST(ReplicationBatch, RejectsOutOfRangeSectionOffsets)
{
	const std::vector<Batch::Component> components{makeComponent("a"), makeComponent("b")};
	auto batch = Batch::encode(components, 1);
	const std::uint64_t invalid = std::numeric_limits<std::uint64_t>::max();
	EXPECT_THROW((void)Batch::decode(overwrite(batch, 24, &invalid, sizeof(invalid))), spk::Exception);
}

TEST(ReplicationBatch, RejectsTruncatedComponentRecord)
{
	const std::vector<Batch::Component> components{makeComponent("x")};
	auto batch = Batch::encode(components, 1);
	const std::uint32_t invalid = std::numeric_limits<std::uint32_t>::max();
	const auto corrupted = overwrite(batch, 16 + 16 + 16 + 8, &invalid, sizeof(invalid));
	auto index = Batch::decode(corrupted);
	EXPECT_THROW((void)Batch::decodeSection(index.section(0), 1), spk::Exception);
}

TEST(ReplicationBatch, RejectsUnexpectedTrailingComponentBytes)
{
	const std::vector<Batch::Component> components{makeComponent("x")};
	auto batch = Batch::encode(components, 1);
	const std::uint32_t invalid = 0;
	const auto corrupted = overwrite(batch, 16 + 16 + 16 + 8, &invalid, sizeof(invalid));
	auto index = Batch::decode(corrupted);
	EXPECT_THROW((void)Batch::decodeSection(index.section(0), 1), spk::Exception);
}

TEST(ReplicationBatch, SectionsAndComponentsShareOriginalStorage)
{
	const std::vector<Batch::Component> components{makeComponent("x")};
	auto stream = Batch::encode(components, 1);
	auto index = Batch::decode(stream);
	auto section = index.section(0);
	auto records = Batch::decodeSection(section, 1);
	const auto *begin = stream.data().data();
	const auto *end = begin + stream.size();
	EXPECT_GE(records[0].payload.data().data(), begin);
	EXPECT_LT(records[0].payload.data().data(), end);
}
