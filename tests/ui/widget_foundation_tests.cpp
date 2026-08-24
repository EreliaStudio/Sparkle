#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string_view>

#include "core/context/update_context.hpp"
#include "graphics/resource.hpp"
#include "input/keyboard.hpp"
#include "input/mouse.hpp"
#include "type/horizontal_alignment.hpp"
#include "type/vertical_alignment.hpp"
#include "ui/widget/animation_label.hpp"
#include "ui/widget/checkable_icon_button.hpp"
#include "ui/widget/container_widget.hpp"
#include "ui/widget/dynamic_text_label.hpp"
#include "ui/widget/icon_button.hpp"
#include "ui/widget/image_label.hpp"
#include "ui/widget/panel.hpp"
#include "ui/widget/push_button.hpp"
#include "ui/widget/screen.hpp"
#include "ui/widget/scroll_area.hpp"
#include "ui/widget/scroll_bar.hpp"
#include "ui/widget/slider_bar.hpp"
#include "ui/widget/spacer_widget.hpp"
#include "ui/widget/text_area.hpp"
#include "ui/widget/text_label.hpp"

namespace
{
	void require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	std::span<const std::uint8_t> resourceBytes(std::string_view path)
	{
		const spk::resources::Data data = spk::resources::get(path);
		return {reinterpret_cast<const std::uint8_t *>(data.data()), data.size()};
	}

	spk::Font loadEmbeddedFont()
	{
		const spk::resources::Data data = spk::resources::get("fonts/arial.ttf");
		spk::Font::Data fontData(data.size());
		std::memcpy(fontData.data(), data.data(), data.size());
		return spk::Font::fromRawData(std::move(fontData));
	}

	template <typename TException, typename TFunction>
	void requireThrows(TFunction &&function, std::string_view message)
	{
		try
		{
			function();
		} catch (const TException &)
		{
			return;
		}
		throw std::runtime_error(std::string(message));
	}

	void testActivationDefaults()
	{
		spk::SpacerWidget spacer("spacer");
		spk::ContainerWidget container("container");
		spk::ImageLabel image("image");
		spk::Panel panel("panel");
		spk::TextLabel text("text");
		spk::Screen screen("screen");

		require(spacer.isActive(), "SpacerWidget should activate itself");
		require(spacer.maximalSize().x > 0.0f && spacer.maximalSize().y > 0.0f, "SpacerWidget should be able to consume layout space");
		require(container.isActive(), "ContainerWidget should activate itself");
		require(image.isActive(), "ImageLabel should activate itself");
		require(panel.isActive(), "Panel should activate itself");
		require(text.isActive(), "TextLabel should activate itself");
		require(!screen.isActive(), "Screen should not activate itself");
	}

	void testScreenSelection()
	{
		spk::Screen first("first");
		first.activate();
		require(spk::Screen::activeScreen() == &first, "Activating a screen should select it");

		{
			spk::Screen second("second");
			second.activate();
			require(!first.isActive(), "Selecting a second screen should deactivate the first");
			require(spk::Screen::activeScreen() == &second, "The last activated screen should be selected");
			second.deactivate();
			require(spk::Screen::activeScreen() == &second, "Ordinary deactivation should preserve the legacy selected-screen pointer");
		}

		require(spk::Screen::activeScreen() == nullptr, "Destroying the selected screen should clear the selection");
	}

	void testContainerGeometryAndValidation()
	{
		spk::ContainerWidget container("container");
		spk::SpacerWidget content("content", &container);
		spk::SpacerWidget unrelated("unrelated");

		requireThrows<std::invalid_argument>([&]() {
			container.setContent(&unrelated);
		},
											 "ContainerWidget should reject a non-child content widget");
		container.setContent(&content);
		container.setContentAnchor({-12, -4});
		container.setContentSize({300, 200});
		require(content.geometry() == spk::Rect2D{spk::Vector2Int{-12, -4}, spk::Vector2UInt{300, 200}}, "ContainerWidget should apply the configured content geometry");

		content.setParent(nullptr);
		require(container.content() == nullptr, "ContainerWidget should not retain content that is no longer its child");
	}

