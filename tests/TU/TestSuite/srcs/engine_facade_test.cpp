#include <gtest/gtest.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/context/render_context.hpp"
#include "core/context/update_context.hpp"
#include "engine/behaviour.hpp"
#include "engine/engine.hpp"
#include "engine/entity.hpp"
#include "engine/system.hpp"
#include "input/keyboard.hpp"
#include "input/mouse.hpp"
#include "rendering/render_command.hpp"

namespace
{
	class LogCommand final : public spk::RenderCommand
	{
	private:
		std::vector<std::string> *_log;
		std::string _entry;

	public:
		LogCommand(std::vector<std::string> &log, std::string entry) :
			_log(&log),
			_entry(std::move(entry))
		{
		}

		void execute(spk::RenderContext &) const override
		{
			_log->push_back(_entry);
		}
	};

	class EngineBehaviour final : public spk::Behaviour
	{
	public:
		std::size_t updateCalls = 0;
		std::size_t keyCalls = 0;

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++updateCalls;
		}

		void _onKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			++keyCalls;
		}
	};

	class EngineSystem final : public spk::System
	{
	public:
		std::size_t updateCalls = 0;
		std::size_t keyCalls = 0;

		explicit EngineSystem(std::string name = "system") : spk::System(std::move(name)) {}

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++updateCalls;
		}

		void _onKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			++keyCalls;
		}
	};

	class CallbackSystem final : public spk::System
	{
	private:
		std::size_t *_updateCalls;

	public:
		std::function<void()> callback;

		explicit CallbackSystem(std::size_t &updateCalls) :
			_updateCalls(&updateCalls)
		{
		}

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++(*_updateCalls);
			if (callback)
			{
				callback();
			}
		}
	};

	class CallbackEngineBehaviour final : public spk::Behaviour
	{
	private:
		std::size_t *_updateCalls;

	public:
		std::function<void()> callback;

		explicit CallbackEngineBehaviour(std::size_t &updateCalls) :
			_updateCalls(&updateCalls)
		{
		}

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++(*_updateCalls);
			if (callback)
			{
				callback();
			}
		}
	};

	class RenderEntity final : public spk::Entity
	{
	private:
		std::vector<std::string> *_log;

	public:
		RenderEntity(std::string name, std::vector<std::string> &log) :
			spk::Entity(std::move(name)),
			_log(&log)
		{
		}

	protected:
		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder) override
		{
			builder.renderPass(spk::Engine::SceneRenderPassKey).emplace<LogCommand>(*_log, "scene");
			builder.renderPass(spk::Engine::PreSceneRenderPassKey).emplace<LogCommand>(*_log, "pre-scene");
		}
	};

	spk::UpdateContext makeUpdateContext(const spk::Keyboard &keyboard, const spk::Mouse &mouse)
	{
		return {
			.time = std::chrono::steady_clock::duration{100},
			.deltaTime = std::chrono::steady_clock::duration{10},
			.keyboard = keyboard,
			.mouse = mouse};
	}

	spk::KeyPressedEvent makeKeyEvent(const spk::Keyboard &keyboard)
	{
		spk::KeyPressedRecord record{};
		record.windowIdentifier = "window";
		record.key = spk::Keyboard::A;
		return spk::KeyPressedEvent(record, keyboard);
	}
}

TEST(EngineFacadeTest, RootAndGeometryAreStableAndGeometryPropagatesToAttachedEntities)
{
	spk::Engine engine;
	spk::Entity parent("parent");
	spk::Entity child("child", &parent);
	const spk::Rect2D geometry{.anchor = {12, 34}, .size = {640, 480}};

	EXPECT_EQ(&engine.root(), &std::as_const(engine).root());
	EXPECT_EQ(engine.root().context(), &engine);
	engine.addEntity(&parent);
	engine.handleGeometryChange(geometry);

	EXPECT_EQ(engine.geometry(), geometry);
	EXPECT_EQ(engine.root().geometry(), geometry);
	EXPECT_EQ(parent.geometry(), geometry);
	EXPECT_EQ(child.geometry(), geometry);
}

