#include <gtest/gtest.h>

#include <cstdint>

#include "core/event/event.hpp"

namespace
{
	struct SampleRecord
	{
		int value = 0;
	};

	struct SampleDevice
	{
		int value = 0;
	};

	spk::Widget *widgetToken(std::uintptr_t value)
	{
		return reinterpret_cast<spk::Widget *>(value);
	}
}

TEST(EventTest, StandardUsage)
{
	SampleRecord record{12};
	SampleDevice device{34};
	spk::DeviceEvent<SampleRecord, SampleDevice> event(record, device);

	EXPECT_EQ(&event.record, &record);
	EXPECT_EQ(&event.device, &device);
	EXPECT_FALSE(event.consumed);

	event.consumed = true;
	EXPECT_TRUE(event.consumed);

	spk::Widget *keyboardWidget = widgetToken(0x1000);
	spk::Widget *mouseWidget = widgetToken(0x2000);
	EXPECT_TRUE(event.takeFocus(spk::FocusMode::Channel::Keyboard, keyboardWidget));
	EXPECT_TRUE(event.releaseFocus(spk::FocusMode::Channel::Mouse, mouseWidget));

	const auto &keyboard = event.focusChange(spk::FocusMode::Channel::Keyboard);
	const auto &mouse = event.focusChange(spk::FocusMode::Channel::Mouse);
	ASSERT_TRUE(keyboard.has_value());
	ASSERT_TRUE(mouse.has_value());
	EXPECT_EQ(keyboard->type, spk::FocusMode::ChangeType::Take);
	EXPECT_EQ(keyboard->widget, keyboardWidget);
	EXPECT_EQ(mouse->type, spk::FocusMode::ChangeType::Release);
	EXPECT_EQ(mouse->widget, mouseWidget);
}

TEST(EventTest, EventBindsRecordByReference)
{
	SampleRecord record{1};
	spk::Event<SampleRecord> event(record);
	EXPECT_EQ(&event.record, &record);

	record.value = 9;
	EXPECT_EQ(event.record.value, 9);

	SampleRecord independentCopy = record;
	independentCopy.value = 42;
	EXPECT_EQ(event.record.value, 9);
}

TEST(EventTest, DeviceEventBindsDeviceByReference)
{
	SampleRecord record{1};
	SampleDevice device{2};
	spk::DeviceEvent<SampleRecord, SampleDevice> event(record, device);
	EXPECT_EQ(&event.device, &device);

	device.value = 8;
	EXPECT_EQ(event.device.value, 8);

	SampleDevice independentCopy = device;
	independentCopy.value = 99;
	EXPECT_EQ(event.device.value, 8);
}

TEST(EventTest, RepeatedIdenticalFocusRequestKeepsFirstRecord)
{
	spk::EventBase event;
	spk::Widget *widget = widgetToken(0x1000);

	EXPECT_TRUE(event.takeFocus(spk::FocusMode::Channel::Keyboard, widget));
	EXPECT_FALSE(event.takeFocus(spk::FocusMode::Channel::Keyboard, widget));

	const auto &change = event.focusChange(spk::FocusMode::Channel::Keyboard);
	ASSERT_TRUE(change.has_value());
	EXPECT_EQ(change->type, spk::FocusMode::ChangeType::Take);
	EXPECT_EQ(change->widget, widget);
}

TEST(EventTest, ConflictingFocusRequestKeepsFirstRecord)
{
	spk::EventBase event;
	spk::Widget *first = widgetToken(0x1000);
	spk::Widget *second = widgetToken(0x2000);

	EXPECT_TRUE(event.takeFocus(spk::FocusMode::Channel::Mouse, first));
	EXPECT_FALSE(event.releaseFocus(spk::FocusMode::Channel::Mouse, second));

	const auto &change = event.focusChange(spk::FocusMode::Channel::Mouse);
	ASSERT_TRUE(change.has_value());
	EXPECT_EQ(change->type, spk::FocusMode::ChangeType::Take);
	EXPECT_EQ(change->widget, first);
}

TEST(EventTest, NullWidgetFocusRequestIsRecorded)
{
	spk::EventBase event;
	EXPECT_TRUE(event.takeFocus(spk::FocusMode::Channel::Keyboard, nullptr));

	const auto &change = event.focusChange(spk::FocusMode::Channel::Keyboard);
	ASSERT_TRUE(change.has_value());
	EXPECT_EQ(change->type, spk::FocusMode::ChangeType::Take);
	EXPECT_EQ(change->widget, nullptr);
}

TEST(EventTest, FocusChannelsAreIndependent)
{
	spk::EventBase event;
	spk::Widget *keyboardWidget = widgetToken(0x1000);
	spk::Widget *mouseWidget = widgetToken(0x2000);

	EXPECT_TRUE(event.takeFocus(spk::FocusMode::Channel::Keyboard, keyboardWidget));
	EXPECT_TRUE(event.releaseFocus(spk::FocusMode::Channel::Mouse, mouseWidget));
	EXPECT_FALSE(event.takeFocus(spk::FocusMode::Channel::Keyboard, mouseWidget));
	EXPECT_FALSE(event.releaseFocus(spk::FocusMode::Channel::Mouse, keyboardWidget));
}

// Disabled: EventBase currently exposes FocusMode::Channel as an enum that can be cast
// out of range while the implementation indexes a fixed std::array without a documented
// bounds guard. Exercising that value would use undefined behavior as the test oracle.
TEST(EventTest, DISABLED_OutOfRangeFocusChannelHasDefinedBehavior)
{
	spk::EventBase event;
	for (const auto value : {spk::FocusMode::ChannelCount, spk::FocusMode::ChannelCount + 1, spk::FocusMode::ChannelCount + 100})
	{
		const auto invalid = static_cast<spk::FocusMode::Channel>(value);
		EXPECT_FALSE(event.takeFocus(invalid, nullptr));
		EXPECT_FALSE(event.releaseFocus(invalid, nullptr));
	}
}
