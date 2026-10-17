#include "design_pattern/publishable.hpp"
#include "exception.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <latch>
#include <memory>
#include <semaphore>
#include <thread>
#include <utility>
#include <vector>

namespace
{
	class TestPublisher final : public spk::Publishable
	{
	public:
		using Publishable::add;
		using Publishable::contains;
		using Publishable::remove;
	};

	class AtomicValuePart : public spk::Publishable::Part
	{
	public:
		std::atomic<int> value{0};
		mutable std::atomic<int> captures{0};

		void set(int next)
		{
			value.store(next);
			invalidate();
		}

		void touch()
		{
			invalidate();
		}

	protected:
		void _encode(spk::ByteStream::Writer &writer) const override
		{
			++captures;
			writer << value.load();
		}

		void _decode(const spk::ByteStream::Reader &reader) override
		{
			int next;
			reader >> next;
			value.store(next);
		}
	};

	class DistinctUpdatePart final : public spk::Publishable::Part
	{
	public:
		int position = 0;
		int velocity = 0;

		void set(int newPosition, int newVelocity)
		{
			position = newPosition;
			velocity = newVelocity;
			invalidate();
		}

	protected:
		void _encode(spk::ByteStream::Writer &writer) const override
		{
			writer << position << velocity;
		}

		void _decode(const spk::ByteStream::Reader &reader) override
		{
			reader >> position >> velocity;
		}

		void _encodeUpdate(spk::ByteStream::Writer &writer) const override
		{
			writer << position;
		}

		void _decodeUpdate(const spk::ByteStream::Reader &reader) override
		{
			reader >> position;
		}
	};

	class EmptyPayloadPart final : public spk::Publishable::Part
	{
	public:
		int received = 0;

	protected:
		void _encode(spk::ByteStream::Writer &) const override
		{
		}

		void _decode(const spk::ByteStream::Reader &) override
		{
			++received;
		}
	};

	class GatedPart final : public spk::Publishable::Part
	{
	private:
		std::atomic<int> _value{0};
		mutable std::atomic<bool> _blockOnce{true};
		mutable std::atomic<bool> _failOnce{false};
		mutable std::binary_semaphore _started{0};
		mutable std::binary_semaphore _resume{0};

	public:
		void set(int value)
		{
			_value.store(value);
			invalidate();
		}

		void failNextCapture()
		{
			_failOnce.store(true);
		}

		void awaitEncoding() const
		{
			_started.acquire();
		}

		void resumeEncoding() const
		{
			_resume.release();
		}

	protected:
		void _encode(spk::ByteStream::Writer &writer) const override
		{
			const int captured = _value.load();
			if (_blockOnce.exchange(false))
			{
				_started.release();
				_resume.acquire();
			}
			if (_failOnce.exchange(false))
			{
				throw spk::Exception("Requested test capture failure.");
			}
			writer << captured;
		}

		void _decode(const spk::ByteStream::Reader &reader) override
		{
			int next;
			reader >> next;
			_value.store(next);
		}
	};

	spk::ByteStream singleInt(int value)
	{
		spk::ByteStream::Writer writer;
		writer << value;
		return std::move(writer).build();
	}

	spk::ByteStream makeCollection(
		std::uint8_t mode,
		const std::vector<std::pair<std::uint32_t, spk::ByteStream>> &sections)
	{
		spk::ByteStream::Writer writer;
		writer << mode << static_cast<std::uint32_t>(sections.size());
		for (const auto &[id, bytes] : sections)
		{
			writer << id << static_cast<std::uint64_t>(bytes.size());
			writer.append(bytes.data().data(), bytes.size());
		}
		return std::move(writer).build();
	}

	int firstInt(const spk::ByteStream &collection)
	{
		auto reader = collection.reader();
		(void)reader.get<std::uint8_t>();
		(void)reader.get<std::uint32_t>();
		(void)reader.get<std::uint32_t>();
		(void)reader.get<std::uint64_t>();
		return reader.get<int>();
	}
}

