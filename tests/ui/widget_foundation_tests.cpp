#include <stdexcept>
#include <string_view>

#include "type/horizontal_alignment.hpp"
#include "type/vertical_alignment.hpp"
#include "ui/widget/container_widget.hpp"
#include "ui/widget/image_label.hpp"
#include "ui/widget/panel.hpp"
#include "ui/widget/screen.hpp"
#include "ui/widget/spacer_widget.hpp"
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
}

int main()
{
	testActivationDefaults();
	testScreenSelection();
	testContainerGeometryAndValidation();
	testValidationAndConfiguration();
}
