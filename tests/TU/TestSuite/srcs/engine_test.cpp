#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "container/query.hpp"
#include "core/context/render_context.hpp"
#include "core/context/update_context.hpp"
#include "engine/behaviour.hpp"
#include "engine/behaviour_collection.hpp"
#include "engine/engine.hpp"
#include "engine/entity.hpp"
#include "engine/entity2d.hpp"
#include "engine/entity3d.hpp"
#include "engine/query_operations.hpp"
#include "engine/registry.hpp"
#include "engine/system_collection.hpp"
#include "engine/component2d.hpp"
#include "engine/component3d.hpp"
#include "engine/component_collection.hpp"
#include "engine/transform2d.hpp"
#include "input/keyboard.hpp"
#include "input/mouse.hpp"
#include "query/operations.hpp"
#include "rendering/render_command.hpp"

namespace
{
	[[nodiscard]] spk::Engine *fakeEngine(std::uintptr_t value)
	{
		return reinterpret_cast<spk::Engine *>(value);
	}

	[[nodiscard]] spk::Rect2D testGeometry()
	{
		return {.anchor = {10, 20}, .size = {300, 120}};
	}

	[[nodiscard]] spk::UpdateContext updateContext(
		const spk::Keyboard &keyboard,
		const spk::Mouse &mouse)
	{
		return {
			.time = std::chrono::steady_clock::duration{42},
			.deltaTime = std::chrono::steady_clock::duration{7},
			.keyboard = keyboard,
			.mouse = mouse};
	}

	[[nodiscard]] spk::WindowMovedEvent windowMovedEvent()
	{
		spk::WindowMovedRecord record{};
		record.windowIdentifier = "window";
		return spk::WindowMovedEvent(record);
	}

	[[nodiscard]] spk::KeyPressedEvent keyPressedEvent(const spk::Keyboard &keyboard)
	{
		spk::KeyPressedRecord record{};
		record.windowIdentifier = "window";
		record.key = spk::Keyboard::A;
		return spk::KeyPressedEvent(record, keyboard);
	}

	class RecordingRenderCommand final : public spk::RenderCommand
	{
	private:
		std::vector<std::string> *_log;
		std::string _label;

	public:
		RecordingRenderCommand(std::vector<std::string> &log, std::string label) :
			_log(&log),
			_label(std::move(label))
		{
		}

		void execute(spk::RenderContext &) const override
		{
			_log->push_back(_label);
		}
	};

	class RecordingBehaviour : public spk::Behaviour
	{
	public:
		std::vector<std::string> *log = nullptr;
		std::size_t geometryCalls = 0;
		std::size_t updateCalls = 0;
		std::size_t renderCalls = 0;
		std::size_t keyCalls = 0;

		RecordingBehaviour(
			std::string name = "behaviour",
			std::vector<std::string> *p_log = nullptr) :
			spk::Behaviour(std::move(name)),
			log(p_log)
		{
		}

	protected:
		void _onGeometryChange(const spk::Rect2D &) override
		{
			++geometryCalls;
			if (log != nullptr)
			{
				log->push_back(name() + ":geometry");
			}
		}

		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder) override
		{
			++renderCalls;
			if (log != nullptr)
			{
				log->push_back(name() + ":render");
				builder.renderPass({"test", 0}).emplace<RecordingRenderCommand>(*log, name() + ":command");
			}
		}

		void _updateState(spk::UpdateContext &) override
		{
			++updateCalls;
			if (log != nullptr)
			{
				log->push_back(name() + ":update");
			}
		}

