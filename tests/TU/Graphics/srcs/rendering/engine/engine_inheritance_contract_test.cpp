#include <gtest/gtest.h>

#include <concepts>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "core/context/render_context.hpp"
#include "core/context/update_context.hpp"
#include "engine/engine_headless.hpp"
#include "engine/entity3d.hpp"
#include "engine/system.hpp"
#include "input/device_context.hpp"
#include "input/keyboard.hpp"
#include "input/mouse.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/engine/rendering_behaviour.hpp"
#include "rendering/engine/rendering_component.hpp"
#include "rendering/engine/rendering_system.hpp"
#include "rendering/render_command.hpp"

namespace
{
	static_assert(std::derived_from<spk::Engine, spk::EngineHeadless>);
	static_assert(std::has_virtual_destructor_v<spk::EngineHeadless>);
	static_assert(!std::derived_from<spk::EngineHeadless, spk::EventDispatcher>);

	class LogicalAndDeviceProbe final : public spk::RenderingBehaviour
	{
	public:
		int logicalUpdates = 0;
		int deviceUpdates = 0;

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++logicalUpdates;
		}

		void _updateState(spk::UpdateContext &, spk::DeviceContext &) override
		{
			++deviceUpdates;
		}
	};

	class KeyProbe final : public spk::RenderingBehaviour
	{
	public:
		int presses = 0;
		int passivePresses = 0;

	protected:
		void _onKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			++presses;
		}

		void _onPassiveKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			++passivePresses;
		}
	};

	class KeyComponentProbe final : public spk::RenderingComponent3D
	{
	public:
		int presses = 0;

	protected:
		void _onKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			++presses;
		}
	};

	class CoreCountingSystem final : public spk::System
	{
	public:
		int updates = 0;

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++updates;
		}
	};

	class GraphicsCountingSystem final : public spk::RenderingSystem
	{
	public:
		int logicalUpdates = 0;
		int deviceUpdates = 0;
		int presses = 0;

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++logicalUpdates;
		}

		void _updateState(spk::UpdateContext &, spk::DeviceContext &) override
		{
			++deviceUpdates;
		}

		void _onKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			++presses;
		}
	};

	class LogCommand final : public spk::RenderCommand
	{
	private:
		std::vector<std::string> &_output;
		std::string _label;

	public:
		LogCommand(std::vector<std::string> &output, std::string label) :
			_output(output),
			_label(std::move(label))
		{
		}

		void execute(spk::RenderContext &) const override
		{
			_output.push_back(_label);
		}
	};

	class SnapshotProbe final : public spk::RenderingBehaviour
	{
	public:
		std::vector<std::string> *output = nullptr;
		std::string label;

	protected:
		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder) override
		{
			if (output != nullptr)
			{
				builder.renderPass(spk::Engine::SceneRenderPassKey).emplace<LogCommand>(*output, label);
			}
		}
	};

	class ThrowingSnapshotProbe final : public spk::RenderingBehaviour
	{
	protected:
		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &) override
		{
			throw std::logic_error("snapshot failure");
		}
	};

	void sendKey(spk::Engine &engine)
	{
		spk::Keyboard keyboard;
		spk::KeyPressedRecord record{};
		record.key = spk::Keyboard::A;
		spk::KeyPressedEvent event(record, keyboard);
		engine.dispatch(event);
	}

	void sendPassiveKey(spk::Engine &engine)
	{
		spk::Keyboard keyboard;
		spk::KeyPressedRecord record{};
		record.key = spk::Keyboard::A;
		spk::KeyPressedEvent event(record, keyboard);
		event.consumed = true;
		engine.observeKeyboard(event);
	}

	void runSnapshot(spk::Engine &engine)
	{
		spk::RenderSnapshot::Builder builder;
		engine.buildRenderSnapshot(builder);
		spk::RenderContext context{.targetSurface = nullptr};
		builder.build().execute(context);
	}
}

TEST(EngineInheritanceContractTest, EngineCanBeUsedAndDeletedThroughHeadlessBase)
{
	std::unique_ptr<spk::EngineHeadless> headless = std::make_unique<spk::Engine>();
	EXPECT_NE(dynamic_cast<spk::Engine *>(headless.get()), nullptr);
	headless.reset();
	EXPECT_EQ(headless, nullptr);
}

TEST(EngineInheritanceContractTest, HeadlessInterfaceUsesSameRootAndGeometry)
{
	spk::Engine engine;
	spk::EngineHeadless &headless = engine;
	const spk::Rect2D geometry{.anchor = {10, 20}, .size = {320, 240}};
	headless.handleGeometryChange(geometry);

	EXPECT_EQ(&headless.root(), &engine.root());
	EXPECT_EQ(headless.geometry(), geometry);
	EXPECT_EQ(engine.geometry(), geometry);
}

