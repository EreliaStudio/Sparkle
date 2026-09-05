#include <gtest/gtest.h>

#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "core/event/event_dispatcher.hpp"

namespace
{
	class RecordingDispatcher final : public spk::EventDispatcher
	{
	public:
		std::string name;
		bool accepting = true;
		bool consume = false;
		bool throwFromHandler = false;
		bool requestKeyboardFocus = false;
		std::vector<RecordingDispatcher *> children;
		std::vector<std::string> *log = nullptr;
		std::function<void()> nestedAction;

		explicit RecordingDispatcher(std::string p_name, std::vector<std::string> &p_log) :
			name(std::move(p_name)),
			log(&p_log)
		{
		}

	private:
		[[nodiscard]] bool _isAcceptingInteraction() const override
		{
			return accepting;
		}

		void _propagateInteraction(const std::function<void(spk::EventDispatcher *)> &callback) override
		{
			for (RecordingDispatcher *child : children)
			{
				callback(child);
			}
		}

		template <typename TEvent>
		void record(const char *eventName, TEvent &event)
		{
			log->push_back(name + ":" + eventName);
			if (requestKeyboardFocus)
			{
				event.takeFocus(spk::FocusMode::Channel::Keyboard, nullptr);
			}
			if (nestedAction)
			{
				auto action = std::exchange(nestedAction, std::function<void()>{});
				action();
			}
			if (throwFromHandler)
			{
				throw std::runtime_error("handler failure");
			}
			if (consume)
			{
				event.consumed = true;
			}
		}

#define SPK_RECORD_HANDLER(Method, EventType, Label) \
		void Method(EventType &event) override { record(Label, event); }

		SPK_RECORD_HANDLER(_onWindowResizedEvent, spk::WindowResizedEvent, "window-resized")
		SPK_RECORD_HANDLER(_onWindowMovedEvent, spk::WindowMovedEvent, "window-moved")
		SPK_RECORD_HANDLER(_onWindowFocusGainedEvent, spk::WindowFocusGainedEvent, "focus-gained")
		SPK_RECORD_HANDLER(_onWindowFocusLostEvent, spk::WindowFocusLostEvent, "focus-lost")
		SPK_RECORD_HANDLER(_onMouseEnteredEvent, spk::MouseEnteredEvent, "mouse-entered")
		SPK_RECORD_HANDLER(_onMouseLeftEvent, spk::MouseLeftEvent, "mouse-left")
		SPK_RECORD_HANDLER(_onMouseMovedEvent, spk::MouseMovedEvent, "mouse-moved")
		SPK_RECORD_HANDLER(_onMouseWheelScrolledEvent, spk::MouseWheelScrolledEvent, "mouse-wheel")
		SPK_RECORD_HANDLER(_onMouseButtonPressedEvent, spk::MouseButtonPressedEvent, "mouse-pressed")
		SPK_RECORD_HANDLER(_onMouseButtonReleasedEvent, spk::MouseButtonReleasedEvent, "mouse-released")
		SPK_RECORD_HANDLER(_onMouseButtonDoubleClickedEvent, spk::MouseButtonDoubleClickedEvent, "mouse-double")
		SPK_RECORD_HANDLER(_onKeyPressedEvent, spk::KeyPressedEvent, "key-pressed")
		SPK_RECORD_HANDLER(_onKeyReleasedEvent, spk::KeyReleasedEvent, "key-released")
		SPK_RECORD_HANDLER(_onTextInputEvent, spk::TextInputEvent, "text-input")
		SPK_RECORD_HANDLER(_onPassiveMouseMovedEvent, spk::MouseMovedEvent, "passive-mouse-moved")
		SPK_RECORD_HANDLER(_onPassiveMouseButtonPressedEvent, spk::MouseButtonPressedEvent, "passive-mouse-pressed")
		SPK_RECORD_HANDLER(_onPassiveKeyPressedEvent, spk::KeyPressedEvent, "passive-key-pressed")
		SPK_RECORD_HANDLER(_onPassiveKeyReleasedEvent, spk::KeyReleasedEvent, "passive-key-released")

#undef SPK_RECORD_HANDLER
	};

	template <typename TRecord>
	TRecord record()
	{
		TRecord value{};
		value.windowIdentifier = "window";
		return value;
	}
}

