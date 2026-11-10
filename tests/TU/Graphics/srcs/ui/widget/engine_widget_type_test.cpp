#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <type_traits>

#include "core/context/update_context.hpp"
#include "engine/entity3d.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/render_snapshot.hpp"
#include "ui/widget/engine_widget.hpp"

namespace
{
	void prepare(spk::EngineWidget &widget)
	{
		widget.setGeometry({.anchor = {0, 0}, .size = {120, 80}});
		widget.activate();
	}

	void capture(spk::EngineWidget &widget)
	{
		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		(void)builder.build();
	}
}

TEST(EngineWidgetTypeTest, FlagsCombineWithoutAffectingOtherRoles)
{
	using Type = spk::EngineWidget::Type;
	static_assert(std::is_constructible_v<spk::EngineWidget, std::string, spk::Widget *, spk::Flags<Type>>);
	constexpr auto all = Type::Update | Type::Render | Type::Draw;
	static_assert(all.raw() == 7);
	spk::EngineWidget widget("widget", nullptr, Type::Draw);
	EXPECT_TRUE(widget.type().has(Type::Draw));
	EXPECT_FALSE(widget.type().has(Type::Render));
	EXPECT_FALSE(widget.type().has(Type::Update));
	widget.setType(Type::Render | Type::Draw);
	EXPECT_EQ(widget.type().raw(), 6u);
	widget.setType(Type::None);
	EXPECT_EQ(widget.type().raw(), 0u);
}

TEST(EngineWidgetTypeTest, InvalidFlagsAndViewNameLeaveConfigurationUnchanged)
{
	using Type = spk::EngineWidget::Type;
	spk::EngineWidget widget("widget", nullptr, Type::Draw);
	EXPECT_THROW(widget.setType(static_cast<Type>(0x80)), spk::Exception);
	EXPECT_EQ(widget.type().raw(), 4u);
	EXPECT_THROW(widget.setViewName(""), spk::Exception);
	EXPECT_EQ(widget.viewName(), "main");
	widget.setViewName("surveillance");
	EXPECT_EQ(widget.viewName(), "surveillance");
	EXPECT_THROW((void)spk::EngineWidget("invalid", nullptr, static_cast<Type>(0x80)), spk::Exception);
}

TEST(EngineWidgetTypeTest, DrawOnlyAndUpdateOnlyNeverPublishASnapshot)
{
	spk::Engine engine;
	spk::EngineWidget drawing("draw", nullptr, spk::EngineWidget::Type::Draw);
	drawing.setEngine(&engine);
	prepare(drawing);
	capture(drawing);
	spk::UpdateContext update{.time = {}, .deltaTime = std::chrono::milliseconds(16)};
	drawing.updateState(update);
	EXPECT_EQ(engine.latestRenderSnapshot(), nullptr);

	spk::EngineWidget updating("update", nullptr, spk::EngineWidget::Type::Update);
	updating.setEngine(&engine);
	prepare(updating);
	capture(updating);
	EXPECT_EQ(engine.latestRenderSnapshot(), nullptr);
}

TEST(EngineWidgetTypeTest, RenderOnlyCapturesExplicitlyAndWithoutUpdate)
{
	spk::Engine engine;
	spk::Entity3D entity("camera");
	auto &camera = entity.addComponent<spk::CameraComponent3D>();
	engine.addEntity(&entity);
	engine.addView("main", &camera, {40, 30});
	spk::EngineWidget widget("render", nullptr, spk::EngineWidget::Type::Render);
	widget.setEngine(&engine);
	prepare(widget);
	EXPECT_EQ(engine.latestRenderSnapshot(), nullptr);
	capture(widget);
	ASSERT_NE(engine.latestRenderSnapshot(), nullptr);
	const auto first = engine.latestRenderSnapshot();
	spk::UpdateContext update{};
	widget.updateState(update);
	EXPECT_EQ(engine.latestRenderSnapshot(), first);
	capture(widget);
	EXPECT_NE(engine.latestRenderSnapshot(), first);
	engine.removeEntity(&entity);
}

TEST(EngineWidgetTypeTest, DefaultHasAllThreeCapabilities)
{
	spk::EngineWidget widget("default", nullptr);
	EXPECT_EQ(widget.type().raw(), 7u);
	EXPECT_EQ(widget.viewName(), "main");
}
