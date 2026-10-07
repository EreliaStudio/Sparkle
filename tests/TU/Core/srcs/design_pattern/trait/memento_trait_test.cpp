#include "design_pattern/trait/memento_trait.hpp"
#include "exception.hpp"
#include <gtest/gtest.h>
#include <string>

namespace
{
	class Counter : public spk::MementoTrait
	{
		struct Snapshot final : spk::MementoTrait::Snapshot
		{
			int value;
			explicit Snapshot(int value) :
				value(value)
			{
			}
		};
		std::unique_ptr<const spk::MementoTrait::Snapshot> _saveSnapshot() const override
		{
			return std::make_unique<Snapshot>(value);
		}
		void _restoreSnapshot(const spk::MementoTrait::Snapshot &snapshot) override
		{
			const auto *saved = dynamic_cast<const Snapshot *>(&snapshot);
			if (!saved)
			{
				throw spk::Exception("Incompatible counter snapshot");
			}
			value = saved->value;
		}

	public:
		int value = 0;
	};
}

TEST(MementoTraitTest, MultipleSnapshotsRemainIndependentAndCanBeRestoredRepeatedly)
{
	Counter value;
	value.value = 1;
	const auto first = value.saveSnapshot();
	value.value = 2;
	const auto second = value.saveSnapshot();
	value.value = 3;
	value.restoreSnapshot(*first);
	EXPECT_EQ(value.value, 1);
	value.restoreSnapshot(*second);
	EXPECT_EQ(value.value, 2);
	value.restoreSnapshot(*first);
	EXPECT_EQ(value.value, 1);
}

TEST(MementoTraitTest, SnapshotOutlivesItsOriginAndCompatibleObjectsMayRestoreIt)
{
	std::unique_ptr<const spk::MementoTrait::Snapshot> saved;
	{
		Counter origin;
		origin.value = 42;
		saved = origin.saveSnapshot();
	}
	Counter target;
	target.restoreSnapshot(*saved);
	EXPECT_EQ(target.value, 42);
}

TEST(MementoTraitTest, ApplicationRejectsIncompatibleSnapshotsWithoutMutation)
{
	struct Other final : spk::MementoTrait::Snapshot
	{
	};
	Counter value;
	value.value = 7;
	EXPECT_THROW(value.restoreSnapshot(Other{}), spk::Exception);
	EXPECT_EQ(value.value, 7);
}

TEST(MementoTraitTest, EmptySnapshotsAreRejected)
{
	class Empty final : public spk::MementoTrait
	{
		std::unique_ptr<const Snapshot> _saveSnapshot() const override
		{
			return nullptr;
		}
		void _restoreSnapshot(const Snapshot &) override
		{
		}
	} value;
	EXPECT_THROW((void)value.saveSnapshot(), spk::Exception);
}
