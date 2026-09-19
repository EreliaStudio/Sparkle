#include <gtest/gtest.h>

#include "core/context/update_context.hpp"
#include "core/platform/clipboard.hpp"
#include "ui/widget/text_edit.hpp"
#include <Windows.h>
#include <chrono>
#include <future>
#include <limits>
#include <thread>

namespace
{
	bool type(spk::TextEdit &edit, char32_t glyph)
	{
		spk::Keyboard keyboard;
		spk::TextInputRecord record{};
		record.glyph = glyph;
		spk::TextInputEvent event(record, keyboard);
		edit.dispatch(event);
		return event.consumed;
	}
	bool key(spk::TextEdit &edit, spk::Keyboard::Key key, bool shift = false, bool control = false)
	{
		spk::Keyboard keyboard;
		if (shift)
		{
			keyboard[spk::Keyboard::Shift] = spk::InputState::Down;
		}
		if (control)
		{
			keyboard[spk::Keyboard::Control] = spk::InputState::Down;
		}
		spk::KeyPressedRecord record{};
		record.key = key;
		spk::KeyPressedEvent event(record, keyboard);
		edit.dispatch(event);
		return event.consumed;
	}
}

TEST(TextEditTest, FocusTypingNavigationDeletionAndShiftedSelection)
{
	spk::TextEdit edit("Edit");
	edit.setGeometry({.anchor = {0, 0}, .size = {200, 40}});
	EXPECT_FALSE(type(edit, U'x'));
	edit.notifyFocusAcquired(spk::FocusMode::Channel::Keyboard);
	int editions = 0;
	auto contract = edit.subscribeToEdition([&](const auto &text) {
		++editions;
		EXPECT_EQ(text, edit.text());
	});
	for (auto glyph : std::u32string(U"abcd"))
	{
		EXPECT_TRUE(type(edit, glyph));
	}
	EXPECT_EQ(edit.text(), U"abcd");
	EXPECT_EQ(editions, 4);
	EXPECT_TRUE(key(edit, spk::Keyboard::LeftArrow, true));
	EXPECT_EQ(edit.selectedText(), U"d");
	key(edit, spk::Keyboard::LeftArrow, true);
	EXPECT_EQ(edit.selectedText(), U"cd");
	EXPECT_TRUE(type(edit, U'\u00e9'));
	EXPECT_EQ(edit.text(), U"ab\u00e9");
	key(edit, spk::Keyboard::Home);
	EXPECT_EQ(edit.cursor(), 0u);
	key(edit, spk::Keyboard::Backspace);
	EXPECT_EQ(edit.text(), U"ab\u00e9");
	key(edit, spk::Keyboard::Delete);
	EXPECT_EQ(edit.text(), U"b\u00e9");
	key(edit, spk::Keyboard::End);
	key(edit, spk::Keyboard::Backspace);
	EXPECT_EQ(edit.text(), U"b");
	key(edit, spk::Keyboard::A, false, true);
	EXPECT_EQ(edit.selectedText(), U"b");
	key(edit, spk::Keyboard::Escape);
	EXPECT_FALSE(edit.hasSelection());
	EXPECT_TRUE(edit.isFocused());
	key(edit, spk::Keyboard::Escape);
	EXPECT_FALSE(edit.isFocused());
	EXPECT_FALSE(type(edit, U'x'));
}

TEST(TextEditTest, SelectionNormalizationClampingAndEmptyPlaceholder)
{
	spk::TextEdit edit("Edit");
	edit.setText(U"abc\U0001f600");
	int selections = 0;
	auto contract = edit.subscribeToSelection([&](const auto &selection) {
		++selections;
		EXPECT_EQ(selection, edit.selection());
	});
	edit.setSelection(3, 1);
	EXPECT_EQ(edit.selection(), (spk::TextEdit::Selection{1, 3}));
	EXPECT_EQ(edit.cursor(), 1u);
	EXPECT_EQ(edit.selectedText(), U"bc");
	edit.setSelection(0, std::numeric_limits<std::size_t>::max());
	EXPECT_EQ(edit.selection().length(), 4u);
	EXPECT_EQ(edit.cursor(), 4u);
	const int before = selections;
	edit.selectAll();
	EXPECT_EQ(selections, before);
	edit.clearSelection();
	EXPECT_FALSE(edit.hasSelection());
	edit.setText("");
	edit.setPlaceholder("hint");
	EXPECT_EQ(edit.cursor(), 0u);
	EXPECT_EQ(edit.visibleStart(), 0u);
	EXPECT_EQ(edit.visibleEnd(), 0u);
	EXPECT_FALSE(edit.hasText());
	EXPECT_EQ(edit.renderedText(), U"hint");
	edit.setSelection(100, 200);
	EXPECT_TRUE(edit.selection().empty());
}

