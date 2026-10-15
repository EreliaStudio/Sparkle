#include <gtest/gtest.h>

#include <chrono>
#include <functional>

#include "core/context/update_context.hpp"
#include "engine/behaviour.hpp"
#include "engine/engine.hpp"
#include "engine/entity3d.hpp"
#include "engine/system.hpp"
#include "engine/transform3d.hpp"
#include "math/vector3.hpp"

namespace
{
	class CountingBehaviour final : public spk::Behaviour
	{
	public:
		std::size_t updates = 0;

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++updates;
		}
	};

	class CountingSystem final : public spk::System
	{
	public:
		std::size_t updates = 0;

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++updates;
		}
	};

	class CallbackSystem final : public spk::System
	{
	public:
		std::function<void()> callback;
		std::size_t updates = 0;

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++updates;
			if (callback)
			{
				callback();
			}
		}
	};
}

TEST(CoreEngineTest, EntityBehaviourAndSystemShareOneHeadlessUpdateMechanism)
{
	spk::Engine engine;
	spk::Entity3D entity("Server entity");
	auto &behaviour = entity.addBehaviour<CountingBehaviour>();
	auto &system = engine.addSystem<CountingSystem>();

	entity.transform().place({1.0f, 2.0f, 3.0f});
	engine.addEntity(&entity);

	spk::UpdateContext context{
		.time = std::chrono::seconds(4),
		.deltaTime = std::chrono::milliseconds(16)};
	engine.updateState(context);

	EXPECT_EQ(behaviour.updates, 1u);
	EXPECT_EQ(system.updates, 1u);
	EXPECT_EQ(entity.transform().position(), spk::Vector3(1.0f, 2.0f, 3.0f));
	EXPECT_EQ(entity.context(), &engine);
	EXPECT_EQ(entity.parent(), &engine.root());
}

TEST(CoreEngineTest, InactiveCoreObjectsDoNotUpdate)
{
	spk::Engine engine;
	spk::Entity entity("Entity");
	auto &behaviour = entity.addBehaviour<CountingBehaviour>();
	auto &system = engine.addSystem<CountingSystem>();
	engine.addEntity(&entity);

	entity.deactivate();
	system.deactivate();

	spk::UpdateContext context{};
	engine.updateState(context);

	EXPECT_EQ(behaviour.updates, 0u);
	EXPECT_EQ(system.updates, 0u);
}

TEST(CoreEngineTest, SystemTraversalRemainsStableDuringMutation)
{
	spk::Engine engine;
	auto &controller = engine.addSystem<CallbackSystem>();
	auto &removed = engine.addSystem<CallbackSystem>();
	CallbackSystem *added = nullptr;
	bool mutated = false;

	controller.callback = [&]() {
		if (mutated)
		{
			return;
		}

		mutated = true;
		engine.removeSystem(removed);
		added = &engine.addSystem<CallbackSystem>();
	};

	spk::UpdateContext context{};
	engine.updateState(context);

	ASSERT_NE(added, nullptr);
	EXPECT_EQ(controller.updates, 1u);
	EXPECT_EQ(added->updates, 0u);

	engine.updateState(context);
	EXPECT_EQ(controller.updates, 2u);
	EXPECT_EQ(added->updates, 1u);
}

TEST(CoreEngineTest, GeometryAndContextStillPropagateWithoutGraphics)
{
	spk::Engine engine;
	spk::Entity parent("Parent");
	spk::Entity child("Child", &parent);
	const spk::Rect2D geometry{
		.anchor = {12, 34},
		.size = {640, 480}};

	engine.addEntity(&parent);
	engine.handleGeometryChange(geometry);

	EXPECT_EQ(engine.geometry(), geometry);
	EXPECT_EQ(parent.geometry(), geometry);
	EXPECT_EQ(child.geometry(), geometry);
	EXPECT_EQ(parent.context(), &engine);
	EXPECT_EQ(child.context(), &engine);

	engine.removeEntity(&parent);
	EXPECT_EQ(parent.context(), nullptr);
	EXPECT_EQ(child.context(), nullptr);
}
