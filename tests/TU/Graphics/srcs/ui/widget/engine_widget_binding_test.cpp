#include <gtest/gtest.h>

#include "engine/engine_headless.hpp"
#include "input/keyboard.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/engine/rendering_behaviour.hpp"
#include "ui/widget/engine_widget.hpp"

namespace
{
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

TEST(EngineWidgetBindingTest, BindingGraphicalEngineThroughHeadlessPointerRetainsEventRouting)
{
	spk::Engine engine;
	auto &probe = engine.root().addBehaviour<WidgetKeyProbe>();
	spk::EngineHeadless *headless = &engine;
	spk::EngineWidget widget("Engine widget", nullptr);
	widget.activate();
	widget.setEngine(headless);

	sendWidgetKey(widget);
	EXPECT_EQ(probe.presses, 1);
	EXPECT_EQ(widget.engine(), headless);
}

TEST(EngineWidgetBindingTest, SwitchingToPlainHeadlessEngineStopsGraphicalDispatch)
{
	spk::Engine graphical;
	spk::EngineHeadless headless;
	auto &probe = graphical.root().addBehaviour<WidgetKeyProbe>();
	spk::EngineWidget widget("Engine widget", nullptr);
	widget.activate();
	widget.setEngine(&graphical);
	sendWidgetKey(widget);

	widget.setEngine(&headless);
	sendWidgetKey(widget);
	EXPECT_EQ(probe.presses, 1);
	EXPECT_EQ(widget.engine(), &headless);
}

TEST(EngineWidgetBindingTest, SwitchingBackToGraphicalEngineRestoresEventRouting)
{
	spk::Engine graphical;
	spk::EngineHeadless headless;
	auto &probe = graphical.root().addBehaviour<WidgetKeyProbe>();
	spk::EngineWidget widget("Engine widget", nullptr);
	widget.activate();
	widget.setEngine(&headless);
	sendWidgetKey(widget);

	widget.setEngine(&graphical);
	sendWidgetKey(widget);
	EXPECT_EQ(probe.presses, 1);
}