		void _onKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			++keyCalls;
			if (log != nullptr)
			{
				log->push_back(name() + ":key");
			}
		}
	};

	class OtherBehaviour final : public RecordingBehaviour
	{
	public:
		using RecordingBehaviour::RecordingBehaviour;
	};

	class CallbackBehaviour final : public spk::Behaviour
	{
	public:
		std::function<void()> callback;
		std::size_t *updateCalls;

		explicit CallbackBehaviour(std::size_t &p_updateCalls) :
			updateCalls(&p_updateCalls)
		{
		}

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++(*updateCalls);
			if (callback)
			{
				callback();
			}
		}
	};

	class DestructionBehaviour final : public spk::Behaviour
	{
	private:
		int *_destructionCount;

	public:
		explicit DestructionBehaviour(int &destructionCount) :
			_destructionCount(&destructionCount)
		{
		}

		~DestructionBehaviour() override
		{
			++(*_destructionCount);
		}
	};

	class RecordingComponent : public spk::Component
	{
	public:
		std::vector<std::string> *log = nullptr;
		std::size_t geometryCalls = 0;
		std::size_t renderCalls = 0;

		RecordingComponent(
			std::string name = "component",
			std::vector<std::string> *p_log = nullptr) :
			spk::Component(std::move(name)),
			log(p_log)
		{
		}

	protected:
		void _onGeometryChange(const spk::Rect2D &) override
		{
			++geometryCalls;
			if (log != nullptr)
			{
				log->push_back(name() + ":geometry");
			}
		}

		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder) override
		{
			++renderCalls;
			if (log != nullptr)
			{
				log->push_back(name() + ":render");
				builder.renderPass({"test", 0}).emplace<RecordingRenderCommand>(*log, name() + ":command");
			}
		}
	};

	class OtherComponent final : public RecordingComponent
	{
	public:
		using RecordingComponent::RecordingComponent;
	};

	class RecordingComponent2D final : public spk::Component2D
	{
	public:
		explicit RecordingComponent2D(std::string name = "component2d") :
			spk::Component2D(std::move(name))
		{
		}
	};

	class RecordingComponent3D final : public spk::Component3D
	{
	public:
		explicit RecordingComponent3D(std::string name = "component3d") :
			spk::Component3D(std::move(name))
		{
		}
	};

	class RecordingSystem : public spk::System
	{
	public:
		std::vector<std::string> *log = nullptr;
		int *destructionCount = nullptr;
		std::size_t updateCalls = 0;
		std::size_t keyCalls = 0;

		RecordingSystem(
			std::string name = "system",
			std::vector<std::string> *p_log = nullptr,
			int *p_destructionCount = nullptr) :
			spk::System(std::move(name)),
			log(p_log),
			destructionCount(p_destructionCount)
		{
		}

		~RecordingSystem() override
		{
			if (destructionCount != nullptr)
			{
				++(*destructionCount);
			}
		}

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++updateCalls;
			if (log != nullptr)
			{
				log->push_back(name() + ":update");
			}
		}

		void _onKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			++keyCalls;
			if (log != nullptr)
			{
				log->push_back(name() + ":key");
			}
		}
	};

	class OtherSystem final : public RecordingSystem
	{
	public:
		using RecordingSystem::RecordingSystem;
	};

	class RecordingEntity : public spk::Entity
	{
	public:
		std::vector<std::string> *log = nullptr;
		std::size_t geometryCalls = 0;
		std::size_t renderCalls = 0;

		RecordingEntity(std::string name, spk::Entity *parent = nullptr, std::vector<std::string> *p_log = nullptr) :
			spk::Entity(std::move(name), parent),
			log(p_log)
		{
		}

	protected:
		void _onGeometryChange(const spk::Rect2D &) override
		{
			++geometryCalls;
			if (log != nullptr)
			{
				log->push_back(name() + ":geometry");
			}
		}

		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder) override
		{
			++renderCalls;
			if (log != nullptr)
			{
				log->push_back(name() + ":render");
				builder.renderPass({"test", 0}).emplace<RecordingRenderCommand>(*log, name() + ":command");
			}
		}
	};

	class TestBehaviourCollection : public spk::BehaviourCollection
	{
	public:
		template <typename TBehaviour, typename... TArgs>
		TBehaviour &add(TArgs &&...args)
		{
			auto behaviour = std::make_unique<TBehaviour>(std::forward<TArgs>(args)...);
			TBehaviour &result = *behaviour;
			registerBehaviour(std::move(behaviour));
			return result;
		}

		void remove(spk::Behaviour &behaviour)
		{
			unregisterBehaviour(behaviour);
		}

		[[nodiscard]] std::size_t size() const
		{
			return behaviours().size();
		}
	};

	class TestComponentCollection : public spk::ComponentCollection
	{
	public:
		template <typename TComponent, typename... TArgs>
		TComponent &add(TArgs &&...args)
		{
			auto component = std::make_unique<TComponent>(std::forward<TArgs>(args)...);
			TComponent &result = *component;
			registerComponent(std::move(component));
			return result;
		}

		void remove(spk::Component &component)
		{
			unregisterComponent(component);
		}

		[[nodiscard]] std::size_t size() const
		{
			return components().size();
		}
	};

	class TestSystemCollection : public spk::SystemCollection
	{
	public:
		template <typename TSystem, typename... TArgs>
		TSystem &add(TArgs &&...args)
		{
			auto system = std::make_unique<TSystem>(std::forward<TArgs>(args)...);
			TSystem &result = *system;
			registerSystem(std::move(system));
			return result;
		}

		void remove(spk::System &system)
		{
			unregisterSystem(system);
		}

		[[nodiscard]] std::size_t size() const
		{
			return systems().size();
		}
	};
}