TEST(TextEditTest, UnicodeScrollingBlinkAndFocusLossUseDeterministicTime)
{
	spk::TextEdit edit("Edit");
	edit.setGeometry({.anchor = {0, 0}, .size = {55, 35}});
	edit.setText(U"A\u00e9\U0001f600 long text that scrolls");
	EXPECT_EQ(spk::Font::textFromUTF8(edit.textAsUTF8()), edit.text());
	EXPECT_GT(edit.visibleStart(), 0u);
	EXPECT_EQ(edit.visibleEnd(), edit.text().size());
	edit.notifyFocusAcquired(spk::FocusMode::Channel::Keyboard);
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext context{.time = {}, .deltaTime = std::chrono::milliseconds(249), .keyboard = keyboard, .mouse = mouse};
	edit.updateState(context);
	EXPECT_TRUE(edit.isCaretVisible());
	context.deltaTime = std::chrono::milliseconds(1);
	edit.updateState(context);
	EXPECT_FALSE(edit.isCaretVisible());
	context.deltaTime = std::chrono::milliseconds(750);
	edit.updateState(context);
	EXPECT_TRUE(edit.isCaretVisible());
	key(edit, spk::Keyboard::Home);
	EXPECT_EQ(edit.visibleStart(), 0u);
	edit.notifyFocusReleased(spk::FocusMode::Channel::Keyboard);
	EXPECT_FALSE(edit.isCaretVisible());
	edit.updateState(context);
	EXPECT_FALSE(edit.isCaretVisible());
	edit.setGeometry({.anchor = {0, 0}, .size = {0, 0}});
	EXPECT_TRUE(edit.visibleText().empty());
}

TEST(TextEditTest, MouseDragDoubleClickHoverAndFocusRequests)
{
	spk::TextEdit edit("Edit");
	edit.setGeometry({.anchor = {0, 0}, .size = {200, 40}});
	edit.setText("hello world");
	spk::Mouse mouse;
	mouse.position = {8, 15};
	spk::MouseButtonPressedRecord press{};
	press.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent pressed(press, mouse);
	edit.dispatch(pressed);
	EXPECT_TRUE(pressed.consumed);
	EXPECT_TRUE(edit.isFocused());
	ASSERT_TRUE(pressed.focusChange(spk::FocusMode::Channel::Keyboard));
	EXPECT_EQ(pressed.focusChange(spk::FocusMode::Channel::Keyboard)->widget, &edit);
	mouse.position = {195, 15};
	spk::MouseMovedRecord movedRecord{};
	movedRecord.position = mouse.position;
	spk::MouseMovedEvent moved(movedRecord, mouse);
	edit.dispatch(moved);
	EXPECT_TRUE(moved.consumed);
	EXPECT_TRUE(edit.hasSelection());
	EXPECT_TRUE(edit.isHovered());
	spk::MouseButtonReleasedRecord release{};
	release.button = spk::Mouse::Button::Left;
	spk::MouseButtonReleasedEvent released(release, mouse);
	edit.dispatch(released);
	EXPECT_TRUE(released.consumed);
	mouse.position = {12, 15};
	spk::MouseButtonDoubleClickedRecord doubleRecord{};
	doubleRecord.button = spk::Mouse::Button::Left;
	spk::MouseButtonDoubleClickedEvent doubled(doubleRecord, mouse);
	edit.dispatch(doubled);
	EXPECT_EQ(edit.selectedText(), U"hello");
	spk::MouseLeftRecord leftRecord{};
	spk::MouseLeftEvent left(leftRecord);
	edit.dispatch(left);
	EXPECT_FALSE(edit.isHovered());
	spk::WindowFocusLostRecord lostRecord{};
	spk::WindowFocusLostEvent lost(lostRecord);
	edit.dispatch(lost);
	EXPECT_FALSE(edit.isFocused());
}

TEST(TextEditTest, ValidationRejectsAtomicallyAndCallbackExceptionsLeaveTextCoherent)
{
	using State = spk::TextEdit::ValidationState;
	spk::TextEdit edit("Edit");
	edit.notifyFocusAcquired(spk::FocusMode::Channel::Keyboard);
	edit.setValidationCallback([](const auto &text) {
		return text.empty() ? State::Undefined : (text.find(U'x') != text.npos ? State::Invalid : State::Valid);
	});
	EXPECT_EQ(edit.validationState(), State::Undefined);
	type(edit, U'a');
	EXPECT_EQ(edit.validationState(), State::Valid);
	edit.selectAll();
	type(edit, U'x');
	EXPECT_EQ(edit.text(), U"a");
	EXPECT_EQ(edit.selectedText(), U"a");
	key(edit, spk::Keyboard::Delete);
	EXPECT_TRUE(edit.text().empty());
	EXPECT_EQ(edit.validationState(), State::Undefined);
	edit.setValidationCallback([](const auto &) -> State {
		throw std::runtime_error("validation");
	});
	EXPECT_ANY_THROW(type(edit, U'a'));
	EXPECT_TRUE(edit.text().empty());
	EXPECT_EQ(edit.cursor(), 0u);
	edit.setValidationCallback({});
	auto contract = edit.subscribeToEdition([](const auto &) {
		throw std::runtime_error("edition");
	});
	EXPECT_ANY_THROW(edit.setText("committed"));
	EXPECT_EQ(edit.text(), U"committed");
	EXPECT_EQ(edit.cursor(), 9u);
	contract.resign();
	EXPECT_NO_THROW(edit.setText("recovered"));
	EXPECT_EQ(edit.text(), U"recovered");
}