TEST(PublishableTest, OnePartNotifiesMultiplePublishablesIndependently)
{
	TestPublisher first;
	TestPublisher second;
	AtomicValuePart part;

	first.add(7, &part, spk::Publishable::Publication::Edition);
	second.add(12, &part, spk::Publishable::Publication::Edition);

	part.set(15);
	EXPECT_EQ(firstInt(first.encodeEdited()), 15);
	EXPECT_EQ(firstInt(second.encodeEdited()), 15);
	EXPECT_TRUE(first.encodeEdited().empty());
	EXPECT_TRUE(second.encodeEdited().empty());

	first.remove(7);
	part.set(20);
	EXPECT_TRUE(first.encodeEdited().empty());
	EXPECT_EQ(firstInt(second.encodeEdited()), 20);
	EXPECT_TRUE(second.contains(12));
}

TEST(PublishableTest, RemovedPartCannotReenterAfterIDReuse)
{
	TestPublisher publisher;
	AtomicValuePart oldPart;
	AtomicValuePart newPart;
	publisher.add(1, &oldPart, spk::Publishable::Publication::Edition);
	oldPart.set(10);
	publisher.remove(1);

	publisher.add(1, &newPart, spk::Publishable::Publication::Edition);
	EXPECT_TRUE(publisher.encodeEdited().empty());
	oldPart.set(40);
	EXPECT_TRUE(publisher.encodeEdited().empty());

	newPart.set(99);
	EXPECT_EQ(firstInt(publisher.encodeEdited()), 99);
	EXPECT_TRUE(publisher.encodeEdited().empty());
}

TEST(PublishableTest, FullAndPartialUpdateAreDecodedIndependently)
{
	TestPublisher origin;
	TestPublisher destination;
	DistinctUpdatePart source;
	DistinctUpdatePart target;
	origin.add(0, &source, spk::Publishable::Publication::Edition | spk::Publishable::Publication::Update);
	destination.add(0, &target);

	source.set(10, 20);
	destination.decode(origin.encodeInitialization().reader());
	EXPECT_EQ(target.position, 10);
	EXPECT_EQ(target.velocity, 20);

	source.set(33, 44);
	destination.decode(origin.encodeUpdate().reader());
	EXPECT_EQ(target.position, 33);
	EXPECT_EQ(target.velocity, 20);
	destination.decode(origin.encodeEdited().reader());
	EXPECT_EQ(target.position, 33);
	EXPECT_EQ(target.velocity, 44);
	EXPECT_TRUE(destination.encodeEdited().empty());
}

TEST(PublishableTest, DefaultUpdateHooksUseTheFullRepresentation)
{
	TestPublisher source;
	TestPublisher destination;
	AtomicValuePart from;
	AtomicValuePart to;
	source.add(1, &from, spk::Publishable::Publication::Update);
	destination.add(1, &to);
	from.set(77);
	destination.decode(source.encodeUpdate().reader());
	EXPECT_EQ(to.value.load(), 77);
}

TEST(PublishableTest, EmptyPartPayloadIsFramedAndDecodable)
{
	TestPublisher source;
	TestPublisher destination;
	EmptyPayloadPart from;
	EmptyPayloadPart to;
	source.add(1, &from);
	destination.add(1, &to);
	const auto serialized = source.encodeInitialization();
	EXPECT_FALSE(serialized.empty());
	EXPECT_NO_THROW(destination.decode(serialized.reader()));
	EXPECT_EQ(to.received, 1);
}

TEST(PublishableTest, UnknownFlagsAndPoliciesAreRejected)
{
	TestPublisher object;
	AtomicValuePart part;
	using Policy = spk::Publishable::Publication;
	using Behaviour = spk::Publishable::UnknownPartBehaviour;
	EXPECT_THROW(object.add(1, &part, static_cast<Policy>(0x80)), spk::Exception);
	EXPECT_FALSE(object.contains(1));
	EXPECT_THROW(object.decode(spk::ByteStream{}.reader(), static_cast<Behaviour>(42)), spk::Exception);
}