TEST(BehaviourCollectionTest, AppliesAttachmentCollectionMatrixToBehaviours)
{
	TestBehaviourCollection collection;
	std::vector<std::string> additions;
	std::vector<std::string> removals;
	auto addContract = collection.subscribeToBehaviourAddition(
		[&](spk::Behaviour &behaviour) {
			additions.push_back(behaviour.name());
		});
	auto removeContract = collection.subscribeToBehaviourRemoval(
		[&](spk::Behaviour &behaviour) {
			removals.push_back(behaviour.name());
		});

	RecordingBehaviour &first = collection.add<RecordingBehaviour>("player.move");
	OtherBehaviour &second = collection.add<OtherBehaviour>("enemy.move");
	RecordingBehaviour &third = collection.add<RecordingBehaviour>("enemy.ai");

	EXPECT_EQ(additions, (std::vector<std::string>{"player.move", "enemy.move", "enemy.ai"}));
	EXPECT_EQ(collection.getBehaviour<RecordingBehaviour>(), &first);
	EXPECT_EQ(collection.getBehaviour<OtherBehaviour>(), &second);
	EXPECT_EQ(collection.getBehaviour<RecordingBehaviour>([](RecordingBehaviour *behaviour) {
		return behaviour->name() == "enemy.ai";
	}),
			  &third);

	const std::regex enemyRegex(R"(^enemy\..+$)");
	const auto enemies = collection.getBehaviours<RecordingBehaviour>(enemyRegex);
	ASSERT_EQ(enemies.size(), 2u);
	EXPECT_EQ(enemies[0], &second);
	EXPECT_EQ(enemies[1], &third);

	const TestBehaviourCollection &constCollection = collection;
	EXPECT_EQ(constCollection.getBehaviour<RecordingBehaviour>(enemyRegex), &second);
	ASSERT_EQ(constCollection.getBehaviours<RecordingBehaviour>().size(), 3u);

	collection.remove(second);
	EXPECT_EQ(removals, (std::vector<std::string>{"enemy.move"}));
	EXPECT_EQ(collection.size(), 2u);
	EXPECT_EQ(collection.getBehaviour<OtherBehaviour>(), nullptr);
}

TEST(ComponentCollectionTest, AppliesAttachmentCollectionMatrixToComponents)
{
	TestComponentCollection collection;
	std::vector<std::string> additions;
	std::vector<std::string> removals;
	auto addContract = collection.subscribeToComponentAddition(
		[&](spk::Component &component) {
			additions.push_back(component.name());
		});
	auto removeContract = collection.subscribeToComponentRemoval(
		[&](spk::Component &component) {
			removals.push_back(component.name());
		});

	RecordingComponent &base = collection.add<RecordingComponent>("physics.body");
	OtherComponent &other = collection.add<OtherComponent>("physics.sensor");
	RecordingComponent &late = collection.add<RecordingComponent>("render.mesh");

	EXPECT_EQ(additions, (std::vector<std::string>{"physics.body", "physics.sensor", "render.mesh"}));
	EXPECT_EQ(collection.getComponent<RecordingComponent>(), &base);
	EXPECT_EQ(collection.getComponent<OtherComponent>(), &other);

	const std::regex physicsRegex(R"(^physics\..+$)");
	const auto physics = collection.getComponents<RecordingComponent>(physicsRegex);
	ASSERT_EQ(physics.size(), 2u);
	EXPECT_EQ(physics[0], &base);
	EXPECT_EQ(physics[1], &other);

	collection.remove(base);
	EXPECT_EQ(removals, (std::vector<std::string>{"physics.body"}));
	EXPECT_EQ(collection.getComponent<RecordingComponent>(), &other);

	collection.remove(other);
	EXPECT_EQ(collection.getComponent<OtherComponent>(), nullptr);
	EXPECT_EQ(collection.getComponent<RecordingComponent>(), &late);

	const TestComponentCollection &constCollection = collection;
	ASSERT_EQ(constCollection.getComponents<RecordingComponent>().size(), 1u);
	EXPECT_EQ(constCollection.getComponent<RecordingComponent>(), &late);
}

TEST(ComponentCollectionTest, Typed2DAnd3DQueriesSurviveCacheInvalidation)
{
	TestComponentCollection collection;
	EXPECT_EQ(collection.getComponent<RecordingComponent2D>(), nullptr);
	EXPECT_TRUE(collection.getComponents<RecordingComponent3D>().empty());

	RecordingComponent2D &component2D = collection.add<RecordingComponent2D>("sprite.transform");
	RecordingComponent3D &component3D = collection.add<RecordingComponent3D>("mesh.transform");

	EXPECT_EQ(collection.getComponent<RecordingComponent2D>(), &component2D);
	EXPECT_EQ(collection.getComponent<RecordingComponent3D>(), &component3D);

	collection.remove(component2D);
	EXPECT_EQ(collection.getComponent<RecordingComponent2D>(), nullptr);
	EXPECT_EQ(collection.getComponent<RecordingComponent3D>(), &component3D);
}

