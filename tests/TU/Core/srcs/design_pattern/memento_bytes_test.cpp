#include "container/byte_stream.hpp"
#include "design_pattern/trait/memento_trait.hpp"
#include "exception.hpp"

#include <cstddef>
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
		mutable std::size_t _saveCalls = 0;

	public:
		void saveMemento(spk::ByteStream::Writer &writer) const
		{
			++_saveCalls;
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

		[[nodiscard]] std::size_t saveCalls() const noexcept
		{
			return _saveCalls;
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

	struct MissingLoadHook
	{
		void saveMemento(spk::ByteStream::Writer &) const
		{
		}
	};

	struct WrongSaveSignature
	{
		void saveMemento(spk::ByteStream::Writer &)
		{
		}
		void loadMemento(const spk::ByteStream::Slice &)
		{
		}
	};

	static_assert(spk::MementoSerializable<Character>);
	static_assert(!spk::MementoSerializable<MissingLoadHook>);
	static_assert(!spk::MementoSerializable<WrongSaveSignature>);
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
	auto failingOperation = [&] {
		character.transaction([&] {
			character.assign(20, 30);
			throw std::runtime_error("transaction failed");
		});
	};
	EXPECT_THROW(failingOperation(), std::runtime_error);
	EXPECT_EQ(character.health(), 100);
	EXPECT_EQ(character.shield(), 50);
}

TEST(MementoBytes, NestedTransactionRestoresInnerAndOuterStates)
{
	Character character;
	auto inner = [&] {
		character.transaction([&] {
			character.assign(20, 25);
			throw spk::Exception("inner");
		});
	};
	auto outer = [&] {
		character.transaction([&] {
			character.assign(10, 15);
			EXPECT_THROW(inner(), spk::Exception);
			EXPECT_EQ(character.health(), 10);
			EXPECT_EQ(character.shield(), 15);
			throw spk::Exception("outer");
		});
	};
	EXPECT_THROW(outer(), spk::Exception);
	EXPECT_EQ(character.health(), 100);
	EXPECT_EQ(character.shield(), 50);
}

TEST(MementoBytes, TruncatedRestoreDoesNotChangeOriginal)
{
	Character character;
	spk::ByteStream::Writer writer;
	writer << std::int32_t{20};
	auto malformed = std::move(writer).build();
	EXPECT_THROW(character.loadSecure(malformed), spk::Exception);
	EXPECT_EQ(character.health(), 100);
	EXPECT_EQ(character.shield(), 50);
}

TEST(MementoBytes, TrailingBytesCauseRollback)
{
	Character character;
	spk::ByteStream::Writer writer;
	writer << std::int32_t{20} << std::int32_t{30} << std::uint8_t{4};
	auto malformed = std::move(writer).build();
	EXPECT_THROW(character.loadSecure(malformed), spk::Exception);
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
	EXPECT_THROW(character.loadSecure(snapshot), spk::Exception);
	character.rejectRestore(false);
	character.load(snapshot);
	EXPECT_EQ(character.health(), 20);
}

TEST(MementoBytes, DirectLoadAvoidsBackupSerialization)
{
	Character character;
	character.assign(11, 12);
	const auto snapshot = character.save();
	character.assign(30, 40);
	const auto before = character.saveCalls();
	character.load(snapshot);
	EXPECT_EQ(character.saveCalls(), before);
	EXPECT_EQ(character.health(), 11);
	EXPECT_EQ(character.shield(), 12);
	character.assign(50, 60);
	character.loadSecure(snapshot);
	EXPECT_EQ(character.saveCalls(), before + 1);
	EXPECT_EQ(character.health(), 11);
	EXPECT_EQ(character.shield(), 12);
}

TEST(MementoBytes, DirectLoadMayLeavePartiallyRestoredState)
{
	Character character;
	spk::ByteStream::Writer writer;
	writer << std::int32_t{20};
	const auto malformed = std::move(writer).build();
	EXPECT_THROW(character.load(malformed), spk::Exception);
	EXPECT_EQ(character.health(), 20);
	EXPECT_EQ(character.shield(), 50);
}

TEST(MementoBytes, DirectLoadMayCommitBeforeTrailingByteRejection)
{
	Character character;
	spk::ByteStream::Writer writer;
	writer << std::int32_t{20} << std::int32_t{30} << std::uint8_t{4};
	const auto malformed = std::move(writer).build();
	EXPECT_THROW(character.load(malformed), spk::Exception);
	EXPECT_EQ(character.health(), 20);
	EXPECT_EQ(character.shield(), 30);
}

TEST(MementoBytes, SecureLoadRestoresValidSnapshot)
{
	Character character;
	character.assign(3, 4);
	const auto snapshot = character.save();
	character.assign(9, 10);
	character.loadSecure(snapshot);
	EXPECT_EQ(character.health(), 3);
	EXPECT_EQ(character.shield(), 4);
}