TEST(EngineFacadeTest, AddRemoveNullDuplicateAndDetachedEntityCallsAreIdempotent)
{
	spk::Engine engine;
	spk::Entity entity("entity");

	EXPECT_NO_THROW(engine.addEntity(nullptr));
	EXPECT_NO_THROW(engine.removeEntity(nullptr));
	EXPECT_NO_THROW(engine.removeEntity(&entity));
	engine.addEntity(&entity);
	engine.addEntity(&entity);
	EXPECT_EQ(entity.parent(), &engine.root());
	EXPECT_EQ(entity.context(), &engine);
	EXPECT_EQ(engine.root().children().size(), 1u);

	engine.removeEntity(&entity);
	engine.removeEntity(&entity);
	EXPECT_EQ(entity.parent(), nullptr);
	EXPECT_EQ(entity.context(), nullptr);
}

TEST(EngineFacadeTest, UpdatesAndEventsReachActiveEntitiesAndSystemsAndSkipInactiveOnes)
{
	spk::Engine engine;
	spk::Entity entity("entity");
	auto &behaviour = entity.addBehaviour<EngineBehaviour>();
	auto &activeSystem = engine.addSystem<EngineSystem>("active");
	auto &inactiveSystem = engine.addSystem<EngineSystem>("inactive");
	engine.addEntity(&entity);
	inactiveSystem.deactivate();

	spk::Keyboard keyboard;
	spk::Mouse mouse;
	auto update = makeUpdateContext(keyboard, mouse);
	engine.updateState(update);
	auto event = makeKeyEvent(keyboard);
	engine.dispatch(event);

	EXPECT_EQ(behaviour.updateCalls, 1u);
	EXPECT_EQ(behaviour.keyCalls, 1u);
	EXPECT_EQ(activeSystem.updateCalls, 1u);
	EXPECT_EQ(activeSystem.keyCalls, 1u);
	EXPECT_EQ(inactiveSystem.updateCalls, 0u);
	EXPECT_EQ(inactiveSystem.keyCalls, 0u);

	entity.deactivate();
	engine.updateState(update);
	auto secondEvent = makeKeyEvent(keyboard);
	engine.dispatch(secondEvent);
	EXPECT_EQ(behaviour.updateCalls, 1u);
	EXPECT_EQ(behaviour.keyCalls, 1u);
}

TEST(EngineFacadeTest, SystemsAttachToEngineAndCanBeRemovedCleanly)
{
	spk::Engine engine;
	auto &system = engine.addSystem<EngineSystem>("system");
	EXPECT_EQ(system.engine(), &engine);
	EXPECT_EQ(std::as_const(system).engine(), &engine);
	EXPECT_EQ(engine.getSystem<EngineSystem>(), &system);

	engine.removeSystem(system);
	EXPECT_EQ(engine.getSystem<EngineSystem>(), nullptr);
}

TEST(EngineFacadeTest, SnapshotExecutesPreSceneBeforeSceneRegardlessOfInsertionOrder)
{
	spk::Engine engine;
	std::vector<std::string> log;
	RenderEntity entity("render", log);
	engine.addEntity(&entity);
	spk::RenderSnapshot::Builder builder;
	engine.buildRenderSnapshot(builder);
	spk::RenderContext context{.targetSurface = nullptr};
	builder.build().execute(context);

	EXPECT_EQ(log, (std::vector<std::string>{"pre-scene", "scene"}));
}

TEST(EngineFacadeTest, DestroyingPopulatedEngineDestroysOwnedSystemsAndDetachesExternalEntities)
{
	spk::Entity entity("external");
	{
		spk::Engine engine;
		engine.addSystem<EngineSystem>();
		engine.addEntity(&entity);
		EXPECT_EQ(entity.context(), &engine);
	}

	EXPECT_EQ(entity.parent(), nullptr);
	EXPECT_EQ(entity.context(), nullptr);
}