TEST(SystemCollectionTest, AppliesPolymorphicCollectionMatrixToSystems)
{
	TestSystemCollection collection;
	std::vector<std::string> additions;
	std::vector<std::string> removals;
	auto addContract = collection.subscribeToSystemAddition(
		[&](spk::System &system) {
			additions.push_back(system.name());
		});
	auto removeContract = collection.subscribeToSystemRemoval(
		[&](spk::System &system) {
			removals.push_back(system.name());
		});

	RecordingSystem &render = collection.add<RecordingSystem>("render");
	OtherSystem &physics = collection.add<OtherSystem>("physics");
	RecordingSystem &ai = collection.add<RecordingSystem>("ai");

	EXPECT_EQ(additions, (std::vector<std::string>{"render", "physics", "ai"}));
	EXPECT_EQ(collection.getSystem<RecordingSystem>(), &render);
	EXPECT_EQ(collection.getSystem<OtherSystem>(), &physics);
	EXPECT_EQ(collection.getSystem<RecordingSystem>([](RecordingSystem *system) {
		return system->name() == "ai";
	}),
			  &ai);

	const std::regex shortNameRegex(R"(^[a-z]{2}$)");
	const auto shortNamedSystems = collection.getSystems<RecordingSystem>(shortNameRegex);
	ASSERT_EQ(shortNamedSystems.size(), 1u);
	EXPECT_EQ(shortNamedSystems.front(), &ai);

	const TestSystemCollection &constCollection = collection;
	EXPECT_EQ(constCollection.getSystem<RecordingSystem>(std::regex("physics")), &physics);
	ASSERT_EQ(constCollection.getSystems<RecordingSystem>().size(), 3u);

	RecordingSystem foreign("foreign");
	collection.remove(foreign);
	EXPECT_EQ(collection.size(), 3u);

	collection.remove(render);
	EXPECT_EQ(removals, (std::vector<std::string>{"render"}));
	EXPECT_EQ(collection.getSystem<RecordingSystem>(), &physics);
}

TEST(BehaviourTest, StandardUsagePropagatesGeometryUpdateRenderAndInteractionsWhileActive)
{
	std::vector<std::string> log;
	RecordingBehaviour behaviour("recorder", &log);
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	auto context = updateContext(keyboard, mouse);

	behaviour.setGeometry(testGeometry());
	behaviour.updateState(context);
	spk::RenderSnapshot::Builder builder;
	behaviour.buildRenderSnapshot(builder);
	auto keyEvent = keyPressedEvent(keyboard);
	behaviour.dispatch(keyEvent);
	spk::RenderContext renderContext{.targetSurface = nullptr};
	builder.build().execute(renderContext);

	EXPECT_EQ(behaviour.geometry(), testGeometry());
	EXPECT_EQ(behaviour.geometryCalls, 1u);
	EXPECT_EQ(behaviour.updateCalls, 1u);
	EXPECT_EQ(behaviour.renderCalls, 1u);
	EXPECT_EQ(behaviour.keyCalls, 1u);
	EXPECT_EQ(log, (std::vector<std::string>{"recorder:geometry", "recorder:update", "recorder:render", "recorder:key", "recorder:command"}));
}

TEST(BehaviourTest, NullOwnerDeactivateReactivateRepeatedGeometryAndHookOrderingAreObservable)
{
	std::vector<std::string> log;
	RecordingBehaviour behaviour("recorder", &log);
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	auto context = updateContext(keyboard, mouse);

	EXPECT_EQ(behaviour.owner(), nullptr);
	EXPECT_EQ(behaviour.context(), nullptr);

	behaviour.deactivate();
	behaviour.setGeometry(testGeometry());
	behaviour.setGeometry(testGeometry());
	behaviour.updateState(context);
	spk::RenderSnapshot::Builder inactiveBuilder;
	behaviour.buildRenderSnapshot(inactiveBuilder);
	auto inactiveEvent = keyPressedEvent(keyboard);
	behaviour.dispatch(inactiveEvent);

	EXPECT_EQ(behaviour.geometryCalls, 2u);
	EXPECT_EQ(behaviour.updateCalls, 0u);
	EXPECT_EQ(behaviour.renderCalls, 0u);
	EXPECT_EQ(behaviour.keyCalls, 0u);

	behaviour.activate();
	behaviour.updateState(context);
	spk::RenderSnapshot::Builder activeBuilder;
	behaviour.buildRenderSnapshot(activeBuilder);
	auto activeEvent = keyPressedEvent(keyboard);
	behaviour.dispatch(activeEvent);

	EXPECT_EQ(behaviour.updateCalls, 1u);
	EXPECT_EQ(behaviour.renderCalls, 1u);
	EXPECT_EQ(behaviour.keyCalls, 1u);
	EXPECT_EQ(log, (std::vector<std::string>{"recorder:geometry", "recorder:geometry", "recorder:update", "recorder:render", "recorder:key"}));
}