TEST(TextEditTest, ReadOnlyObscuredAndInvalidResourcesPreserveState)
{
	spk::TextEdit edit("Edit");
	edit.setText("secret");
	edit.selectAll();
	edit.setObscured(true);
	EXPECT_NE(edit.renderedText(), edit.text());
	EXPECT_EQ(edit.renderedText().size(), edit.text().size());
	EXPECT_FALSE(edit.copySelection());
	EXPECT_FALSE(edit.cutSelection());
	edit.disableEdit();
	edit.notifyFocusAcquired(spk::FocusMode::Channel::Keyboard);
	EXPECT_FALSE(edit.isFocused());
	EXPECT_FALSE(type(edit, U'x'));
	EXPECT_FALSE(edit.pasteClipboard());
	EXPECT_FALSE(edit.cutSelection());
	EXPECT_EQ(edit.text(), U"secret");
	edit.enableEdit();
	edit.setCopyObscuredTextEnabled(true);
	EXPECT_TRUE(edit.isCopyObscuredTextEnabled());
	const auto sheet = edit.spriteSheet();
	const auto font = edit.font();
	const auto corners = edit.cornerSize();
	EXPECT_THROW(edit.setSpriteSheet(nullptr), std::invalid_argument);
	EXPECT_THROW(edit.setSpriteSheet(spk::Widget::defaultStyle->iconset.get()), std::invalid_argument);
	EXPECT_THROW(edit.setFont(nullptr), std::invalid_argument);
	EXPECT_THROW(edit.setCornerSize({-1, 0}), std::invalid_argument);
	EXPECT_THROW(edit.setCornerSize({0, -1}), std::invalid_argument);
	EXPECT_EQ(edit.spriteSheet(), sheet);
	EXPECT_EQ(edit.font(), font);
	EXPECT_EQ(edit.cornerSize(), corners);
}

TEST(TextEditTest, ClipboardCopyCutPasteAndContentionPreserveTextAndSelection)
{
	struct Restore
	{
		std::optional<spk::Font::Text> previous = spk::Clipboard::readText();
		bool empty = CountClipboardFormats() == 0;
		~Restore()
		{
			if (previous)
			{
				(void)spk::Clipboard::writeText(*previous);
			}
			else if (empty && OpenClipboard(nullptr))
			{
				EmptyClipboard();
				CloseClipboard();
			}
		}
	} restore;
	if (!restore.previous && !restore.empty)
	{
		GTEST_SKIP() << "Preserving existing non-text clipboard contents";
	}
	spk::TextEdit edit("Edit");
	edit.setText(U"A\u00e9\U0001f600Z");
	edit.setSelection(1, 3);
	ASSERT_TRUE(edit.copySelection());
	EXPECT_EQ(spk::Clipboard::readText(), U"\u00e9\U0001f600");
	ASSERT_TRUE(edit.cutSelection());
	EXPECT_EQ(edit.text(), U"AZ");
	EXPECT_EQ(edit.cursor(), 1u);
	ASSERT_TRUE(edit.pasteClipboard());
	EXPECT_EQ(edit.text(), U"A\u00e9\U0001f600Z");
	edit.selectAll();
	edit.setObscured(true);
	EXPECT_FALSE(edit.copySelection());
	edit.setCopyObscuredTextEnabled(true);
	ASSERT_TRUE(edit.copySelection());
	EXPECT_EQ(spk::Clipboard::readText(), edit.text());
	std::promise<bool> opened;
	auto ready = opened.get_future();
	std::promise<void> release;
	auto released = release.get_future();
	std::jthread holder([&] {
		const HWND owner = CreateWindowExW(0, L"STATIC", L"Sparkle clipboard lock", 0, 0, 0, 1, 1, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
		const bool success = owner != nullptr && OpenClipboard(owner);
		opened.set_value(success);
		if (success)
		{
			released.wait();
			CloseClipboard();
		}
		if (owner)
		{
			DestroyWindow(owner);
		}
	});
	const bool locked = ready.get();
	if (locked)
	{
		const auto before = edit.text();
		const auto selection = edit.selection();
		EXPECT_FALSE(edit.copySelection());
		EXPECT_FALSE(edit.cutSelection());
		EXPECT_FALSE(edit.pasteClipboard());
		EXPECT_EQ(edit.text(), before);
		EXPECT_EQ(edit.selection(), selection);
	}
	release.set_value();
	EXPECT_TRUE(locked);
}
