#include <gtest/gtest.h>

#include "core/context/update_context.hpp"
#include "engine/behaviour.hpp"
#include "exception.hpp"
#include "rendering/render_command.hpp"
#include "sparkle_test.hpp"
#include "ui/widget/engine_widget.hpp"

namespace
{
	struct Events : spk::Behaviour
	{
		std::vector<std::string> received;
#define RECORD_EVENT(Name)                             \
	void _on##Name##Event(spk::Name##Event &) override \
	{                                                  \
		received.push_back(#Name);                     \
	}
		RECORD_EVENT(WindowResized)
		RECORD_EVENT(WindowMoved) RECORD_EVENT(WindowFocusGained) RECORD_EVENT(WindowFocusLost)
			RECORD_EVENT(MouseEntered) RECORD_EVENT(MouseLeft) RECORD_EVENT(MouseMoved) RECORD_EVENT(MouseWheelScrolled)
				RECORD_EVENT(MouseButtonPressed) RECORD_EVENT(MouseButtonReleased) RECORD_EVENT(MouseButtonDoubleClicked)
					RECORD_EVENT(KeyPressed) RECORD_EVENT(KeyReleased) RECORD_EVENT(TextInput)
#undef RECORD_EVENT
#define RECORD_PASSIVE(Name)                                  \
	void _onPassive##Name##Event(spk::Name##Event &) override \
	{                                                         \
		received.push_back("Passive" #Name);                  \
	}
						RECORD_PASSIVE(MouseMoved) RECORD_PASSIVE(MouseButtonPressed) RECORD_PASSIVE(KeyPressed) RECORD_PASSIVE(KeyReleased)
#undef RECORD_PASSIVE
	};
	void allEvents(spk::EngineWidget &widget)
	{
		spk::Mouse mouse;
		spk::Keyboard keyboard;
#define SEND_EVENT(Name)                \
	{                                   \
		spk::Name##Record record{};     \
		spk::Name##Event event(record); \
		widget.dispatch(event);         \
	}
		SEND_EVENT(WindowResized)
		SEND_EVENT(WindowMoved) SEND_EVENT(WindowFocusGained) SEND_EVENT(WindowFocusLost) SEND_EVENT(MouseEntered) SEND_EVENT(MouseLeft)
#undef SEND_EVENT
#define SEND_DEVICE(Name, Device)               \
	{                                           \
		spk::Name##Record record{};             \
		spk::Name##Event event(record, Device); \
		widget.dispatch(event);                 \
	}
			SEND_DEVICE(MouseMoved, mouse) SEND_DEVICE(MouseWheelScrolled, mouse) SEND_DEVICE(MouseButtonPressed, mouse)
				SEND_DEVICE(MouseButtonReleased, mouse) SEND_DEVICE(MouseButtonDoubleClicked, mouse)
					SEND_DEVICE(KeyPressed, keyboard) SEND_DEVICE(KeyReleased, keyboard) SEND_DEVICE(TextInput, keyboard)
#undef SEND_DEVICE
#define OBSERVE(Name, Device, Method)           \
	{                                           \
		spk::Name##Record record{};             \
		spk::Name##Event event(record, Device); \
		event.consumed = true;                  \
		widget.Method(event);                   \
	}
						OBSERVE(MouseMoved, mouse, observePointer) OBSERVE(MouseButtonPressed, mouse, observePointer)
							OBSERVE(KeyPressed, keyboard, observeKeyboard) OBSERVE(KeyReleased, keyboard, observeKeyboard)
#undef OBSERVE
	}
	struct Mark : spk::RenderCommand
	{
		std::vector<int> &log;
		int value;
		Mark(std::vector<int> &log, int value) :
			log(log),
			value(value)
		{
		}
		void execute(spk::RenderContext &) const override
		{
			log.push_back(value);
		}
	};
	struct Rendering : spk::Behaviour
	{
		bool fail = false;
		std::vector<int> *log = nullptr;
		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder) override
		{
			if (fail)
			{
				throw std::runtime_error("engine render failure");
			}
			builder.renderPass(spk::Engine::SceneRenderPassKey).emplace<Mark>(*log, 2);
		}
	};
}

TEST(EngineWidgetIntegrationTest, EveryActiveAndPassiveEventHandlesInactiveNullAndReplacementStates)
{
	const std::vector<std::string> expected{"WindowResized", "WindowMoved", "WindowFocusGained", "WindowFocusLost", "MouseEntered", "MouseLeft", "MouseMoved", "MouseWheelScrolled", "MouseButtonPressed", "MouseButtonReleased", "MouseButtonDoubleClicked", "KeyPressed", "KeyReleased", "TextInput", "PassiveMouseMoved", "PassiveMouseButtonPressed", "PassiveKeyPressed", "PassiveKeyReleased"};
	spk::Engine first, second;
	auto &a = first.root().addBehaviour<Events>();
	auto &b = second.root().addBehaviour<Events>();
	spk::EngineWidget widget("EngineWidget", nullptr);
	widget.setEngine(&first);
	widget.activate();
	allEvents(widget);
	EXPECT_EQ(a.received, expected);
	EXPECT_TRUE(b.received.empty());
	a.received.clear();
	widget.deactivate();
	allEvents(widget);
	EXPECT_TRUE(a.received.empty());
	widget.activate();
	first.root().deactivate();
	allEvents(widget);
	EXPECT_TRUE(a.received.empty());
	first.root().activate();
	widget.setEngine(&second);
	allEvents(widget);
	EXPECT_EQ(b.received, expected);
	EXPECT_TRUE(a.received.empty());
	b.received.clear();
	widget.setEngine(nullptr);
	allEvents(widget);
	EXPECT_TRUE(a.received.empty());
	EXPECT_TRUE(b.received.empty());
}

TEST(EngineWidgetIntegrationTest, RenderPassOrderingExceptionsAndExplicitDetachmentRespectLifetime)
{
	std::vector<int> log;
	auto engine = std::make_unique<spk::Engine>();
	auto &behaviour = engine->root().addBehaviour<Rendering>();
	behaviour.log = &log;
	spk::EngineWidget widget("RenderingWidget", nullptr);
	widget.activate();
	widget.setGeometry({.anchor = {0, 0}, .size = {100, 100}});
	widget.setEngine(engine.get());
	spk::RenderSnapshot::Builder builder;
	builder.renderPass({"before-scene", spk::Engine::SceneRenderPassKey.order - 1}).emplace<Mark>(log, 1);
	builder.renderPass({"after-scene", spk::Engine::SceneRenderPassKey.order + 1}).emplace<Mark>(log, 3);
	widget.buildRenderSnapshot(builder);
	const auto snapshot = builder.build();
	auto &context = sparkle_test::OpenGLTestContext::instance();
	context.reset();
	snapshot.execute(context.renderContext());
	EXPECT_EQ(log, (std::vector<int>{1, 2, 3}));
	behaviour.fail = true;
	try
	{
		spk::RenderSnapshot::Builder failing;
		widget.buildRenderSnapshot(failing);
		FAIL();
	} catch (const spk::Exception &error)
	{
		EXPECT_NE(std::string(error.what()).find("RenderingWidget"), std::string::npos);
		EXPECT_NE(error.cause(), nullptr);
	}
	widget.setEngine(nullptr);
	engine.reset();
	spk::RenderSnapshot::Builder detached;
	EXPECT_NO_THROW(widget.buildRenderSnapshot(detached));
	log.clear();
	snapshot.execute(context.renderContext());
	EXPECT_EQ(log, (std::vector<int>{1, 2, 3}));
	spk::Engine survivor;
	{
		spk::EngineWidget temporary("Temporary", nullptr);
		temporary.setEngine(&survivor);
	}
	EXPECT_NO_THROW(survivor.handleGeometryChange({.anchor = {0, 0}, .size = {10, 10}}));
}