TEST(BehaviourTest, DirectDispatchShouldRejectInteractionWhenOwnerIsInactive)
{
	spk::Entity owner("owner");
	RecordingBehaviour &behaviour = owner.addBehaviour<RecordingBehaviour>("behaviour");
	spk::Keyboard keyboard;

	owner.deactivate();
	auto inactiveEvent = keyPressedEvent(keyboard);
	behaviour.dispatch(inactiveEvent);
	EXPECT_EQ(behaviour.keyCalls, 0u);
	EXPECT_FALSE(inactiveEvent.consumed);

	owner.activate();
	auto activeEvent = keyPressedEvent(keyboard);
	behaviour.dispatch(activeEvent);
	EXPECT_EQ(behaviour.keyCalls, 1u);
}

TEST(SystemTest, StandardUsageAddsToEngineUpdatesActiveSystemsAndDispatchesEvents)
{
	std::vector<std::string> log;
	spk::Engine engine;
	RecordingSystem &system = engine.addSystem<RecordingSystem>("system", &log);
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	auto context = updateContext(keyboard, mouse);

	EXPECT_EQ(system.engine(), &engine);
	EXPECT_EQ(engine.getSystem<RecordingSystem>(), &system);

	engine.updateState(context);
	auto keyEvent = keyPressedEvent(keyboard);
	engine.dispatch(keyEvent);

	EXPECT_EQ(system.updateCalls, 1u);
	EXPECT_EQ(system.keyCalls, 1u);
	EXPECT_EQ(log, (std::vector<std::string>{"system:update", "system:key"}));
}

TEST(SystemTest, NullEngineReattachmentDeactivationRepeatedAttachAndRemovalAreObservable)
{
	spk::Engine engine;
	RecordingSystem detached("detached");
	int destructionCount = 0;
	RecordingSystem &owned = engine.addSystem<RecordingSystem>("owned", nullptr, &destructionCount);
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	auto context = updateContext(keyboard, mouse);

	EXPECT_EQ(detached.engine(), nullptr);
	detached.attach(&engine);
	EXPECT_EQ(detached.engine(), &engine);
	detached.attach(&engine);
	EXPECT_EQ(detached.engine(), &engine);
	detached.attach(nullptr);
	EXPECT_EQ(detached.engine(), nullptr);

	owned.deactivate();
	engine.updateState(context);
	EXPECT_EQ(owned.updateCalls, 0u);
	owned.activate();
	engine.updateState(context);
	EXPECT_EQ(owned.updateCalls, 1u);

	engine.removeSystem(owned);
	EXPECT_EQ(destructionCount, 1);
	EXPECT_EQ(engine.getSystem<RecordingSystem>(), nullptr);
}

TEST(ComponentTest, StandardUsageAttachesToEntityReceivesGeometryRendersAndAppearsInRegistries)
{
	spk::Engine engine;
	spk::Entity entity("entity");
	engine.addEntity(&entity);
	std::vector<std::string> log;

	RecordingComponent &component = entity.addComponent<RecordingComponent>("component", &log);
	entity.setGeometry(testGeometry());
	spk::RenderSnapshot::Builder builder;
	entity.buildRenderSnapshot(builder);
	spk::RenderContext renderContext{.targetSurface = nullptr};
	builder.build().execute(renderContext);

	EXPECT_EQ(component.owner(), &entity);
	EXPECT_EQ(component.context(), &engine);
	EXPECT_TRUE((spk::Registry<spk::Component, spk::Engine *>::instance().elements(&engine).contains(&component)));
	EXPECT_EQ(component.geometry(), testGeometry());
	EXPECT_EQ(component.geometryCalls, 2u);
	EXPECT_EQ(component.renderCalls, 1u);
	EXPECT_EQ(log, (std::vector<std::string>{"component:geometry", "component:geometry", "component:render", "component:command"}));
}

