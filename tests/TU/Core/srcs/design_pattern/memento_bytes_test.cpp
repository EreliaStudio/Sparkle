#include "container/byte_stream.hpp"
#include "design_pattern/trait/memento_trait.hpp"
#include "exception.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <stdexcept>
#include <utility>

namespace
{
	class Character final : public spk::MementoTrait<Character>
	{
	private:
		std::int32_t _health = 100;
		std::int32_t _shield = 50;
		bool _rejectRestore = false;

	public:
		void saveMemento(spk::ByteStream::Writer &writer) const
		{
			writer << _health << _shield;
		}

		void loadMemento(const spk::ByteStream::Slice &reader)
		{
			reader >> _health;
			if (_rejectRestore)
			{
				throw spk::Exception("Simulated restore failure after partial mutation.");
			}
			reader >> _shield;
		}

		void assign(std::int32_t health, std::int32_t shield)
		{
			_health = health;
			_shield = shield;
		}

		[[nodiscard]] std::int32_t health() const noexcept
		{
			return _health;
		}

		[[nodiscard]] std::int32_t shield() const noexcept
		{
			return _shield;
		}

		void rejectRestore(bool value)
		{
			_rejectRestore = value;
		}
	};
}

TEST(MementoBytes, RestoresMultipleIndependentSnapshots)
{
	Character character;
	auto original = character.save();
	character.assign(10, 20);
	auto second = character.save();
	character.assign(7, 8);
	character.load(original);
	EXPECT_EQ(character.health(), 100);
	EXPECT_EQ(character.shield(), 50);
	character.load(second);
	EXPECT_EQ(character.health(), 10);
	EXPECT_EQ(character.shield(), 20);
	character.load(original);
	EXPECT_EQ(character.health(), 100);
}

TEST(MementoBytes, SnapshotSurvivesOriginalObject)
{
	const auto snapshot = [] {
		Character character;
		character.assign(22, 33);
		return character.save();
	}();
	Character restored;
	restored.load(snapshot);
	EXPECT_EQ(restored.health(), 22);
	EXPECT_EQ(restored.shield(), 33);
}

TEST(MementoBytes, SuccessfulTransactionCommits)
{
	Character character;
	character.transaction([&] {
		character.assign(20, 30);
	});
	EXPECT_EQ(character.health(), 20);
	EXPECT_EQ(character.shield(), 30);
}

TEST(MementoBytes, FailedTransactionRollsBack)
{
	Character character;
	EXPECT_THROW(character.transaction([&] {
		character.assign(20, 30);
		throw std::runtime_error("transaction failed");
	}), std::runtime_error);
	EXPECT_EQ(character.health(), 100);
	EXPECT_EQ(character.shield(), 50);
}

TEST(MementoBytes, NestedTransactionRestoresInnerAndOuterStates)
{
	Character character;
	EXPECT_THROW(character.transaction([&] {
		character.assign(10, 15);
		EXPECT_THROW(character.transaction([&] {
			character.assign(20, 25);
			throw spk::Exception("inner");
		}), spk::Exception);
		EXPECT_EQ(character.health(), 10);
		EXPECT_EQ(character.shield(), 15);
		throw spk::Exception("outer");
	}), spk::Exception);
	EXPECT_EQ(character.health(), 100);
	EXPECT_EQ(character.shield(), 50);
}

TEST(MementoBytes, TruncatedRestoreDoesNotChangeOriginal)
{
	Character character;
	spk::ByteStream::Writer writer;
	writer << std::int32_t{20};
	auto malformed = std::move(writer).build();
	EXPECT_THROW(character.load(malformed), spk::Exception);
	EXPECT_EQ(character.health(), 100);
	EXPECT_EQ(character.shield(), 50);
}

TEST(MementoBytes, TrailingBytesCauseRollback)
{
	Character character;
	spk::ByteStream::Writer writer;
	writer << std::int32_t{20} << std::int32_t{30} << std::uint8_t{4};
	auto malformed = std::move(writer).build();
	EXPECT_THROW(character.load(malformed), spk::Exception);
	EXPECT_EQ(character.health(), 100);
	EXPECT_EQ(character.shield(), 50);
}

TEST(MementoBytes, RollbackFailureIsReportedNotMisrepresentedAsAtomic)
{
	Character character;
	spk::ByteStream::Writer writer;
	writer << std::int32_t{20} << std::int32_t{30};
	const auto snapshot = std::move(writer).build();
	character.rejectRestore(true);
	EXPECT_THROW(character.load(snapshot), spk::Exception);
	EXPECT_EQ(character.health(), 100);
	character.rejectRestore(false);
	character.load(snapshot);
	EXPECT_EQ(character.health(), 20);
}
