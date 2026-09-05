#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include <chrono>

#include "core/context/update_context.hpp"
#include "ui/widget/dynamic_text_label.hpp"

namespace
{
	void expectWidgetImage(spk::Widget &widget, const std::filesystem::path &category, const std::string &name)
	{
		auto &context = sparkle_test::OpenGLTestContext::instance();
		context.reset();

		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		spk::RenderSnapshot snapshot = builder.build();
		snapshot.execute(context.renderContext());

		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		const auto difference = sparkle_test::resultImagePath(category, name + "_difference");
		context.save(actual);

		ASSERT_TRUE(std::filesystem::exists(expected))
			<< "Missing golden image: " << expected << "\n"
			<< "The current render was saved to: " << actual;

		const sparkle_test::ImageComparisonResult result =
			sparkle_test::compareImages(actual, expected, difference);
		EXPECT_TRUE(result.matches)
			<< "Image mismatch for [" << category.string() << "/" << name << "]\n"
			<< "Different pixels: " << result.differentPixelCount << "\n"
			<< "Actual size: " << result.actualWidth << "x" << result.actualHeight << "\n"
			<< "Expected size: " << result.expectedWidth << "x" << result.expectedHeight << "\n"
			<< "Difference image: " << difference;
	}

	[[nodiscard]] const spk::Widget::Style &defaultStyle()
	{
		return spk::Widget::defaultStyle.get();
	}
}

TEST(DynamicTextLabelTest, DefaultState)
{
	spk::DynamicTextLabel label("Dynamic");
	EXPECT_FALSE(static_cast<bool>(label.textProducer()));
	EXPECT_EQ(label.refreshDuration(), std::chrono::seconds(1));
}

TEST(DynamicTextLabelTest, ConstructorStoresProducerAndFont)
{
	int value = 7;
	spk::DynamicTextLabel label("Dynamic", defaultStyle().font.get(), [&] {
		return std::to_string(value);
	});
	EXPECT_TRUE(static_cast<bool>(label.textProducer()));
	label.refresh();
	EXPECT_EQ(label.text(), spk::Font::textFromUTF8("7"));
}

TEST(DynamicTextLabelTest, SetProducerRefreshAndDuration)
{
	spk::DynamicTextLabel label("Dynamic");
	int value = 0;
	label.setTextProducer([&] {
		return "Value=" + std::to_string(++value);
	});
	label.setRefreshDuration(std::chrono::milliseconds(25));
	EXPECT_EQ(label.refreshDuration(), std::chrono::milliseconds(25));
	EXPECT_EQ(label.text(), spk::Font::textFromUTF8("Value=1"));
	label.refresh();
	EXPECT_EQ(label.text(), spk::Font::textFromUTF8("Value=2"));
	label.refresh();
	EXPECT_EQ(label.text(), spk::Font::textFromUTF8("Value=3"));
}

TEST(DynamicTextLabelTest, UpdateRefreshesAtConfiguredInterval)
{
	int calls = 0;
	spk::DynamicTextLabel label("Dynamic", defaultStyle().font.get(), [&] {
		return std::to_string(++calls);
	});
	label.setRefreshDuration(std::chrono::milliseconds(20));
	label.activate();
	EXPECT_EQ(calls, 1);
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext shortTick{.time = {}, .deltaTime = std::chrono::milliseconds(10), .keyboard = keyboard, .mouse = mouse};
	label.updateState(shortTick);
	EXPECT_EQ(calls, 1);
	label.updateState(shortTick);
	EXPECT_EQ(calls, 2);
	EXPECT_EQ(label.text(), spk::Font::textFromUTF8("2"));
}

TEST(DynamicTextLabelTest, NullAndReplacementProducerFollowAcceptedContract)
{
	spk::DynamicTextLabel label("Dynamic");
	label.setText("preserved");
	label.setTextProducer({});
	EXPECT_FALSE(static_cast<bool>(label.textProducer()));
	EXPECT_EQ(label.text(), spk::Font::textFromUTF8("preserved"));
	label.setTextProducer([] {
		return std::string{};
	});
	EXPECT_TRUE(label.text().empty());
	label.setTextProducer([] {
		return std::string("replacement");
	});
	EXPECT_EQ(label.text(), spk::Font::textFromUTF8("replacement"));
}

TEST(DynamicTextLabelTest, ZeroAndNegativeDurationsRefreshOncePerUpdate)
{
	int calls = 0;
	spk::DynamicTextLabel label("Dynamic", defaultStyle().font.get(), [&] {
		return std::to_string(++calls);
	});
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext context{.time = {}, .deltaTime = std::chrono::milliseconds(100), .keyboard = keyboard, .mouse = mouse};
	label.setRefreshDuration(spk::DynamicTextLabel::Duration::zero());
	label.updateState(context);
	EXPECT_EQ(calls, 2);
	label.setRefreshDuration(-std::chrono::milliseconds(1));
	label.updateState(context);
	EXPECT_EQ(calls, 3);
}

TEST(DynamicTextLabelTest, ProducerExceptionsPropagateAndReplacementRemainsPossible)
{
	spk::DynamicTextLabel label("Dynamic");
	EXPECT_THROW(label.setTextProducer([]() -> std::string {
		throw std::runtime_error("producer failure");
	}),
				 std::runtime_error);
	label.setTextProducer([] {
		return std::string("recovered");
	});
	EXPECT_EQ(label.text(), spk::Font::textFromUTF8("recovered"));
}

TEST(DynamicTextLabelRenderTest, DISABLED_InitialValue)
{
	spk::DynamicTextLabel label("Dynamic", defaultStyle().font.get(), [] {
		return std::string("FPS: 60");
	});
	label.refresh();
	label.setGeometry({.anchor = {50, 50}, .size = {260, 60}});
	label.activate();
	expectWidgetImage(label, "ui/widget/dynamic_text_label", "initial_value");
}

TEST(DynamicTextLabelRenderTest, DISABLED_RefreshedValue)
{
	int value = 41;
	spk::DynamicTextLabel label("Dynamic", defaultStyle().font.get(), [&] {
		return "Counter: " + std::to_string(++value);
	});
	label.refresh();
	label.refresh();
	label.setGeometry({.anchor = {50, 50}, .size = {300, 60}});
	label.activate();
	expectWidgetImage(label, "ui/widget/dynamic_text_label", "refreshed_value");
}