TEST(ComponentTest, NullOwnerActiveInactiveReattachmentRemovalAndHookOrderingAreObservable)
{
	spk::Engine engine;
	spk::Entity first("first");
	spk::Entity second("second");
	engine.addEntity(&first);
	engine.addEntity(&second);
	RecordingComponent component("component");

	EXPECT_EQ(component.owner(), nullptr);
	EXPECT_EQ(component.context(), nullptr);

	component.attach(&first);
	EXPECT_EQ(component.owner(), &first);
	EXPECT_EQ(component.context(), &engine);
	component.attach(&second);
	EXPECT_EQ(component.owner(), &second);
	EXPECT_EQ(component.context(), &engine);

	component.deactivate();
	spk::RenderSnapshot::Builder inactiveBuilder;
	component.buildRenderSnapshot(inactiveBuilder);
	EXPECT_EQ(component.renderCalls, 0u);

	component.activate();
	spk::RenderSnapshot::Builder activeBuilder;
	component.buildRenderSnapshot(activeBuilder);
	EXPECT_EQ(component.renderCalls, 1u);

	RecordingComponent &owned = second.addComponent<RecordingComponent>("owned");
	EXPECT_EQ(second.getComponent<RecordingComponent>(), &owned);
	second.removeComponent(owned);
	EXPECT_EQ(second.getComponent<RecordingComponent>(), nullptr);
}

TEST(Component2DTest, StandardUsageAttachesToEntity2DWithCovariantOwnerAndTypedRegistryMembership)
{
	spk::Engine engine;
	spk::Entity2D entity("entity2d");
	engine.addEntity(&entity);

	RecordingComponent2D &component = entity.addComponent<RecordingComponent2D>("sprite");

	EXPECT_EQ(component.owner(), &entity);
	EXPECT_EQ(std::as_const(component).owner(), &entity);
	EXPECT_EQ(entity.getComponent<RecordingComponent2D>(), &component);
	EXPECT_TRUE((spk::Registry<spk::Component2D, spk::Engine *>::instance().elements(&engine).contains(&component)));
}

TEST(Component2DTest, AttachToPlainOr3DEntityThrowsAndPreservesPriorOwnershipAndContext)
{
	spk::Engine engine;
	spk::Entity plain("plain");
	spk::Entity3D entity3D("entity3d");
	spk::Entity2D prior("prior");
	engine.addEntity(&plain);
	engine.addEntity(&entity3D);
	engine.addEntity(&prior);

	RecordingComponent2D component("sprite");
	component.attach(&prior);

	EXPECT_THROW(component.attach(&plain), std::invalid_argument);
	EXPECT_EQ(component.owner(), &prior);
	EXPECT_EQ(component.context(), &engine);
	EXPECT_THROW(component.attach(&entity3D), std::invalid_argument);
	EXPECT_EQ(component.owner(), &prior);
	EXPECT_EQ(component.context(), &engine);
}

TEST(Component3DTest, StandardUsageAttachesToEntity3DWithCovariantOwnerAndTypedRegistryMembership)
{
	spk::Engine engine;
	spk::Entity3D entity("entity3d");
	engine.addEntity(&entity);

	RecordingComponent3D &component = entity.addComponent<RecordingComponent3D>("mesh");

	EXPECT_EQ(component.owner(), &entity);
	EXPECT_EQ(std::as_const(component).owner(), &entity);
	EXPECT_EQ(entity.getComponent<RecordingComponent3D>(), &component);
	EXPECT_TRUE((spk::Registry<spk::Component3D, spk::Engine *>::instance().elements(&engine).contains(&component)));
}

TEST(Component3DTest, AttachToPlainOr2DEntityThrowsAndPreservesPriorOwnershipAndContext)
{
	spk::Engine engine;
	spk::Entity plain("plain");
	spk::Entity2D entity2D("entity2d");
	spk::Entity3D prior("prior");
	engine.addEntity(&plain);
	engine.addEntity(&entity2D);
	engine.addEntity(&prior);

	RecordingComponent3D component("mesh");
	component.attach(&prior);

	EXPECT_THROW(component.attach(&plain), std::invalid_argument);
	EXPECT_EQ(component.owner(), &prior);
	EXPECT_EQ(component.context(), &engine);
	EXPECT_THROW(component.attach(&entity2D), std::invalid_argument);
	EXPECT_EQ(component.owner(), &prior);
	EXPECT_EQ(component.context(), &engine);
}

TEST(EntityTest, StandardUsageHierarchyAttachmentsEngineGeometryUpdateRenderEventsAndCleanRemoval)
{
	std::vector<std::string> log;
	spk::Engine engine;
	RecordingEntity parent("parent", nullptr, &log);
	RecordingEntity child("child", &parent, &log);
	RecordingBehaviour &behaviour = parent.addBehaviour<RecordingBehaviour>("behaviour", &log);
	RecordingComponent &component = parent.addComponent<RecordingComponent>("component", &log);
	RecordingBehaviour &childBehaviour = child.addBehaviour<RecordingBehaviour>("child.behaviour", &log);
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	auto context = updateContext(keyboard, mouse);

	engine.addEntity(&parent);
	engine.handleGeometryChange(testGeometry());
	engine.updateState(context);
	spk::RenderSnapshot::Builder builder;
	engine.buildRenderSnapshot(builder);
	auto keyEvent = keyPressedEvent(keyboard);
	engine.dispatch(keyEvent);
	spk::RenderContext renderContext{.targetSurface = nullptr};
	builder.build().execute(renderContext);

	EXPECT_EQ(parent.parent(), &engine.root());
	EXPECT_EQ(child.parent(), &parent);
	EXPECT_EQ(parent.context(), &engine);
	EXPECT_EQ(behaviour.owner(), &parent);
	EXPECT_EQ(component.owner(), &parent);
	EXPECT_EQ(parent.geometry(), testGeometry());
	EXPECT_EQ(child.geometry(), testGeometry());
	EXPECT_EQ(behaviour.updateCalls, 1u);
	EXPECT_EQ(childBehaviour.updateCalls, 1u);
	EXPECT_EQ(behaviour.keyCalls, 1u);
	EXPECT_EQ(childBehaviour.keyCalls, 1u);

	engine.removeEntity(&parent);
	EXPECT_EQ(parent.parent(), nullptr);
	EXPECT_EQ(parent.context(), nullptr);
}

