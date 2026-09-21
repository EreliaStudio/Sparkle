#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "core/context/update_context.hpp"
#include "design_pattern/trait/updatable_by_device_trait.hpp"
#include "input/device_context.hpp"
#include "input/keyboard.hpp"
#include "input/mouse.hpp"

namespace
{
	class DeviceProbe final : public spk::UpdatableByDeviceTrait
	{
	public:
		bool canUpdate = true;
		std::vector<std::string> calls;

	protected:
		[[nodiscard]] bool _canUpdateByDevice() const override
		{
			return canUpdate;
		}

		void _beforeUpdate(
			spk::UpdateContext &,
			spk::DeviceContext &) override
		{
			calls.emplace_back("before");
		}

		void _updateState(
			spk::UpdateContext &,
			spk::DeviceContext &) override
		{
			calls.emplace_back("update");
		}

		void _afterUpdate(
			spk::UpdateContext &,
			spk::DeviceContext &) override
		{
			calls.emplace_back("after");
		}
	};
}

TEST(UpdatableByDeviceTraitTest, RunsIndependentTwoContextLifecycle)
{
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext updateContext{};
	spk::DeviceContext deviceContext{
		.keyboard = &keyboard,
		.mouse = &mouse};
	DeviceProbe probe;

	probe.updateState(updateContext, deviceContext);
	EXPECT_EQ(
		probe.calls,
		(std::vector<std::string>{"before", "update", "after"}));

	probe.calls.clear();
	probe.canUpdate = false;
	probe.updateState(updateContext, deviceContext);
	EXPECT_TRUE(probe.calls.empty());
}
