#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

#include "engine/engine_headless.hpp"
#include "input/keyboard.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/engine/rendering_behaviour.hpp"
#include "ui/widget/engine_widget.hpp"

namespace
{
	static_assert(std::is_same_v<decltype(&spk::EngineWidget::setEngine), void (spk::EngineWidget::*)(spk::Engine *) noexcept>);
	static_assert(std::is_same_v<decltype(std::declval<spk::EngineWidget &>().engine()), spk::Engine *>);
	static_assert(std::is_same_v<decltype(std::declval<const spk::EngineWidget &>().engine()), const spk::Engine *>);
	static_assert(!std::is_convertible_v<spk::EngineHeadless *, spk::Engine *>);

	class WidgetKeyProbe final : public spk::RenderingBehaviour
	{
	public:
		int presses = 0;

	protected:
		void _onKeyPressedEvent(spk::KeyPressedEvent &) override
		{
			++presses;
		}
	};

	void sendWidgetKey(spk::EngineWidget &widget)
	{
		spk::Keyboard keyboard;
		spk::KeyPressedRecord record{};
		record.key = spk::Keyboard::A;
		spk::KeyPressedEvent event(record, keyboard);
		widget.dispatch(event);
	}
}

TEST(EngineWidgetBindingTest, BindingGraphicalEngineRetainsEventRouting)
{
	spk::Engine engine;
	auto &probe = engine.root().addBehaviour<WidgetKeyProbe>();
	spk::EngineWidget widget("Engine widget", nullptr);
	widget.activate();
	widget.setEngine(&engine);

	sendWidgetKey(widget);
	EXPECT_EQ(probe.presses, 1);
	EXPECT_EQ(widget.engine(), &engine);
}

TEST(EngineWidgetBindingTest, SwitchingBetweenGraphicalEnginesTransfersEventRouting)
{
	spk::Engine first;
	spk::Engine second;
	auto &firstProbe = first.root().addBehaviour<WidgetKeyProbe>();
	auto &secondProbe = second.root().addBehaviour<WidgetKeyProbe>();
	spk::EngineWidget widget("Engine widget", nullptr);
	widget.activate();
	widget.setEngine(&first);
	sendWidgetKey(widget);

	widget.setEngine(&second);
	sendWidgetKey(widget);
	EXPECT_EQ(firstProbe.presses, 1);
	EXPECT_EQ(secondProbe.presses, 1);
	EXPECT_EQ(widget.engine(), &second);
}

TEST(EngineWidgetBindingTest, DetachmentStopsEventRouting)
{
	spk::Engine engine;
	auto &probe = engine.root().addBehaviour<WidgetKeyProbe>();
	spk::EngineWidget widget("Engine widget", nullptr);
	widget.activate();
	widget.setEngine(&engine);
	sendWidgetKey(widget);

	widget.setEngine(nullptr);
	sendWidgetKey(widget);
	EXPECT_EQ(probe.presses, 1);
	EXPECT_EQ(widget.engine(), nullptr);
}