TEST(EngineInheritanceContractTest, HeadlessAndDeviceUpdatesAreIndependent)
{
	spk::Engine engine;
	auto &probe = engine.root().addBehaviour<LogicalAndDeviceProbe>();
	spk::UpdateContext update{};
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::DeviceContext device{.keyboard = keyboard, .mouse = mouse};

	static_cast<spk::EngineHeadless &>(engine).updateState(update);
	EXPECT_EQ(probe.logicalUpdates, 1);
	EXPECT_EQ(probe.deviceUpdates, 0);

	engine.updateState(update, device);
	EXPECT_EQ(probe.logicalUpdates, 1);
	EXPECT_EQ(probe.deviceUpdates, 1);
}

TEST(EngineInheritanceContractTest, GraphicalEngineKeepsCoreAndGraphicalSystemsInOneCollection)
{
	spk::Engine engine;
	auto &core = engine.addSystem<CoreCountingSystem>();
	auto &graphics = engine.addSystem<GraphicsCountingSystem>();
	spk::UpdateContext update{};
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::DeviceContext device{.keyboard = keyboard, .mouse = mouse};

	engine.updateState(update);
	engine.updateState(update, device);

	EXPECT_EQ(core.updates, 1);
	EXPECT_EQ(graphics.logicalUpdates, 1);
	EXPECT_EQ(graphics.deviceUpdates, 1);
	EXPECT_EQ(core.engine(), &engine);
	EXPECT_EQ(graphics.engine(), &engine);
}

TEST(EngineInheritanceContractTest, CoreEntityCanCarryRenderingBehaviour)
{
	spk::Engine engine;
	spk::Entity3D entity("Plain entity");
	auto &probe = entity.addBehaviour<KeyProbe>();
	engine.addEntity(&entity);

	sendKey(engine);
	EXPECT_EQ(probe.presses, 1);
	EXPECT_EQ(entity.context(), &engine);
}

TEST(EngineInheritanceContractTest, CoreEntityCanCarryRenderingComponent3D)
{
	spk::Engine engine;
	spk::Entity3D entity("Plain entity");
	auto &probe = entity.addComponent<KeyComponentProbe>();
	engine.addEntity(&entity);

	sendKey(engine);
	EXPECT_EQ(probe.presses, 1);
	EXPECT_EQ(probe.owner(), &entity);
}

TEST(EngineInheritanceContractTest, GraphicalEventsTraverseNestedCoreEntities)
{
	spk::Engine engine;
	spk::Entity3D parent("Parent");
	spk::Entity3D child("Child", &parent);
	auto &parentProbe = parent.addBehaviour<KeyProbe>();
	auto &childProbe = child.addBehaviour<KeyProbe>();
	engine.addEntity(&parent);

	sendKey(engine);
	EXPECT_EQ(parentProbe.presses, 1);
	EXPECT_EQ(childProbe.presses, 1);
}

TEST(EngineInheritanceContractTest, DetachedEntityStopsReceivingGraphicalEvents)
{
	spk::Engine engine;
	spk::Entity3D entity("Entity");
	auto &probe = entity.addBehaviour<KeyProbe>();
	engine.addEntity(&entity);
	sendKey(engine);

	engine.removeEntity(&entity);
	sendKey(engine);
	EXPECT_EQ(probe.presses, 1);
	EXPECT_EQ(entity.context(), nullptr);
	EXPECT_EQ(entity.parent(), nullptr);
}

TEST(EngineInheritanceContractTest, MovingEntityBetweenEnginesTransfersEventRouting)
{
	spk::Engine first;
	spk::Engine second;
	spk::Entity3D entity("Entity");
	auto &probe = entity.addBehaviour<KeyProbe>();
	first.addEntity(&entity);
	sendKey(first);

	second.addEntity(&entity);
	sendKey(first);
	sendKey(second);

	EXPECT_EQ(probe.presses, 2);
	EXPECT_EQ(entity.context(), &second);
}

TEST(EngineInheritanceContractTest, DeactivatedParentSuppressesDescendantGraphicalEvents)
{
	spk::Engine engine;
	spk::Entity3D parent("Parent");
	spk::Entity3D child("Child", &parent);
	auto &probe = child.addBehaviour<KeyProbe>();
	engine.addEntity(&parent);

	parent.deactivate();
	sendKey(engine);
	EXPECT_EQ(probe.presses, 0);

	parent.activate();
	sendKey(engine);
	EXPECT_EQ(probe.presses, 1);
}

