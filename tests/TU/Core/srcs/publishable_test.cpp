#include "design_pattern/publishable.hpp"
#include "exception.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
    class TestPart final : public spk::Publishable::Part
    {
    public:
        int value = 0;
        mutable int editionEncodes = 0;
        mutable int updateEncodes = 0;
        bool failEdition = false;
        bool failUpdate = false;

        void set(int newValue)
        {
            value = newValue;
            invalidate();
        }

    protected:
        void _encode(spk::ByteStream::Writer &writer) const override
        {
            ++editionEncodes;
            if (failEdition)
            {
                throw spk::Exception("Edition generator failure");
            }
            writer << value;
        }

        void _decode(const spk::ByteStream::Reader &reader) override
        {
            reader >> value;
        }

        void _encodeUpdate(spk::ByteStream::Writer &writer) const override
        {
            ++updateEncodes;
            if (failUpdate)
            {
                throw spk::Exception("Update generator failure");
            }
            writer << value;
        }

        void _decodeUpdate(const spk::ByteStream::Reader &reader) override
        {
            reader >> value;
        }
    };

    class ShortReaderPart final : public spk::Publishable::Part
    {
    protected:
        void _encode(spk::ByteStream::Writer &writer) const override
        {
            writer << std::uint64_t{1234};
        }

        void _decode(const spk::ByteStream::Reader &reader) override
        {
            std::uint32_t ignored;
            reader >> ignored;
        }
    };

    class TestPublishable final : public spk::Publishable
    {
    public:
        using Publishable::add;
        using Publishable::remove;
        using Publishable::contains;
    };

    spk::ByteStream collection(
        std::uint8_t mode,
        const std::vector<std::pair<std::uint32_t, spk::ByteStream>> &parts)
    {
        spk::ByteStream::Writer writer;
        writer << mode << static_cast<std::uint32_t>(parts.size());
        for (const auto &[id, data] : parts)
        {
            writer << id << static_cast<std::uint64_t>(data.size());
            writer.append(data.data().data(), data.size());
        }
        return std::move(writer).build();
    }

    spk::ByteStream integerPayload(int value)
    {
        spk::ByteStream::Writer writer;
        writer << value;
        return std::move(writer).build();
    }
}

TEST(PublishableTest, IdentityIsNeitherCopyableNorMovable)
{
    static_assert(!std::is_copy_constructible_v<spk::Publishable>);
    static_assert(!std::is_move_constructible_v<spk::Publishable>);
    static_assert(!std::is_copy_constructible_v<TestPart>);
    static_assert(!std::is_move_constructible_v<TestPart>);
}

TEST(PublishableTest, EmptyCollectionsAreNoOps)
{
    TestPublishable object;
    EXPECT_TRUE(object.encodeInitialization().empty());
    EXPECT_TRUE(object.encodeEdited().empty());
    EXPECT_TRUE(object.encodeUpdate().empty());

    object.decode(spk::ByteStream{}.reader());
    object.decode(collection(2, {}).reader());
}

TEST(PublishableTest, RegistrationValidationAndAutomaticUnregistration)
{
    TestPublishable object;
    EXPECT_FALSE(object.contains(1));
    EXPECT_THROW(object.add(1, nullptr), spk::Exception);

    {
        TestPart part;
        object.add(1, &part);
        EXPECT_TRUE(object.contains(1));
        EXPECT_THROW(object.add(1, &part), spk::Exception);
        EXPECT_THROW(object.add(2, &part), spk::Exception);
        EXPECT_TRUE(object.encodeEdited().empty());
        EXPECT_TRUE(object.encodeUpdate().empty());
    }
    EXPECT_FALSE(object.contains(1));
    EXPECT_THROW(object.remove(1), spk::Exception);
}