TEST(EntityTest, InactiveBranchesSkipUpdateRenderAndInteraction)
{
	std::vector<std::string> log;
	spk::Engine engine;
	RecordingEntity parent("parent", nullptr, &log);
	RecordingEntity child("child", &parent, &log);
	RecordingBehaviour &behaviour = parent.addBehaviour<RecordingBehaviour>("behaviour", &log);
	RecordingBehaviour &childBehaviour = child.addBehaviour<RecordingBehaviour>("child.behaviour", &log);
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	auto context = updateContext(keyboard, mouse);
	engine.addEntity(&parent);

	parent.deactivate();
	engine.updateState(context);
	spk::RenderSnapshot::Builder builder;
	engine.buildRenderSnapshot(builder);
	auto keyEvent = keyPressedEvent(keyboard);
	engine.dispatch(keyEvent);

	EXPECT_EQ(behaviour.updateCalls, 0u);
	EXPECT_EQ(childBehaviour.updateCalls, 0u);
	EXPECT_EQ(behaviour.renderCalls, 0u);
	EXPECT_EQ(childBehaviour.renderCalls, 0u);
	EXPECT_EQ(behaviour.keyCalls, 0u);
	EXPECT_EQ(childBehaviour.keyCalls, 0u);
}

TEST(EntityTest, ContextParentDuplicateNamesDuplicateTypesAndRegistryQueriesAreObservable)
{
	spk::Engine engine;
	spk::Entity detached("duplicate");
	spk::Entity parent("parent");
	spk::Entity child("duplicate", &parent);

	EXPECT_EQ(detached.context(), nullptr);
	EXPECT_TRUE((spk::Registry<spk::Entity, spk::Engine *>::instance().elements(nullptr).contains(&detached)));

	engine.addEntity(&parent);
	child.changeContext(&engine);
	EXPECT_TRUE((spk::Registry<spk::Entity, spk::Engine *>::instance().elements(&engine).contains(&parent)));
	EXPECT_TRUE((spk::Registry<spk::Entity, spk::Engine *>::instance().elements(&engine).contains(&child)));

	RecordingBehaviour &first = child.addBehaviour<RecordingBehaviour>("same");
	RecordingBehaviour &second = child.addBehaviour<RecordingBehaviour>("same");
	EXPECT_EQ(child.getBehaviour<RecordingBehaviour>(), &first);
	ASSERT_EQ(child.getBehaviours<RecordingBehaviour>(std::regex("same")).size(), 2u);

	spk::Query<spk::Entity, spk::Engine *> query;
	query.insert<spk::FromRegistry<spk::Entity>>()
		.insert<spk::ContainBehaviour<RecordingBehaviour>>();
	EXPECT_TRUE(query.elements(&engine).contains(&child));

	child.removeBehaviour(first);
	EXPECT_TRUE(query.elements(&engine).contains(&child));
	child.removeBehaviour(second);
	EXPECT_FALSE(query.elements(&engine).contains(&child));
}

TEST(EntityTest, DestructionRemovesEntityAndOwnedAttachmentsFromRegistries)
{
	spk::Engine engine;
	auto parent = std::make_unique<spk::Entity>("parent");
	engine.addEntity(parent.get());
	ASSERT_TRUE((spk::Registry<spk::Entity, spk::Engine *>::instance().elements(&engine).contains(parent.get())));

	parent.reset();

	EXPECT_FALSE((spk::Registry<spk::Entity, spk::Engine *>::instance().elements(&engine).contains(parent.get())));
}

TEST(EntityTest, DestructionDestroysOwnedAttachmentsAndDetachesNonOwnedChildren)
{
	int attachmentDestructions = 0;
	spk::Entity child("child");
	{
		auto parent = std::make_unique<spk::Entity>("parent");
		child.setParent(*parent);
		parent->addBehaviour<DestructionBehaviour>(attachmentDestructions);
	}

	EXPECT_EQ(attachmentDestructions, 1);
	EXPECT_EQ(child.parent(), nullptr);
	EXPECT_EQ(child.context(), nullptr);
}