TEST(EngineInheritanceContractTest, DisabledComponentDoesNotHideActiveSibling)
{
	spk::Engine engine;
	spk::Entity3D entity("Entity");
	auto &disabled = entity.addComponent<KeyComponentProbe>();
	auto &enabled = entity.addComponent<KeyComponentProbe>();
	engine.addEntity(&entity);
	disabled.deactivate();

	sendKey(engine);
	EXPECT_EQ(disabled.presses, 0);
	EXPECT_EQ(enabled.presses, 1);
}

TEST(EngineInheritanceContractTest, PassiveEventsReachGraphicalBehaviours)
{
	spk::Engine engine;
	auto &probe = engine.root().addBehaviour<KeyProbe>();
	sendPassiveKey(engine);

	EXPECT_EQ(probe.presses, 0);
	EXPECT_EQ(probe.passivePresses, 1);
}

TEST(EngineInheritanceContractTest, RemovedSystemStopsReceivingGraphicalEvents)
{
	spk::Engine engine;
	auto &system = engine.addSystem<GraphicsCountingSystem>();
	sendKey(engine);
	EXPECT_EQ(system.presses, 1);

	engine.removeSystem(system);
	EXPECT_NO_THROW(sendKey(engine));
	EXPECT_TRUE(engine.systems().empty());
}

TEST(EngineInheritanceContractTest, DisabledSystemStopsBothDeviceUpdatesAndEvents)
{
	spk::Engine engine;
	auto &system = engine.addSystem<GraphicsCountingSystem>();
	spk::UpdateContext update{};
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::DeviceContext device{.keyboard = keyboard, .mouse = mouse};
	system.deactivate();

	engine.updateState(update, device);
	sendKey(engine);
	EXPECT_EQ(system.deviceUpdates, 0);
	EXPECT_EQ(system.presses, 0);

	system.activate();
	engine.updateState(update, device);
	sendKey(engine);
	EXPECT_EQ(system.deviceUpdates, 1);
	EXPECT_EQ(system.presses, 1);
}

TEST(EngineInheritanceContractTest, SnapshotContainsNestedCoreEntityContributors)
{
	spk::Engine engine;
	spk::Entity3D parent("Parent");
	spk::Entity3D child("Child", &parent);
	std::vector<std::string> log;
	auto &first = parent.addBehaviour<SnapshotProbe>();
	auto &second = child.addBehaviour<SnapshotProbe>();
	first.output = &log;
	second.output = &log;
	first.label = "parent";
	second.label = "child";
	engine.addEntity(&parent);

	runSnapshot(engine);
	EXPECT_EQ(log, (std::vector<std::string>{"parent", "child"}));
}

TEST(EngineInheritanceContractTest, DetachedEntityStopsContributingToNewSnapshots)
{
	spk::Engine engine;
	spk::Entity3D entity("Entity");
	std::vector<std::string> log;
	auto &probe = entity.addBehaviour<SnapshotProbe>();
	probe.output = &log;
	probe.label = "visible";
	engine.addEntity(&entity);
	runSnapshot(engine);

	engine.removeEntity(&entity);
	runSnapshot(engine);
	EXPECT_EQ(log, (std::vector<std::string>{"visible"}));
}

TEST(EngineInheritanceContractTest, SnapshotCommandsSurviveSourceDetachment)
{
	spk::Engine engine;
	spk::Entity3D entity("Entity");
	std::vector<std::string> log;
	auto &probe = entity.addBehaviour<SnapshotProbe>();
	probe.output = &log;
	probe.label = "captured";
	engine.addEntity(&entity);
	spk::RenderSnapshot::Builder builder;
	engine.buildRenderSnapshot(builder);
	auto snapshot = builder.build();

	engine.removeEntity(&entity);
	spk::RenderContext context{.targetSurface = nullptr};
	snapshot.execute(context);
	EXPECT_EQ(log, (std::vector<std::string>{"captured"}));
}

TEST(EngineInheritanceContractTest, SnapshotExceptionsPropagateToCaller)
{
	spk::Engine engine;
	engine.root().addBehaviour<ThrowingSnapshotProbe>();
	spk::RenderSnapshot::Builder builder;

	EXPECT_THROW(engine.buildRenderSnapshot(builder), std::logic_error);
}

TEST(EngineInheritanceContractTest, DeactivatedContributorsAreExcludedFromSnapshots)
{
	spk::Engine engine;
	spk::Entity3D entity("Entity");
	std::vector<std::string> log;
	auto &probe = entity.addBehaviour<SnapshotProbe>();
	probe.output = &log;
	probe.label = "visible";
	engine.addEntity(&entity);
	entity.deactivate();

	runSnapshot(engine);
	EXPECT_TRUE(log.empty());

	entity.activate();
	runSnapshot(engine);
	EXPECT_EQ(log, (std::vector<std::string>{"visible"}));
}
