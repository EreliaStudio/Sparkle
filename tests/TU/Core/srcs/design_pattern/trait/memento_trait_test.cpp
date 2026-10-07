#include "design_pattern/trait/memento_trait.hpp"
#include "exception.hpp"
#include <gtest/gtest.h>
#include <string>

namespace
{
	class Counter : public spk::MementoTrait
	{
		Snapshot _save() const override
		{
			auto snapshot = Snapshot::object();
			snapshot["value"] = value;
			return snapshot;
		}
		void _restore(const Snapshot &snapshot) override
		{
			const auto *saved = snapshot.find("value");
			if (!saved || !saved->canAs<int>())
			{
				throw spk::Exception("Invalid counter snapshot");
			}
			value = saved->as<int>();
		}

	public:
		int value = 0;
	};
}

TEST(MementoTraitTest, MultipleSnapshotsRemainIndependentAndCanBeRestoredRepeatedly)
{
	Counter value;
	value.value = 1;
	const auto first = value.save();
	value.value = 2;
	const auto second = value.save();
	value.value = 3;
	value.restore(first);
	EXPECT_EQ(value.value, 1);
	value.restore(second);
	EXPECT_EQ(value.value, 2);
	value.restore(first);
	EXPECT_EQ(value.value, 1);
}

TEST(MementoTraitTest, SnapshotOutlivesItsOriginAndCompatibleObjectsMayRestoreIt)
{
	spk::MementoTrait::Snapshot saved;
	{
		Counter origin;
		origin.value = 42;
		saved = origin.save();
	}
	Counter target;
	target.restore(saved);
	EXPECT_EQ(target.value, 42);
}

TEST(MementoTraitTest, ApplicationRejectsIncompatibleSnapshotsWithoutMutation)
{
	Counter value;
	value.value = 7;
	EXPECT_THROW(value.restore(spk::JSON::Object::object()), spk::Exception);
	EXPECT_EQ(value.value, 7);
}

TEST(MementoTraitTest, EmptyObjectSnapshotsAreValid)
{
	class Empty final : public spk::MementoTrait
	{
		Snapshot _save() const override
		{
			return Snapshot::object();
		}
		void _restore(const Snapshot &) override
		{
		}
	} value;
	const auto snapshot = value.save();
	EXPECT_TRUE(snapshot.isObject());
	EXPECT_TRUE(snapshot.empty());
	EXPECT_NO_THROW(value.restore(snapshot));
}

TEST(MementoTraitTest, SnapshotIsAnIndependentEditableJSONValue)
{
	static_assert(std::same_as<spk::MementoTrait::Snapshot, spk::JSON::Object>);
	Counter value;
	value.value = 7;
	const auto original = value.save();
	auto edited = original;
	edited["value"] = 12;
	EXPECT_EQ(original.at("value").as<int>(), 7);
	EXPECT_EQ(value.value, 7);
	value.restore(edited);
	EXPECT_EQ(value.value, 12);
	value.restore(spk::JSON::Object::fromString(original.toString()));
	EXPECT_EQ(value.value, 7);
}
