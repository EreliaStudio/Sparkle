#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "core/context/render_context.hpp"
#include "core/context/update_context.hpp"
#include "engine/engine.hpp"
#include "input/device_context.hpp"
#include "input/keyboard.hpp"
#include "input/mouse.hpp"
#include "rendering/engine/rendering_behaviour.hpp"
#include "rendering/engine/rendering_component.hpp"
#include "rendering/engine/rendering_engine.hpp"
#include "rendering/engine/rendering_entity.hpp"
#include "rendering/engine/rendering_system.hpp"
#include "rendering/render_command.hpp"

namespace
{
	class MarkCommand final : public spk::RenderCommand
	{
	private:
		std::vector<std::string> *_log;
		std::string _name;

	public:
		MarkCommand(
			std::vector<std::string> &log,
			std::string name) :
			_log(&log),
			_name(std::move(name))
		{
		}

		void execute(spk::RenderContext &) const override
		{
			_log->push_back(_name);
		}
	};

	template <typename TBase>
	class RenderingProbe : public TBase
	{
	private:
		std::string _marker;

	public:
		std::size_t deviceUpdates = 0;
		std::size_t keyEvents = 0;
		std::vector<std::string> *renderLog = nullptr;

		explicit RenderingProbe(const std::string &name) :
			TBase(name),
			_marker(name)
		{
		}

	protected:
		void _updateState(
			spk::UpdateContext &,
			spk::DeviceContext &) override
		{
			++deviceUpdates;
		}

		void _onKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			++keyEvents;
		}

		void _buildRenderSnapshot(
			spk::RenderSnapshot::Builder &builder) override
		{
			if (renderLog != nullptr)
			{
				builder.renderPass(spk::RenderingEngine::SceneRenderPassKey)
					.emplace<MarkCommand>(*renderLog, _marker);
			}
		}
	};

	using ProbeEntity = RenderingProbe<spk::RenderingEntity3D>;
	using ProbeComponent = RenderingProbe<spk::RenderingComponent3D>;
	using ProbeBehaviour = RenderingProbe<spk::RenderingBehaviour3D>;
	using ProbeSystem = RenderingProbe<spk::RenderingSystem>;

	class MutatingSystem final : public spk::RenderingSystem
	{
	public:
		std::function<void()> callback;
		std::size_t updates = 0;

		using RenderingSystem::RenderingSystem;

	protected:
		void _updateState(
			spk::UpdateContext &,
			spk::DeviceContext &) override
		{
			++updates;
			if (callback)
			{
				callback();
			}
		}
	};
}

TEST(RenderingEngineTest, GraphicalExtensionsReuseTheCoreEngineObjects)
{
	spk::Engine engine;
	ProbeEntity entity("entity");
	auto &component = entity.addComponent<ProbeComponent>("component");
	auto &behaviour = entity.addBehaviour<ProbeBehaviour>("behaviour");
	auto &system = engine.addSystem<ProbeSystem>("system");
	engine.addEntity(&entity);

	spk::RenderingEngine renderingEngine(&engine);
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext updateContext{};
	spk::DeviceContext deviceContext{
		.keyboard = keyboard,
		.mouse = mouse};

	renderingEngine.updateState(updateContext, deviceContext);

	EXPECT_EQ(entity.deviceUpdates, 1u);
	EXPECT_EQ(component.deviceUpdates, 1u);
	EXPECT_EQ(behaviour.deviceUpdates, 1u);
	EXPECT_EQ(system.deviceUpdates, 1u);
	EXPECT_EQ(entity.context(), &engine);
	EXPECT_EQ(component.context(), &engine);
	EXPECT_EQ(behaviour.context(), &engine);
	EXPECT_EQ(system.engine(), &engine);
}

