#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/context/update_context.hpp"
#include "engine/behaviour.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/engine_widget.hpp"

namespace
{
	class RecordingBehaviour final : public spk::Behaviour
	{
	public:
		int updates = 0;
		int mousePresses = 0;
		int keyPresses = 0;

	protected:
		void _updateState(spk::UpdateContext &) override
		{
			++updates;
		}
		void _onMouseButtonPressedEvent(spk::MouseButtonPressedEvent &) override
		{
			++mousePresses;
		}
		void _onKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			++keyPresses;
		}
	};

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

TEST(EngineWidgetTest, DefaultEngineIsNull)
{
	spk::Widget parent("Parent", nullptr);
	spk::EngineWidget widget("EngineWidget", &parent);
	EXPECT_EQ(widget.engine(), nullptr);
	const spk::EngineWidget &constant = widget;
	EXPECT_EQ(constant.engine(), nullptr);
}

TEST(EngineWidgetTest, SetEngineRoundTripDoesNotTransferOwnership)
{
	spk::Widget parent("Parent", nullptr);
	spk::Engine engine;
	spk::EngineWidget widget("EngineWidget", &parent);
	widget.setEngine(&engine);
	EXPECT_EQ(widget.engine(), &engine);
	const spk::EngineWidget &constant = widget;
	EXPECT_EQ(constant.engine(), &engine);
	widget.setEngine(nullptr);
	EXPECT_EQ(widget.engine(), nullptr);
}

TEST(EngineWidgetTest, GeometryCanBeChangedWithoutEngine)
{
	spk::Widget parent("Parent", nullptr);
	spk::EngineWidget widget("EngineWidget", &parent);
	widget.setGeometry({.anchor = {12, 18}, .size = {320, 180}});
	EXPECT_EQ(widget.geometry().anchor, spk::Vector2Int(12, 18));
	EXPECT_EQ(widget.geometry().size, spk::Vector2UInt(320, 180));
}

TEST(EngineWidgetTest, AttachingAndResizingPropagatesGeometry)
{
	spk::Widget parent("Parent", nullptr);
	spk::Engine engine;
	spk::EngineWidget widget("EngineWidget", &parent);
	widget.setGeometry({.anchor = {12, 18}, .size = {320, 180}});
	widget.setEngine(&engine);
	EXPECT_EQ(engine.geometry(), widget.geometry());
	widget.setGeometry({.anchor = {-5, 7}, .size = {640, 360}});
	EXPECT_EQ(engine.geometry(), widget.geometry());
}

TEST(EngineWidgetTest, NullEngineUpdateAndSnapshotAreNoOps)
{
	spk::Widget parent("Parent", nullptr);
	spk::EngineWidget widget("EngineWidget", &parent);
	widget.activate();
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext context{.time = {}, .deltaTime = std::chrono::milliseconds(16), .keyboard = keyboard, .mouse = mouse};
	EXPECT_NO_THROW(widget.updateState(context));
	spk::RenderSnapshot::Builder builder;
	EXPECT_NO_THROW(widget.buildRenderSnapshot(builder));
}

TEST(EngineWidgetTest, UpdateAndInteractionAreForwardedToAttachedEngine)
{
	spk::Widget parent("Parent", nullptr);
	spk::Engine engine;
	auto &behaviour = engine.root().addBehaviour<RecordingBehaviour>();
	spk::EngineWidget widget("EngineWidget", &parent);
	widget.setEngine(&engine);
	widget.setGeometry({.anchor = {0, 0}, .size = {100, 60}});
	widget.activate();
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	mouse.position = {10, 10};
	spk::UpdateContext context{.time = {}, .deltaTime = std::chrono::milliseconds(16), .keyboard = keyboard, .mouse = mouse};
	widget.updateState(context);
	EXPECT_EQ(behaviour.updates, 1);
	spk::MouseButtonPressedRecord mouseRecord{};
	mouseRecord.button = spk::Mouse::Button::Left;
	spk::MouseButtonPressedEvent mouseEvent(mouseRecord, mouse);
	widget.dispatch(mouseEvent);
	EXPECT_EQ(behaviour.mousePresses, 1);
	spk::KeyPressedRecord keyRecord{};
	keyRecord.key = spk::Keyboard::A;
	spk::KeyPressedEvent keyEvent(keyRecord, keyboard);
	widget.dispatch(keyEvent);
	EXPECT_EQ(behaviour.keyPresses, 1);
}

TEST(EngineWidgetRenderTest, DISABLED_NoEngineSmallViewport)
{
	spk::Widget parent("Parent", nullptr);
	parent.setGeometry({.anchor = {0, 0}, .size = {640, 480}});
	parent.activate();
	spk::EngineWidget widget("EngineWidget", &parent);
	widget.setEngine(nullptr);
	widget.setGeometry({.anchor = {40, 40}, .size = {220, 140}});
	widget.activate();
	expectWidgetImage(widget, "ui/widget/engine_widget", "no_engine_small_viewport");
}

TEST(EngineWidgetRenderTest, DISABLED_NoEngineLargeViewport)
{
	spk::Widget parent("Parent", nullptr);
	parent.setGeometry({.anchor = {0, 0}, .size = {640, 480}});
	parent.activate();
	spk::EngineWidget widget("EngineWidget", &parent);
	widget.setEngine(nullptr);
	widget.setGeometry({.anchor = {20, 20}, .size = {500, 360}});
	widget.activate();
	expectWidgetImage(widget, "ui/widget/engine_widget", "no_engine_large_viewport");
}