	void testValidationAndConfiguration()
	{
		spk::ImageLabel image("image");
		requireThrows<std::invalid_argument>([&]() {
			image.setTexture(nullptr);
		},
											 "ImageLabel should reject a null texture assignment");

		spk::Panel panel("panel");
		requireThrows<std::invalid_argument>([&]() {
			panel.setSpriteSheet(nullptr);
		},
											 "Panel should reject a null sprite sheet assignment");
		requireThrows<std::invalid_argument>([&]() {
			panel.setCornerSize({-1, 2});
		},
											 "Panel should reject negative corner sizes");

		spk::TextLabel text("text");
		requireThrows<std::invalid_argument>([&]() {
			text.setFont(nullptr);
		},
											 "TextLabel should reject a null font assignment");
		text.setText("Sparkle");
		text.setTextSize({24, 2});
		text.setPadding({3, 4});
		text.setAlignment(spk::HorizontalAlignment::Right, spk::VerticalAlignment::Bottom);
		require(text.text() == U"Sparkle", "TextLabel should convert UTF-8 text");
		require(text.textSize() == spk::Font::Size{24, 2}, "TextLabel should retain its configured text size");
		require(text.padding() == spk::Vector2UInt{3, 4}, "TextLabel should retain its configured padding");
		require(text.horizontalAlignment() == spk::HorizontalAlignment::Right, "TextLabel should retain horizontal alignment");
		require(text.verticalAlignment() == spk::VerticalAlignment::Bottom, "TextLabel should retain vertical alignment");
		require(text.minimalSize() == spk::Vector2{0.0f, 0.0f}, "Text without a font should have no intrinsic size");
	}

	void testAnimationLabel()
	{
		using namespace std::chrono_literals;

		spk::SpriteSheet sprites(resourceBytes("textures/default_iconset.png"), {10, 10});
		spk::AnimationLabel animation("animation", &sprites);
		spk::Keyboard keyboard;
		spk::Mouse mouse;
		spk::UpdateContext update{.time = 125ms, .deltaTime = 125ms, .keyboard = keyboard, .mouse = mouse};

		require(animation.currentFrame() == 0, "AnimationLabel should begin at frame zero");
		require(animation.rangeEnd() == 99, "AnimationLabel should initially cover the whole sheet");
		animation.setAnimationRange(2, 3);
		animation.updateState(update);
		require(animation.currentFrame() == 3, "AnimationLabel should advance inside its custom range");
		animation.updateState(update);
		require(animation.currentFrame() == 2, "AnimationLabel should wrap its custom range");
		requireThrows<std::invalid_argument>([&]() {
			animation.setAnimationRange(4, 3);
		},
											 "AnimationLabel should reject a reversed range");
	}

	void testDynamicTextLabel()
	{
		int generation = 0;
		spk::DynamicTextLabel label("dynamic");
		label.setTextProducer([&]() {
			return "generation " + std::to_string(++generation);
		});
		require(generation == 1 && label.text() == U"generation 1", "DynamicTextLabel should evaluate a producer immediately");
		label.refresh();
		require(generation == 2 && label.text() == U"generation 2", "DynamicTextLabel should refresh on request");
	}

	void testTextAreaMeasurement()
	{
		spk::Font font = loadEmbeddedFont();
		spk::TextArea area("area", &font);
		area.setTextSize(16);
		area.setMinimalWidth(80);
		area.setLinePadding(3);
		require(area.computePreferredSize(20) == spk::Vector2UInt{80, 0}, "An empty TextArea should retain its minimal width and have zero height");

		area.setText("line");
		const spk::Vector2UInt oneLine = area.computePreferredSize(80);
		area.setText("line\n");
		const spk::Vector2UInt trailingBlankLine = area.computePreferredSize(80);
		require(trailingBlankLine.y == oneLine.y * 2 + area.linePadding(), "TextArea should preserve a trailing blank line");

		area.setText("averylongwordthatmustnotbesplit");
		require(area.computePreferredSize(80).x > 80, "TextArea should not split a single over-wide word");
	}

