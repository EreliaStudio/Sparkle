#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "engine/registry.hpp"

namespace
{
	class TrackedObject : public spk::ContextualizableTrait<int>,
						  public spk::Registry<int, TrackedObject>::Object
	{
	public:
		explicit TrackedObject(int context = 0) :
			spk::ContextualizableTrait<int>(context)
		{
		}
	};

	class PointerTrackedObject : public spk::ContextualizableTrait<void *>,
							 public spk::Registry<void *, PointerTrackedObject>::Object
	{
	public:
		explicit PointerTrackedObject(void *context = nullptr) :
			spk::ContextualizableTrait<void *>(context)
		{
		}
	};

	using IntRegistry = spk::Registry<int, TrackedObject>;
	using PointerRegistry = spk::Registry<void *, PointerTrackedObject>;

	static_assert(!std::is_copy_constructible_v<TrackedObject>);
	static_assert(!std::is_move_constructible_v<TrackedObject>);
}

TEST(RegistryTest, StandardObjectConstructionContextMoveAndDestructionAreAutomatic)
{
	ASSERT_TRUE(IntRegistry::elements(10).empty());
	ASSERT_TRUE(IntRegistry::elements(20).empty());

	{
		TrackedObject object(10);
		EXPECT_TRUE(IntRegistry::elements(10).contains(&object));
		EXPECT_FALSE(IntRegistry::elements(20).contains(&object));

		object.changeContext(20);
		EXPECT_FALSE(IntRegistry::elements(10).contains(&object));
		EXPECT_TRUE(IntRegistry::elements(20).contains(&object));
	}

	EXPECT_TRUE(IntRegistry::elements(10).empty());
	EXPECT_TRUE(IntRegistry::elements(20).empty());
}

TEST(RegistryTest, MultipleObjectsAreTrackedIndependently)
{
	TrackedObject first(42);
	TrackedObject second(42);
	TrackedObject third(7);

	EXPECT_EQ(IntRegistry::elements(42).size(), 2u);
	EXPECT_TRUE(IntRegistry::elements(42).contains(&first));
	EXPECT_TRUE(IntRegistry::elements(42).contains(&second));
	EXPECT_EQ(IntRegistry::elements(7).size(), 1u);
	EXPECT_TRUE(IntRegistry::elements(7).contains(&third));
}

TEST(RegistryTest, SameContextChangeIsANoOpForRegistryContracts)
{
	TrackedObject object(5);
	int additions = 0;
	int removals = 0;

	auto addition = IntRegistry::subscribeToAddition(5, [&](const int &, TrackedObject *) { ++additions; });
	auto removal = IntRegistry::subscribeToRemoval(5, [&](const int &, TrackedObject *) { ++removals; });

	object.changeContext(5);

	EXPECT_EQ(additions, 0);
	EXPECT_EQ(removals, 0);
	EXPECT_TRUE(IntRegistry::elements(5).contains(&object));
}

TEST(RegistryTest, AdditionAndRemovalContractsReportCorrectContextsAndObject)
{
	struct Event
	{
		std::string kind;
		int context;
		TrackedObject *object;
	};

	std::vector<Event> events;
	auto add10 = IntRegistry::subscribeToAddition(10, [&](const int &context, TrackedObject *object) {
		events.push_back({"add", context, object});
	});
	auto remove10 = IntRegistry::subscribeToRemoval(10, [&](const int &context, TrackedObject *object) {
		events.push_back({"remove", context, object});
	});
	auto add20 = IntRegistry::subscribeToAddition(20, [&](const int &context, TrackedObject *object) {
		events.push_back({"add", context, object});
	});
	auto remove20 = IntRegistry::subscribeToRemoval(20, [&](const int &context, TrackedObject *object) {
		events.push_back({"remove", context, object});
	});

	TrackedObject *address = nullptr;
	{
		TrackedObject object(10);
		address = &object;
		object.changeContext(20);
	}

	ASSERT_EQ(events.size(), 4u);
	EXPECT_EQ(events[0].kind, "add");
	EXPECT_EQ(events[0].context, 10);
	EXPECT_EQ(events[0].object, address);
	EXPECT_EQ(events[1].kind, "remove");
	EXPECT_EQ(events[1].context, 10);
	EXPECT_EQ(events[1].object, address);
	EXPECT_EQ(events[2].kind, "add");
	EXPECT_EQ(events[2].context, 20);
	EXPECT_EQ(events[2].object, address);
	EXPECT_EQ(events[3].kind, "remove");
	EXPECT_EQ(events[3].context, 20);
	EXPECT_EQ(events[3].object, address);
}

TEST(RegistryTest, NullAndDefaultPointerContextsAreSupported)
{
	int contextToken = 0;
	void *context = &contextToken;

	PointerTrackedObject object;
	EXPECT_TRUE(PointerRegistry::elements(nullptr).contains(&object));

	object.changeContext(context);
	EXPECT_FALSE(PointerRegistry::elements(nullptr).contains(&object));
	EXPECT_TRUE(PointerRegistry::elements(context).contains(&object));
}

TEST(RegistryTest, ContextKeyCanDieBeforeTheRegisteredObjectWhenItIsANonOwningPointer)
{
	struct RegistryContext
	{
		int value = 0;
	};

	class Object : public spk::ContextualizableTrait<RegistryContext *>,
				   public spk::Registry<RegistryContext *, Object>::Object
	{
	public:
		explicit Object(RegistryContext *context) : spk::ContextualizableTrait<RegistryContext *>(context) {}
	};

	using Registry = spk::Registry<RegistryContext *, Object>;
	std::unique_ptr<Object> object;
	RegistryContext *contextAddress = nullptr;

	{
		auto context = std::make_unique<RegistryContext>();
		contextAddress = context.get();
		object = std::make_unique<Object>(contextAddress);
		ASSERT_TRUE(Registry::elements(contextAddress).contains(object.get()));
	}

	EXPECT_NO_THROW(object.reset());
	EXPECT_TRUE(Registry::elements(contextAddress).empty());
}

TEST(RegistryTest, ConstRegistryResultExposesTheLiveSetWithoutMutationAPI)
{
	TrackedObject object(91);
	const IntRegistry::ElementSet &elements = IntRegistry::elements(91);

	ASSERT_EQ(elements.size(), 1u);
	EXPECT_TRUE(elements.contains(&object));
}

TEST(RegistryTest, RegistryIsObservablyCleanAfterObjectsAndContractsLeaveScope)
{
	constexpr int Context = 1337;
	{
		auto addition = IntRegistry::subscribeToAddition(Context, [](const int &, TrackedObject *) {});
		TrackedObject first(Context);
		TrackedObject second(Context);
		ASSERT_EQ(IntRegistry::elements(Context).size(), 2u);
	}

	EXPECT_TRUE(IntRegistry::elements(Context).empty());
}