TEST(EventDispatcherTest, StandardUsageDispatchesEveryEventType)
{
	std::vector<std::string> log;
	RecordingDispatcher dispatcher("root", log);
	spk::Mouse mouse;
	spk::Keyboard keyboard;

	auto resizedRecord = record<spk::WindowResizedRecord>();
	spk::WindowResizedEvent resized(resizedRecord);
	dispatcher.dispatch(resized);
	auto movedRecord = record<spk::WindowMovedRecord>();
	spk::WindowMovedEvent moved(movedRecord);
	dispatcher.dispatch(moved);
	auto gainedRecord = record<spk::WindowFocusGainedRecord>();
	spk::WindowFocusGainedEvent gained(gainedRecord);
	dispatcher.dispatch(gained);
	auto lostRecord = record<spk::WindowFocusLostRecord>();
	spk::WindowFocusLostEvent lost(lostRecord);
	dispatcher.dispatch(lost);
	auto enteredRecord = record<spk::MouseEnteredRecord>();
	spk::MouseEnteredEvent entered(enteredRecord);
	dispatcher.dispatch(entered);
	auto leftRecord = record<spk::MouseLeftRecord>();
	spk::MouseLeftEvent left(leftRecord);
	dispatcher.dispatch(left);
	auto mouseMovedRecord = record<spk::MouseMovedRecord>();
	spk::MouseMovedEvent mouseMoved(mouseMovedRecord, mouse);
	dispatcher.dispatch(mouseMoved);
	auto wheelRecord = record<spk::MouseWheelScrolledRecord>();
	spk::MouseWheelScrolledEvent wheel(wheelRecord, mouse);
	dispatcher.dispatch(wheel);
	auto pressedRecord = record<spk::MouseButtonPressedRecord>();
	spk::MouseButtonPressedEvent pressed(pressedRecord, mouse);
	dispatcher.dispatch(pressed);
	auto releasedRecord = record<spk::MouseButtonReleasedRecord>();
	spk::MouseButtonReleasedEvent released(releasedRecord, mouse);
	dispatcher.dispatch(released);
	auto doubleRecord = record<spk::MouseButtonDoubleClickedRecord>();
	spk::MouseButtonDoubleClickedEvent doubleClicked(doubleRecord, mouse);
	dispatcher.dispatch(doubleClicked);
	auto keyPressedRecord = record<spk::KeyPressedRecord>();
	spk::KeyPressedEvent keyPressed(keyPressedRecord, keyboard);
	dispatcher.dispatch(keyPressed);
	auto keyReleasedRecord = record<spk::KeyReleasedRecord>();
	spk::KeyReleasedEvent keyReleased(keyReleasedRecord, keyboard);
	dispatcher.dispatch(keyReleased);
	auto textRecord = record<spk::TextInputRecord>();
	spk::TextInputEvent text(textRecord, keyboard);
	dispatcher.dispatch(text);

	EXPECT_EQ(log, (std::vector<std::string>{
		"root:window-resized", "root:window-moved", "root:focus-gained", "root:focus-lost",
		"root:mouse-entered", "root:mouse-left", "root:mouse-moved", "root:mouse-wheel",
		"root:mouse-pressed", "root:mouse-released", "root:mouse-double", "root:key-pressed",
		"root:key-released", "root:text-input"}));
}

TEST(EventDispatcherTest, ChildrenAreVisitedBeforeParentInPropagationOrder)
{
	std::vector<std::string> log;
	RecordingDispatcher root("root", log);
	RecordingDispatcher first("first", log);
	RecordingDispatcher second("second", log);
	RecordingDispatcher grandchild("grandchild", log);
	root.children = {&first, &second};
	first.children = {&grandchild};

	auto eventRecord = record<spk::WindowMovedRecord>();
	spk::WindowMovedEvent event(eventRecord);
	root.dispatch(event);

	EXPECT_EQ(log, (std::vector<std::string>{
		"grandchild:window-moved",
		"first:window-moved",
		"second:window-moved",
		"root:window-moved"}));
}

TEST(EventDispatcherTest, ConsumptionShortCircuitsRemainingPropagation)
{
	std::vector<std::string> log;
	RecordingDispatcher root("root", log);
	RecordingDispatcher first("first", log);
	RecordingDispatcher second("second", log);
	root.children = {&first, &second};
	first.consume = true;

	auto eventRecord = record<spk::WindowMovedRecord>();
	spk::WindowMovedEvent event(eventRecord);
	root.dispatch(event);

	EXPECT_TRUE(event.consumed);
	EXPECT_EQ(log, (std::vector<std::string>{"first:window-moved"}));
}