TEST(PublishableTest, LaterTruncatedSectionPreventsEarlierPartMutation)
{
	TestPublisher object;
	AtomicValuePart first;
	AtomicValuePart second;
	object.add(1, &first);
	object.add(2, &second);
	first.value.store(5);
	second.value.store(6);

	spk::ByteStream::Writer writer;
	writer << std::uint8_t{0} << std::uint32_t{2};
	writer << std::uint32_t{1} << std::uint64_t{sizeof(int)} << int{10};
	writer << std::uint32_t{2} << std::uint64_t{500} << int{20};
	EXPECT_THROW(object.decode(std::move(writer).build().reader()), spk::Exception);
	EXPECT_EQ(first.value.load(), 5);
	EXPECT_EQ(second.value.load(), 6);
}

TEST(PublishableTest, LateDuplicatePreventsEarlierPartMutation)
{
	TestPublisher object;
	AtomicValuePart first;
	object.add(1, &first);
	first.value.store(5);
	const auto data = makeCollection(0, {{1, singleInt(10)}, {1, singleInt(20)}});
	EXPECT_THROW(object.decode(data.reader()), spk::Exception);
	EXPECT_EQ(first.value.load(), 5);
}

TEST(PublishableTest, ExplicitZeroPartCollectionsAreAcceptedInEveryMode)
{
	TestPublisher object;
	for (std::uint8_t mode = 0; mode < 3; ++mode)
	{
		EXPECT_NO_THROW(object.decode(makeCollection(mode, {}).reader()));
	}
}

TEST(PublishableTest, IncomingDecodeDoesNotMarkPartEdited)
{
	TestPublisher object;
	AtomicValuePart part;
	object.add(1, &part, spk::Publishable::Publication::Edition);
	object.decode(makeCollection(1, {{1, singleInt(88)}}).reader());
	EXPECT_EQ(part.value.load(), 88);
	EXPECT_TRUE(object.encodeEdited().empty());
}

TEST(PublishableTest, SimultaneousReadersShareOneLazyCapture)
{
	TestPublisher publisher;
	AtomicValuePart part;
	publisher.add(1, &part, spk::Publishable::Publication::Update);
	part.set(47);
	std::latch start(1);
	std::atomic<int> completed{0};
	std::vector<std::thread> readers;
	for (int i = 0; i < 12; ++i)
	{
		readers.emplace_back([&]() {
			start.wait();
			if (firstInt(publisher.encodeUpdate()) == 47)
			{
				++completed;
			}
		});
	}
	start.count_down();
	for (auto &reader : readers)
	{
		reader.join();
	}
	EXPECT_EQ(completed.load(), 12);
	EXPECT_EQ(part.captures.load(), 1);
}

TEST(PublishableTest, InvalidationDuringEditionEncodingIsNotLost)
{
	TestPublisher publisher;
	GatedPart part;
	publisher.add(1, &part, spk::Publishable::Publication::Edition);
	part.set(1);

	spk::ByteStream first;
	std::thread encoding([&]() {
		first = publisher.encodeEdited();
	});
	part.awaitEncoding();

	std::latch invalidating(1);
	std::thread mutation([&]() {
		invalidating.count_down();
		part.set(2);
	});
	invalidating.wait();
	part.resumeEncoding();
	encoding.join();
	mutation.join();

	EXPECT_EQ(firstInt(first), 1);
	EXPECT_EQ(firstInt(publisher.encodeEdited()), 2);
	EXPECT_TRUE(publisher.encodeEdited().empty());
}

TEST(PublishableTest, RemovalDuringInFlightEncodingDoesNotRequeueOldPart)
{
	TestPublisher publisher;
	GatedPart part;
	AtomicValuePart replacement;
	publisher.add(1, &part, spk::Publishable::Publication::Edition);
	part.set(3);
	spk::ByteStream result;
	std::thread encoding([&]() {
		result = publisher.encodeEdited();
	});
	part.awaitEncoding();

	std::thread removal([&]() {
		publisher.remove(1);
	});
	part.resumeEncoding();
	encoding.join();
	removal.join();

	EXPECT_TRUE(publisher.encodeEdited().empty());
	publisher.add(1, &replacement, spk::Publishable::Publication::Edition);
	replacement.set(42);
	EXPECT_EQ(firstInt(publisher.encodeEdited()), 42);
	EXPECT_TRUE(publisher.encodeEdited().empty());
}

