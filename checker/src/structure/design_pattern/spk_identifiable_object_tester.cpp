#include "structure/design_pattern/spk_identifiable_object_tester.hpp"

#include <set>

TEST_F(IdentifiedObjectTest, UniqueIdentifiersAreGenerated)
{
	TestIdentifiedObject first;
	TestIdentifiedObject second;

	ASSERT_NE(first.id(), second.id());
}

TEST_F(IdentifiedObjectTest, CopyKeepsDistinctIdentifier)
{
	TestIdentifiedObject original;
	TestIdentifiedObject copy(original);

	ASSERT_NE(original.id(), copy.id());
}

TEST_F(IdentifiedObjectTest, MoveConstructorTransfersIdentifier)
{
	TestIdentifiedObject original;
	auto originalId = original.id();

	TestIdentifiedObject moved(std::move(original));
	ASSERT_EQ(moved.id(), originalId);
	ASSERT_EQ(original.id(), spk::IdentifiedObject<TestIdentifiedObject>::InvalidID);
}

TEST_F(IdentifiedObjectTest, MoveAssignmentTransfersAndRecyclesIdentifier)
{
	TestIdentifiedObject source;
	auto sourceId = source.id();

	TestIdentifiedObject destination;
	auto destinationId = destination.id();

	destination = std::move(source);

	ASSERT_EQ(destination.id(), sourceId);
	ASSERT_EQ(source.id(), spk::IdentifiedObject<TestIdentifiedObject>::InvalidID);

	TestIdentifiedObject reused;
	ASSERT_EQ(reused.id(), destinationId);
}

TEST_F(IdentifiedObjectTest, ChangeIdToUnusedValue)
{
	TestIdentifiedObject object;
	auto originalId = object.id();
	auto newId = originalId + 1234;

	object.changeID(newId);

	ASSERT_EQ(object.id(), newId);

	TestIdentifiedObject another;
	ASSERT_NE(another.id(), newId);
}

TEST_F(IdentifiedObjectTest, ChangeIdSwapsWhenTargetExists)
{
	TestIdentifiedObject first;
	TestIdentifiedObject second;

	auto firstId = first.id();
	auto secondId = second.id();

	first.changeID(secondId);

	ASSERT_EQ(first.id(), secondId);
	ASSERT_EQ(second.id(), firstId);
}

TEST_F(IdentifiedObjectTest, RegistryTracksLiveObjects)
{
	TestIdentifiedObject tracked;
	auto trackedId = tracked.id();

	auto ptr = spk::IdentifiedObject<TestIdentifiedObject>::getObjectById(trackedId);
	ASSERT_EQ(ptr.get(), &tracked);
}

TEST_F(IdentifiedObjectTest, RegistryReturnsNullForUnknownId)
{
	auto ptr = spk::IdentifiedObject<TestIdentifiedObject>::getObjectById(987654321);
	ASSERT_EQ(ptr.get(), nullptr);
}

TEST_F(IdentifiedObjectTest, ChangeIdToSameValueDoesNothing)
{
	TestIdentifiedObject object;
	auto id = object.id();
	object.changeID(id);
	ASSERT_EQ(object.id(), id);
}