TEST(EngineFacadeTest, DestructionClearsNestedAndDetachedEntityContextsAndDetachesSystemsBeforeDeletion)
{
	struct ObservedSystem : spk::System
	{
		bool &detached;
		bool &destroyed;
		ObservedSystem(bool &p_detached, bool &p_destroyed) : detached(p_detached), destroyed(p_destroyed) {}
		void attach(spk::Engine *value) override
		{
			spk::System::attach(value);
			if (value == nullptr) detached = true;
		}
		~ObservedSystem() override
		{
			EXPECT_EQ(engine(), nullptr);
			destroyed = true;
		}
	};
	bool detached = false;
	bool destroyed = false;
	spk::Entity parent("parent");
	spk::Entity child("child", &parent);
	spk::Entity unparented("unparented");
	auto &behaviour = child.addBehaviour<EngineBehaviour>();
	using Registry = spk::Registry<spk::Engine *, spk::Entity>;
	spk::Engine *oldEngine = nullptr;
	{
		spk::Engine engine;
		oldEngine = &engine;
		engine.addEntity(&parent);
		child.changeContext(&engine);
		unparented.changeContext(&engine);
		engine.addSystem<ObservedSystem>(detached, destroyed);
		ASSERT_EQ(behaviour.context(), &engine);
	}
	EXPECT_TRUE(detached);
	EXPECT_TRUE(destroyed);
	EXPECT_EQ(parent.parent(), nullptr);
	EXPECT_EQ(child.parent(), &parent);
	EXPECT_EQ(parent.context(), nullptr);
	EXPECT_EQ(child.context(), nullptr);
	EXPECT_EQ(unparented.context(), nullptr);
	EXPECT_EQ(behaviour.context(), nullptr);
	EXPECT_TRUE(Registry::elements(oldEngine).empty());
}

TEST(EngineFacadeTest, ModifyingSystemsDuringUpdateUsesStableTraversal)
{
	spk::Engine engine;
	std::size_t controllerCalls = 0;
	std::size_t removedCalls = 0;
	std::size_t addedCalls = 0;
	auto &controller = engine.addSystem<CallbackSystem>(controllerCalls);
	auto &removed = engine.addSystem<CallbackSystem>(removedCalls);
	CallbackSystem *added = nullptr;
	controller.callback = [&]() {
		controller.callback = {};
		engine.removeSystem(removed);
		added = &engine.addSystem<CallbackSystem>(addedCalls);
	};
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	auto update = makeUpdateContext(keyboard, mouse);

	engine.updateState(update);
	ASSERT_NE(added, nullptr);
	EXPECT_EQ(controllerCalls, 1u);
	EXPECT_EQ(removedCalls, 0u);
	EXPECT_EQ(addedCalls, 0u);

	engine.updateState(update);
	EXPECT_EQ(controllerCalls, 2u);
	EXPECT_EQ(addedCalls, 1u);
}

TEST(EngineFacadeTest, ModifyingEntitiesDuringUpdateUsesStableTraversal)
{
	spk::Engine engine;
	spk::Entity first("first");
	spk::Entity second("second");
	spk::Entity added("added");
	engine.addEntity(&first);
	engine.addEntity(&second);

	spk::Entity *controllerEntity = engine.root().children().front();
	spk::Entity *removedEntity = engine.root().children().back();
	std::size_t controllerCalls = 0;
	std::size_t removedCalls = 0;
	std::size_t addedCalls = 0;
	auto &controller = controllerEntity->addBehaviour<CallbackEngineBehaviour>(controllerCalls);
	removedEntity->addBehaviour<CallbackEngineBehaviour>(removedCalls);
	added.addBehaviour<CallbackEngineBehaviour>(addedCalls);
	controller.callback = [&]() {
		controller.callback = {};
		engine.removeEntity(removedEntity);
		engine.addEntity(&added);
	};
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	auto update = makeUpdateContext(keyboard, mouse);

	engine.updateState(update);
	EXPECT_EQ(controllerCalls, 1u);
	EXPECT_EQ(removedCalls, 0u);
	EXPECT_EQ(addedCalls, 0u);

	engine.updateState(update);
	EXPECT_EQ(controllerCalls, 2u);
	EXPECT_EQ(removedCalls, 0u);
	EXPECT_EQ(addedCalls, 1u);
}