	void testPushButtonInteraction()
	{
		spk::PushButton button("button");
		button.setGeometry({spk::Vector2Int{10, 20}, spk::Vector2UInt{100, 50}});
		int clicks = 0;
		auto clickContract = button.subscribeToClick([&]() {
			++clicks;
		});

		spk::Mouse mouse;
		mouse.position = {30, 40};
		spk::MouseButtonPressedRecord pressedRecord;
		pressedRecord.button = spk::Mouse::Button::Left;
		spk::MouseButtonPressedEvent pressedEvent(pressedRecord, mouse);
		button.dispatch(pressedEvent);
		require(pressedEvent.consumed && button.isPressed(), "PushButton should consume a left press inside its viewport");
		require(!pressedEvent.focusChange(spk::FocusMode::Channel::Keyboard).has_value() && !pressedEvent.focusChange(spk::FocusMode::Channel::Mouse).has_value(), "PushButton should not request focus");

		spk::MouseButtonReleasedRecord releasedRecord;
		releasedRecord.button = spk::Mouse::Button::Left;
		spk::MouseButtonReleasedEvent releasedInside(releasedRecord, mouse);
		button.dispatch(releasedInside);
		require(releasedInside.consumed && clicks == 1 && !button.isPressed(), "PushButton should click once on release inside after a press");

		spk::MouseButtonPressedEvent secondPress(pressedRecord, mouse);
		button.dispatch(secondPress);
		mouse.position = {200, 200};
		spk::MouseButtonReleasedEvent releasedOutside(releasedRecord, mouse);
		button.dispatch(releasedOutside);
		require(!releasedOutside.consumed && clicks == 1 && !button.isPressed(), "PushButton should cancel a release outside");

		button.setFlat(true);
		require(!button.releasedBackground().isActive() && !button.pressedBackground().isActive(), "A flat PushButton should hide both backgrounds");

		mouse.position = {30, 40};
		spk::MouseButtonDoubleClickedRecord doubleClickedRecord;
		doubleClickedRecord.button = spk::Mouse::Button::Left;
		spk::MouseButtonDoubleClickedEvent doubleClickedEvent(doubleClickedRecord, mouse);
		button.dispatch(doubleClickedEvent);
		require(doubleClickedEvent.consumed && button.isPressed(), "PushButton should treat a left double-click event as the second press");
		require(!doubleClickedEvent.focusChange(spk::FocusMode::Channel::Keyboard).has_value() && !doubleClickedEvent.focusChange(spk::FocusMode::Channel::Mouse).has_value(), "A double-click press should not request focus");
		spk::MouseButtonReleasedEvent doubleClickRelease(releasedRecord, mouse);
		button.dispatch(doubleClickRelease);
		require(doubleClickRelease.consumed && clicks == 2, "The release following a double-click event should emit the second ordinary click");
	}

	void testButtonIconsAndCheckableState()
	{
		spk::SpriteSheet sprites(resourceBytes("textures/default_iconset.png"), {10, 10});
		spk::IconButton icon("icon", &sprites, 0);
		icon.setGeometry({spk::Vector2Int{0, 0}, spk::Vector2UInt{80, 60}});
		icon.setIconSpriteID(spk::Vector2UInt{2, 0});
		icon.setIconSize({20, 20});
		icon.setIconPadding({5, 5});
		require(icon.iconSpriteID() == 2 && icon.hasIcon(), "IconButton should resolve coordinate sprite IDs and refresh its icon");
		require(icon.releasedIcon().geometry() == spk::Rect2D{spk::Vector2Int{30, 20}, spk::Vector2UInt{20, 20}}, "PushButton should center a forced icon size");
		require(icon.minimalSize() == spk::Vector2{30.0f, 30.0f}, "PushButton icon size and padding should contribute to its minimum");

		spk::CheckableIconButton checkable("checkable", &sprites, 0, 8);
		checkable.setGeometry({spk::Vector2Int{0, 0}, spk::Vector2UInt{50, 50}});
		require(checkable.uncheckedButton().isActive() && !checkable.checkedButton().isActive(), "CheckableIconButton should initially show only its unchecked child");
		int stateChanges = 0;
		auto stateContract = checkable.subscribeToState([&](bool checked) {
			require(checked, "The first CheckableIconButton transition should enter the checked state");
			++stateChanges;
		});

		spk::Mouse mouse;
		mouse.position = {25, 25};
		spk::MouseButtonPressedRecord pressedRecord;
		pressedRecord.button = spk::Mouse::Button::Left;
		spk::MouseButtonPressedEvent pressedEvent(pressedRecord, mouse);
		checkable.dispatch(pressedEvent);
		spk::MouseButtonReleasedRecord releasedRecord;
		releasedRecord.button = spk::Mouse::Button::Left;
		spk::MouseButtonReleasedEvent releasedEvent(releasedRecord, mouse);
		checkable.dispatch(releasedEvent);
		require(checkable.isChecked() && stateChanges == 1, "Clicking the visible CheckableIconButton child should toggle once");
		require(!checkable.uncheckedButton().isActive() && checkable.checkedButton().isActive(), "CheckableIconButton should show only its checked child after toggling");
	}