TEST(EventDispatcherTest, NonAcceptingNodeSkipsItsSubtree)
{
	std::vector<std::string> log;
	RecordingDispatcher root("root", log);
	RecordingDispatcher inactive("inactive", log);
	RecordingDispatcher hiddenChild("hidden-child", log);
	RecordingDispatcher active("active", log);
	inactive.accepting = false;
	inactive.children = {&hiddenChild};
	root.children = {&inactive, &active};

	auto eventRecord = record<spk::WindowMovedRecord>();
	spk::WindowMovedEvent event(eventRecord);
	root.dispatch(event);

	EXPECT_EQ(log, (std::vector<std::string>{"active:window-moved", "root:window-moved"}));
}

TEST(EventDispatcherTest, EmptyTreeHandlesEventOnce)
{
	std::vector<std::string> log;
	RecordingDispatcher root("root", log);
	auto eventRecord = record<spk::WindowMovedRecord>();
	spk::WindowMovedEvent event(eventRecord);
	root.dispatch(event);
	EXPECT_EQ(log, (std::vector<std::string>{"root:window-moved"}));
}

TEST(EventDispatcherTest, PassiveObserversVisitNodeThenChildrenAndIgnoreConsumption)
{
	std::vector<std::string> log;
	RecordingDispatcher root("root", log);
	RecordingDispatcher child("child", log);
	root.children = {&child};
	spk::Mouse mouse;
	spk::Keyboard keyboard;

	auto movedRecord = record<spk::MouseMovedRecord>();
	spk::MouseMovedEvent moved(movedRecord, mouse);
	moved.consumed = true;
	root.observePointer(moved);
	auto pressedRecord = record<spk::MouseButtonPressedRecord>();
	spk::MouseButtonPressedEvent pressed(pressedRecord, mouse);
	root.observePointer(pressed);
	auto keyPressedRecord = record<spk::KeyPressedRecord>();
	spk::KeyPressedEvent keyPressed(keyPressedRecord, keyboard);
	root.observeKeyboard(keyPressed);
	auto keyReleasedRecord = record<spk::KeyReleasedRecord>();
	spk::KeyReleasedEvent keyReleased(keyReleasedRecord, keyboard);
	root.observeKeyboard(keyReleased);

	EXPECT_EQ(log, (std::vector<std::string>{
		"root:passive-mouse-moved", "child:passive-mouse-moved",
		"root:passive-mouse-pressed", "child:passive-mouse-pressed",
		"root:passive-key-pressed", "child:passive-key-pressed",
		"root:passive-key-released", "child:passive-key-released"}));
}

TEST(EventDispatcherTest, NestedDispatchIsSupported)
{
	std::vector<std::string> log;
	RecordingDispatcher root("root", log);
	auto nestedRecord = record<spk::KeyReleasedRecord>();
	spk::Keyboard keyboard;
	spk::KeyReleasedEvent nestedEvent(nestedRecord, keyboard);
	root.nestedAction = [&] { root.dispatch(nestedEvent); };

	auto outerRecord = record<spk::WindowMovedRecord>();
	spk::WindowMovedEvent outerEvent(outerRecord);
	root.dispatch(outerEvent);

	EXPECT_EQ(log, (std::vector<std::string>{"root:window-moved", "root:key-released"}));
}

TEST(EventDispatcherTest, FocusChangeRequestedByHandlerRemainsOnEvent)
{
	std::vector<std::string> log;
	RecordingDispatcher root("root", log);
	root.requestKeyboardFocus = true;
	auto eventRecord = record<spk::WindowMovedRecord>();
	spk::WindowMovedEvent event(eventRecord);
	root.dispatch(event);

	const auto &change = event.focusChange(spk::FocusMode::Channel::Keyboard);
	ASSERT_TRUE(change.has_value());
	EXPECT_EQ(change->type, spk::FocusMode::ChangeType::Take);
	EXPECT_EQ(change->widget, nullptr);
}

TEST(EventDispatcherTest, HandlerExceptionDoesNotCorruptLaterDispatch)
{
	std::vector<std::string> log;
	RecordingDispatcher root("root", log);
	root.throwFromHandler = true;
	auto firstRecord = record<spk::WindowMovedRecord>();
	spk::WindowMovedEvent first(firstRecord);
	EXPECT_THROW(root.dispatch(first), std::runtime_error);

	root.throwFromHandler = false;
	auto secondRecord = record<spk::WindowMovedRecord>();
	spk::WindowMovedEvent second(secondRecord);
	EXPECT_NO_THROW(root.dispatch(second));
	EXPECT_EQ(log, (std::vector<std::string>{"root:window-moved", "root:window-moved"}));
}