TEST(PublishableTest, InitializationSerializesAllPartsInAscendingIDOrder)
{
    TestPublishable object;
    TestPart high, low, middle;
    high.value = 50;
    low.value = 10;
    middle.value = 30;
    object.add(5, &high);
    object.add(1, &low);
    object.add(3, &middle);

    const auto data = object.encodeInitialization();
    auto reader = data.reader();
    EXPECT_EQ(reader.get<std::uint8_t>(), 0);
    EXPECT_EQ(reader.get<std::uint32_t>(), 3u);
    for (auto [id, value] : {std::pair{1u, 10}, {3u, 30}, {5u, 50}})
    {
        EXPECT_EQ(reader.get<std::uint32_t>(), id);
        EXPECT_EQ(reader.get<std::uint64_t>(), sizeof(int));
        EXPECT_EQ(reader.get<int>(), value);
    }
    EXPECT_EQ(reader.remaining(), 0u);
}

TEST(PublishableTest, LazyCachesGenerateOnlyOnDemandAndAreReused)
{
    TestPublishable object;
    TestPart part;
    object.add(0, &part, spk::Publishable::Publication::Edition |
                            spk::Publishable::Publication::Update);
    part.set(1);
    part.set(2);
    part.set(3);

    EXPECT_EQ(part.editionEncodes, 0);
    EXPECT_EQ(part.updateEncodes, 0);

    EXPECT_FALSE(object.encodeUpdate().empty());
    EXPECT_FALSE(object.encodeUpdate().empty());
    EXPECT_EQ(part.updateEncodes, 1);
    EXPECT_EQ(part.editionEncodes, 0);

    EXPECT_FALSE(object.encodeEdited().empty());
    EXPECT_TRUE(object.encodeEdited().empty());
    EXPECT_EQ(part.editionEncodes, 1);

    part.set(4);
    EXPECT_EQ(part.editionEncodes, 1);
    EXPECT_EQ(part.updateEncodes, 1);
    EXPECT_FALSE(object.encodeInitialization().empty());
    EXPECT_EQ(part.editionEncodes, 2);
    EXPECT_FALSE(object.encodeUpdate().empty());
    EXPECT_EQ(part.updateEncodes, 2);
}

TEST(PublishableTest, OnlyOutgoingSelectionsAreControlledByFlags)
{
    TestPublishable source;
    TestPart editionOnly, updateOnly, initializationOnly;
    source.add(1, &editionOnly, spk::Publishable::Publication::Edition);
    source.add(2, &updateOnly, spk::Publishable::Publication::Update);
    source.add(3, &initializationOnly);

    editionOnly.set(42);
    updateOnly.set(43);
    initializationOnly.set(44);

    auto edited = source.encodeEdited().reader();
    EXPECT_EQ(edited.get<std::uint8_t>(), 1);
    EXPECT_EQ(edited.get<std::uint32_t>(), 1u);
    EXPECT_EQ(edited.get<std::uint32_t>(), 1u);

    auto update = source.encodeUpdate().reader();
    EXPECT_EQ(update.get<std::uint8_t>(), 2);
    EXPECT_EQ(update.get<std::uint32_t>(), 1u);
    EXPECT_EQ(update.get<std::uint32_t>(), 2u);

    TestPublishable destination;
    TestPart decoded;
    destination.add(1, &decoded);
    destination.decode(collection(2, {{1, integerPayload(99)}}).reader());
    EXPECT_EQ(decoded.value, 99);
}

TEST(PublishableTest, FailedEditionKeepsTheFullPendingBatch)
{
    TestPublishable object;
    TestPart first, second;
    object.add(1, &first, spk::Publishable::Publication::Edition);
    object.add(2, &second, spk::Publishable::Publication::Edition);
    first.set(12);
    second.set(34);

    second.failEdition = true;
    EXPECT_THROW((void)object.encodeEdited(), spk::Exception);
    second.failEdition = false;

    TestPublishable destination;
    TestPart recoveredFirst, recoveredSecond;
    destination.add(1, &recoveredFirst);
    destination.add(2, &recoveredSecond);
    const auto recovered = object.encodeEdited();
    destination.decode(recovered.reader());
    EXPECT_EQ(recoveredFirst.value, 12);
    EXPECT_EQ(recoveredSecond.value, 34);
    EXPECT_TRUE(object.encodeEdited().empty());
}