	void testSliderBarInteractionAndMapping()
	{
		spk::SliderBar slider("slider");
		slider.setGeometry({spk::Vector2Int{10, 20}, spk::Vector2UInt{200, 20}});
		slider.setScale(0.25f);
		require(slider.body().geometry() == spk::Rect2D{spk::Vector2Int{0, 0}, spk::Vector2UInt{50, 20}}, "SliderBar should derive its horizontal thumb length from its scale");

		int editions = 0;
		float lastRatio = -1.0f;
		auto editionContract = slider.subscribeToEdition([&](float ratio) {
			++editions;
			lastRatio = ratio;
		});

		spk::Mouse mouse;
		mouse.position = {110, 30};
		spk::MouseButtonPressedRecord pressedRecord;
		pressedRecord.button = spk::Mouse::Button::Left;
		spk::MouseButtonPressedEvent pressedEvent(pressedRecord, mouse);
		slider.dispatch(pressedEvent);
		require(pressedEvent.consumed && slider.isDragging(), "SliderBar should begin dragging after a track click");
		require(pressedEvent.focusChange(spk::FocusMode::Channel::Mouse).has_value() && pressedEvent.focusChange(spk::FocusMode::Channel::Mouse)->type == spk::FocusMode::ChangeType::Take, "SliderBar should request mouse focus while dragging");
		require(std::abs(slider.ratio() - 0.5f) < 0.0001f && editions == 1, "A SliderBar track click should center its thumb on the pointer");

		mouse.position = {140, 30};
		spk::MouseMovedRecord movedRecord;
		spk::MouseMovedEvent movedEvent(movedRecord, mouse);
		slider.dispatch(movedEvent);
		require(movedEvent.consumed && std::abs(lastRatio - 0.7f) < 0.0001f, "SliderBar dragging should map pointer displacement over its travel range");

		mouse.position = {500, 30};
		spk::MouseButtonReleasedRecord releasedRecord;
		releasedRecord.button = spk::Mouse::Button::Left;
		spk::MouseButtonReleasedEvent releasedEvent(releasedRecord, mouse);
		slider.dispatch(releasedEvent);
		require(releasedEvent.consumed && !slider.isDragging(), "SliderBar should finish a drag even when released outside");
		require(releasedEvent.focusChange(spk::FocusMode::Channel::Mouse).has_value() && releasedEvent.focusChange(spk::FocusMode::Channel::Mouse)->type == spk::FocusMode::ChangeType::Release, "SliderBar should release mouse focus after dragging");

		slider.setRange(-10.0f, 30.0f);
		slider.setValue(10.0f);
		require(std::abs(slider.ratio() - 0.5f) < 0.0001f && std::abs(slider.value() - 10.0f) < 0.0001f, "SliderBar should map values through its configured range");
		slider.setOrientation(spk::Orientation::Vertical);
		slider.setGeometry({spk::Vector2Int{0, 0}, spk::Vector2UInt{20, 200}});
		require(slider.body().geometry() == spk::Rect2D{spk::Vector2Int{0, 75}, spk::Vector2UInt{20, 50}}, "Vertical SliderBar geometry should mirror horizontal behavior");
		requireThrows<std::invalid_argument>([&]() {
			slider.setScale(0.0f);
		},
											 "SliderBar should reject a zero scale");
	}

	void testScrollBarComposition()
	{
		spk::SpriteSheet sprites(resourceBytes("textures/default_iconset.png"), {10, 10});
		spk::ScrollBar scroll("scroll", &sprites);
		scroll.setGeometry({spk::Vector2Int{0, 0}, spk::Vector2UInt{200, 20}});
		scroll.setStep(0.2f);
		require(scroll.negativeButton().geometry() == spk::Rect2D{spk::Vector2Int{0, 0}, spk::Vector2UInt{20, 20}}, "A horizontal ScrollBar should use a square negative button");
		require(scroll.slider().geometry() == spk::Rect2D{spk::Vector2Int{20, 0}, spk::Vector2UInt{160, 20}}, "A horizontal ScrollBar should fill its center with the slider");
		require(scroll.negativeButton().iconSpriteID() == 6 && scroll.positiveButton().iconSpriteID() == 7, "A horizontal ScrollBar should use left and right arrow icons");

		int editions = 0;
		auto editionContract = scroll.subscribeToEdition([&](float ratio) {
			require(std::abs(ratio - 0.2f) < 0.0001f, "The positive ScrollBar button should advance by one step");
			++editions;
		});
		spk::Mouse mouse;
		mouse.position = {190, 10};
		spk::MouseButtonPressedRecord pressedRecord;
		pressedRecord.button = spk::Mouse::Button::Left;
		spk::MouseButtonPressedEvent pressedEvent(pressedRecord, mouse);
		scroll.dispatch(pressedEvent);
		spk::MouseButtonReleasedRecord releasedRecord;
		releasedRecord.button = spk::Mouse::Button::Left;
		spk::MouseButtonReleasedEvent releasedEvent(releasedRecord, mouse);
		scroll.dispatch(releasedEvent);
		require(editions == 1 && std::abs(scroll.ratio() - 0.2f) < 0.0001f, "ScrollBar should forward ratio editions caused by its arrow buttons");

		scroll.setOrientation(spk::Orientation::Vertical);
		scroll.setGeometry({spk::Vector2Int{0, 0}, spk::Vector2UInt{20, 200}});
		require(scroll.negativeButton().geometry() == spk::Rect2D{spk::Vector2Int{0, 0}, spk::Vector2UInt{20, 20}}, "A vertical ScrollBar should use a square upper button");
		require(scroll.slider().geometry() == spk::Rect2D{spk::Vector2Int{0, 20}, spk::Vector2UInt{20, 160}}, "A vertical ScrollBar should fill its center with the slider");
		require(scroll.negativeButton().iconSpriteID() == 4 && scroll.positiveButton().iconSpriteID() == 5, "A vertical ScrollBar should use up and down arrow icons");
		requireThrows<std::invalid_argument>([&]() {
			scroll.setStep(1.1f);
		},
											 "ScrollBar should reject a step larger than one");
	}