TEST(RenderingEngineTest, EventsReachOnlyRenderingExtensions)
{
	spk::Engine engine;
	ProbeEntity entity("entity");
	auto &component = entity.addComponent<ProbeComponent>("component");
	auto &behaviour = entity.addBehaviour<ProbeBehaviour>("behaviour");
	auto &system = engine.addSystem<ProbeSystem>("system");
	engine.addEntity(&entity);
	spk::RenderingEngine renderingEngine(&engine);

	spk::Keyboard keyboard;
	spk::KeyPressedRecord record{};
	record.key = spk::Keyboard::A;
	spk::KeyPressedEvent event(record, keyboard);
	renderingEngine.dispatch(event);

	EXPECT_EQ(entity.keyEvents, 1u);
	EXPECT_EQ(component.keyEvents, 1u);
	EXPECT_EQ(behaviour.keyEvents, 1u);
	EXPECT_EQ(system.keyEvents, 1u);
}

TEST(RenderingEngineTest, SnapshotCollectsEveryRenderingExtension)
{
	spk::Engine engine;
	ProbeEntity entity("entity");
	auto &component = entity.addComponent<ProbeComponent>("component");
	auto &behaviour = entity.addBehaviour<ProbeBehaviour>("behaviour");
	auto &system = engine.addSystem<ProbeSystem>("system");
	engine.addEntity(&entity);

	std::vector<std::string> log;
	entity.renderLog = &log;
	component.renderLog = &log;
	behaviour.renderLog = &log;
	system.renderLog = &log;

	spk::RenderingEngine renderingEngine(&engine);
	spk::RenderSnapshot::Builder builder;
	renderingEngine.buildRenderSnapshot(builder);
	spk::RenderContext renderContext{.targetSurface = nullptr};
	builder.build().execute(renderContext);

	EXPECT_EQ(
		log,
		(std::vector<std::string>{
			"entity",
			"component",
			"behaviour",
			"system"}));
}

TEST(RenderingEngineTest, SystemTraversalRemainsStableDuringMutation)
{
	spk::Engine engine;
	auto &controller = engine.addSystem<MutatingSystem>("controller");
	auto &removed = engine.addSystem<MutatingSystem>("removed");
	MutatingSystem *added = nullptr;
	bool mutated = false;

	controller.callback = [&]() {
		if (mutated)
		{
			return;
		}

		mutated = true;
		engine.removeSystem(removed);
		added = &engine.addSystem<MutatingSystem>("added");
	};

	spk::RenderingEngine renderingEngine(&engine);
	spk::UpdateContext updateContext{};
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::DeviceContext deviceContext{.keyboard = keyboard, .mouse = mouse};
	renderingEngine.updateState(updateContext, deviceContext);

	ASSERT_NE(added, nullptr);
	EXPECT_TRUE(added->isActive());
	EXPECT_EQ(controller.updates, 1u);
	EXPECT_EQ(added->updates, 0u);

	renderingEngine.updateState(updateContext, deviceContext);
	EXPECT_EQ(controller.updates, 2u);
	EXPECT_EQ(added->updates, 1u);
}

TEST(RenderingEngineTest, InactiveObjectsAreSkippedByGraphicalContracts)
{
	spk::Engine engine;
	ProbeEntity entity("entity");
	auto &component = entity.addComponent<ProbeComponent>("component");
	auto &behaviour = entity.addBehaviour<ProbeBehaviour>("behaviour");
	auto &system = engine.addSystem<ProbeSystem>("system");
	engine.addEntity(&entity);
	entity.deactivate();
	system.deactivate();

	spk::RenderingEngine renderingEngine(&engine);
	spk::UpdateContext updateContext{};
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::DeviceContext deviceContext{.keyboard = keyboard, .mouse = mouse};
	renderingEngine.updateState(updateContext, deviceContext);

	std::vector<std::string> log;
	entity.renderLog = &log;
	component.renderLog = &log;
	behaviour.renderLog = &log;
	system.renderLog = &log;
	spk::RenderSnapshot::Builder builder;
	renderingEngine.buildRenderSnapshot(builder);
	spk::RenderContext renderContext{.targetSurface = nullptr};
	builder.build().execute(renderContext);

	EXPECT_EQ(entity.deviceUpdates, 0u);
	EXPECT_EQ(component.deviceUpdates, 0u);
	EXPECT_EQ(behaviour.deviceUpdates, 0u);
	EXPECT_EQ(system.deviceUpdates, 0u);
	EXPECT_TRUE(log.empty());
}