TEST(PublishableTest, RemovedRegistrationsAreNotReplayed)
{
    TestPublishable object;
    TestPart part;
    object.add(1, &part, spk::Publishable::Publication::Edition);
    part.set(33);
    object.remove(1);
    EXPECT_TRUE(object.encodeEdited().empty());
    EXPECT_FALSE(object.contains(1));
}

TEST(PublishableTest, DecodeAcceptsUnorderedPartsAndSkipPolicy)
{
    TestPublishable object;
    TestPart first, second;
    object.add(1, &first);
    object.add(2, &second);

    const auto input = collection(0, {
        {2, integerPayload(20)},
        {99, integerPayload(999)},
        {1, integerPayload(10)}
    });
    EXPECT_THROW(object.decode(input.reader()), spk::Exception);
    EXPECT_EQ(first.value, 0);
    EXPECT_EQ(second.value, 0);

    object.decode(input.reader(), spk::Publishable::UnknownPartBehaviour::Skip);
    EXPECT_EQ(first.value, 10);
    EXPECT_EQ(second.value, 20);
}

TEST(PublishableTest, DecodeRejectsMalformedHeadersDuplicateIDsAndTruncatedPayloads)
{
    TestPublishable object;
    TestPart part;
    object.add(1, &part);

    spk::ByteStream::Writer shortHeader;
    shortHeader << std::uint8_t{0};
    const auto truncated = std::move(shortHeader).build();
    EXPECT_THROW(object.decode(truncated.reader()), spk::Exception);

    EXPECT_THROW(object.decode(collection(99, {}).reader()), spk::Exception);
    EXPECT_THROW(object.decode(collection(0, {
        {1, integerPayload(1)}, {1, integerPayload(2)}
    }).reader()), spk::Exception);

    spk::ByteStream::Writer oversized;
    oversized << std::uint8_t{1} << std::uint32_t{1};
    oversized << std::uint32_t{1} << std::uint64_t{1000};
    EXPECT_THROW(object.decode(std::move(oversized).build().reader()), spk::Exception);
}

TEST(PublishableTest, PartMustConsumeAllOfItsPayload)
{
    TestPublishable object;
    ShortReaderPart part;
    object.add(1, &part);

    const auto serialized = object.encodeInitialization();
    EXPECT_THROW(object.decode(serialized.reader()), spk::Exception);
}

TEST(PublishableTest, DecodeLeavesEnclosingReaderRemainder)
{
    TestPublishable object;
    TestPart part;
    object.add(1, &part);

    const auto first = collection(0, {{1, integerPayload(1)}});
    const auto second = collection(2, {{1, integerPayload(2)}});
    spk::ByteStream::Writer writer;
    writer.append(first.data().data(), first.size());
    writer.append(second.data().data(), second.size());
    auto reader = std::move(writer).build().reader();

    object.decode(reader);
    EXPECT_EQ(part.value, 1);
    EXPECT_GT(reader.remaining(), 0u);
    object.decode(reader);
    EXPECT_EQ(part.value, 2);
    EXPECT_EQ(reader.remaining(), 0u);
}

TEST(PublishableTest, ConcurrentEditionCallsConsumeOneBatch)
{
    TestPublishable object;
    TestPart part;
    object.add(1, &part, spk::Publishable::Publication::Edition);
    part.set(42);

    std::atomic<int> nonEmpty{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i)
    {
        threads.emplace_back([&]() {
            if (!object.encodeEdited().empty())
            {
                ++nonEmpty;
            }
        });
    }
    for (auto &thread : threads)
    {
        thread.join();
    }

    EXPECT_EQ(nonEmpty.load(), 1);
}