	void testScrollAreaSynchronization()
	{
		spk::IScrollArea area("area");
		spk::Widget content("content", &area.container());
		area.setContent(&content);
		area.setContentSize({200, 160});
		area.setGeometry({spk::Vector2Int{0, 0}, spk::Vector2UInt{100, 80}});
		require(area.viewSize() == spk::Vector2UInt{84, 64}, "IScrollArea should subtract both visible scrollbars from its view");
		require(std::abs(area.horizontalScrollBar().scale() - 0.42f) < 0.0001f && std::abs(area.verticalScrollBar().scale() - 0.4f) < 0.0001f, "IScrollArea should derive thumb scales from its current view and content sizes");

		area.horizontalScrollBar().setRatio(1.0f);
		area.verticalScrollBar().setRatio(1.0f);
		require(area.container().contentAnchor() == spk::Vector2Int{-116, -96}, "IScrollArea ratio one should expose the exact far edge of oversized content");

		area.resize({spk::Vector2Int{0, 0}, spk::Vector2UInt{150, 100}});
		require(area.viewSize() == spk::Vector2UInt{134, 84}, "IScrollArea should recompute its view after resizing");
		require(std::abs(area.horizontalScrollBar().scale() - 0.67f) < 0.0001f && std::abs(area.verticalScrollBar().scale() - 0.525f) < 0.0001f, "IScrollArea should recompute both thumb scales after resizing");
		require(area.container().contentAnchor() == spk::Vector2Int{-66, -76}, "IScrollArea should recompute overflow offsets after resizing");

		area.setScrollBarVisible(spk::Orientation::Vertical, false);
		require(area.viewSize() == spk::Vector2UInt{150, 84} && !area.verticalScrollBar().isActive(), "Hiding the vertical scrollbar should enlarge the view and deactivate the bar");
		area.setContentSize({100, 80});
		require(area.container().contentAnchor() == spk::Vector2Int{0, 0} && area.horizontalScrollBar().scale() == 1.0f, "Content smaller than the view should remain at the origin with a full thumb");

		area.setScrollBarVisible(spk::Orientation::Vertical, true);
		area.setContentSize({200, 200});
		area.verticalScrollBar().setRatio(0.5f);
		spk::Mouse mouse;
		mouse.position = {50, 50};
		spk::MouseWheelScrolledRecord wheelRecord;
		wheelRecord.value = {0.0f, 1.0f};
		spk::MouseWheelScrolledEvent wheelEvent(wheelRecord, mouse);
		area.dispatch(wheelEvent);
		require(wheelEvent.consumed && std::abs(area.verticalScrollBar().ratio() - 0.4f) < 0.0001f, "The mouse wheel should move the visible vertical scrollbar by one step");

		spk::ScrollArea<spk::SpacerWidget> typedArea("typed-area");
		require(typedArea.content() == &typedArea.contentObject() && typedArea.contentObject().parent() == &typedArea.container(), "ScrollArea<T> should own and register its typed content inside the container");
	}
}

int main()
{
	testActivationDefaults();
	testScreenSelection();
	testContainerGeometryAndValidation();
	testValidationAndConfiguration();
	testAnimationLabel();
	testDynamicTextLabel();
	testTextAreaMeasurement();
	testPushButtonInteraction();
	testButtonIconsAndCheckableState();
	testSliderBarInteractionAndMapping();
	testScrollBarComposition();
	testScrollAreaSynchronization();
}