TEST(PublishableMemorySafetyTest, RepeatedRegistrationDestructionAndReuse)
{
	for (int iteration = 0; iteration < 250; ++iteration)
	{
		if ((iteration % 2) == 0)
		{
			TestPublisher publisher;
			auto part = std::make_unique<AtomicValuePart>();
			publisher.add(1, part.get(), spk::Publishable::Publication::Edition);
			part->set(iteration);
			part.reset();
			EXPECT_FALSE(publisher.contains(1));
			EXPECT_TRUE(publisher.encodeEdited().empty());
		}
		else
		{
			auto part = std::make_unique<AtomicValuePart>();
			{
				TestPublisher publisher;
				publisher.add(1, part.get(), spk::Publishable::Publication::Edition);
				part->set(iteration);
				EXPECT_FALSE(publisher.encodeEdited().empty());
			}
			part->set(iteration + 1);
		}
	}
}

TEST(PublishableStressTest, ConcurrentIndependentPartsEventuallyDeliverLatestState)
{
	constexpr std::size_t PartCount = 6;
	constexpr int Iterations = 350;
	TestPublisher source;
	TestPublisher target;
	std::array<std::unique_ptr<AtomicValuePart>, PartCount> originals;
	std::array<std::unique_ptr<AtomicValuePart>, PartCount> replicas;

	for (std::size_t index = 0; index < PartCount; ++index)
	{
		originals[index] = std::make_unique<AtomicValuePart>();
		replicas[index] = std::make_unique<AtomicValuePart>();
		source.add(static_cast<std::uint32_t>(index), originals[index].get(), spk::Publishable::Publication::Edition);
		target.add(static_cast<std::uint32_t>(index), replicas[index].get());
	}

	std::latch begin(1);
	std::atomic<std::size_t> remaining{PartCount};
	std::vector<std::thread> workers;

	for (std::size_t index = 0; index < PartCount; ++index)
	{
		workers.emplace_back([&, index]() {
			begin.wait();
			for (int revision = 1; revision <= Iterations; ++revision)
			{
				originals[index]->set(revision);
			}
			--remaining;
		});
	}

	std::thread consumer([&]() {
		begin.wait();
		while (remaining.load() != 0)
		{
			target.decode(source.encodeEdited().reader());
		}
		target.decode(source.encodeEdited().reader());
	});
	begin.count_down();
	for (auto &worker : workers)
	{
		worker.join();
	}
	consumer.join();

	for (const auto &part : replicas)
	{
		EXPECT_EQ(part->value.load(), Iterations);
	}
	EXPECT_TRUE(source.encodeEdited().empty());
}

TEST(PublishableTest, NewInvalidationsSurviveAnEncodingFailure)
{
	TestPublisher publisher;
	GatedPart gated;
	AtomicValuePart independent;
	publisher.add(1, &gated, spk::Publishable::Publication::Edition);
	publisher.add(2, &independent, spk::Publishable::Publication::Edition);
	gated.set(5);
	independent.set(7);
	gated.failNextCapture();

	std::atomic<bool> failed{false};
	std::thread encoding([&]() {
		try
		{
			(void)publisher.encodeEdited();
		} catch (const spk::Exception &)
		{
			failed.store(true);
		}
	});
	gated.awaitEncoding();
	independent.set(8);
	gated.resumeEncoding();
	encoding.join();

	EXPECT_TRUE(failed.load());

	TestPublisher receiver;
	AtomicValuePart first;
	AtomicValuePart second;
	receiver.add(1, &first);
	receiver.add(2, &second);
	receiver.decode(publisher.encodeEdited().reader());
	receiver.decode(publisher.encodeEdited().reader());
	EXPECT_TRUE(publisher.encodeEdited().empty());
	EXPECT_EQ(first.value.load(), 5);
	EXPECT_EQ(second.value.load(), 8);
}