TEST(EntityTest, ContextChangesPropagateThroughExistingDescendantsAndAttachments)
{
	spk::Engine engine;
	spk::Entity parent("parent");
	spk::Entity child("child", &parent);
	spk::Entity grandchild("grandchild", &child);
	auto &behaviour = child.addBehaviour<RecordingBehaviour>();

	parent.changeContext(&engine);
	EXPECT_EQ(child.context(), &engine);
	EXPECT_EQ(grandchild.context(), &engine);
	EXPECT_EQ(behaviour.context(), &engine);

	parent.changeContext(nullptr);
	EXPECT_EQ(child.context(), nullptr);
	EXPECT_EQ(grandchild.context(), nullptr);
	EXPECT_EQ(behaviour.context(), nullptr);
}

TEST(EntityTest, AttachmentEditsDuringUpdateUseStableTraversal)
{
	spk::Entity entity("entity");
	std::size_t controllerCalls = 0;
	std::size_t removedCalls = 0;
	std::size_t addedCalls = 0;
	auto &controller = entity.addBehaviour<CallbackBehaviour>(controllerCalls);
	auto &removed = entity.addBehaviour<CallbackBehaviour>(removedCalls);
	CallbackBehaviour *added = nullptr;
	controller.callback = [&]() {
		controller.callback = {};
		entity.removeBehaviour(removed);
		added = &entity.addBehaviour<CallbackBehaviour>(addedCalls);
	};

	spk::Keyboard keyboard;
	spk::Mouse mouse;
	auto context = updateContext(keyboard, mouse);
	entity.updateState(context);

	ASSERT_NE(added, nullptr);
	EXPECT_EQ(controllerCalls, 1u);
	EXPECT_EQ(removedCalls, 0u);
	EXPECT_EQ(addedCalls, 0u);

	entity.updateState(context);
	EXPECT_EQ(controllerCalls, 2u);
	EXPECT_EQ(addedCalls, 1u);
}

TEST(Entity2DTest, StandardUsageConstructsTransformTypedRegistriesAndParentTransformRelationship)
{
	spk::Engine engine;
	spk::Entity2D parent("parent");
	parent.transform().place({10.0f, 20.0f});
	spk::Entity2D child("child", &parent);
	child.transform().place({3.0f, 4.0f});
	engine.addEntity(&parent);
	child.changeContext(&engine);

	EXPECT_EQ(parent.getComponent<spk::Transform2D>(), &parent.transform());
	EXPECT_EQ(child.getComponent<spk::Transform2D>(), &child.transform());
	EXPECT_EQ(parent.transform().owner(), &parent);
	EXPECT_EQ(std::as_const(child).transform().owner(), &child);
	EXPECT_TRUE((spk::Registry<spk::Entity2D, spk::Engine *>::instance().elements(&engine).contains(&parent)));
	EXPECT_TRUE((spk::Registry<spk::Transform2D, spk::Engine *>::instance().elements(&engine).contains(&child.transform())));
	EXPECT_EQ(child.transform().position(spk::ReferenceFrame::World), spk::Vector2(13.0f, 24.0f));
}

TEST(Entity2DTest, ReparentingPlain2D3DContextChangesTransformLifetimeAndAccessAreObservable)
{
	spk::Engine engine;
	spk::Entity plain("plain");
	spk::Entity2D parent2D("parent2d");
	spk::Entity3D parent3D("parent3d");
	spk::Entity2D child("child");
	spk::Transform2D *transform = &child.transform();

	parent2D.transform().place({5.0f, 6.0f});
	child.transform().place({1.0f, 2.0f});
	engine.addEntity(&plain);
	engine.addEntity(&parent2D);
	engine.addEntity(&parent3D);
	engine.addEntity(&child);

	child.setParent(parent2D);
	EXPECT_EQ(child.transform().position(spk::ReferenceFrame::World), spk::Vector2(6.0f, 8.0f));

	child.setParent(plain);
	EXPECT_EQ(child.transform().position(spk::ReferenceFrame::World), spk::Vector2(1.0f, 2.0f));

	child.setParent(parent3D);
	EXPECT_EQ(child.transform().position(spk::ReferenceFrame::World), spk::Vector2(1.0f, 2.0f));
	EXPECT_EQ(&child.transform(), transform);
	EXPECT_EQ(&std::as_const(child).transform(), transform);

	engine.removeEntity(&child);
	EXPECT_EQ(child.context(), nullptr);
	EXPECT_EQ(transform->context(), nullptr);
}
