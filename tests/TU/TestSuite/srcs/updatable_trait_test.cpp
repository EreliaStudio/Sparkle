#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "core/context/update_context.hpp"
#include "design_pattern/trait/updatable_trait.hpp"
#include "input/keyboard.hpp"
#include "input/mouse.hpp"

namespace
{
	class UpdatableProbe : public spk::UpdatableTrait
	{
	public:
		bool canUpdate = true;
		std::vector<std::string> calls;

	protected:
		[[nodiscard]] bool _canUpdate() const override
		{
			return canUpdate;
		}

		void _beforeUpdate(spk::UpdateContext &) override
		{
			calls.emplace_back("before");
		}

		void _updateState(spk::UpdateContext &) override
		{
			calls.emplace_back("update");
		}

		void _afterUpdate(spk::UpdateContext &) override
		{
			calls.emplace_back("after");
		}
	};
}

TEST(UpdatableTraitTest, RunsPhasesInOrderAndHonorsEligibility)
{
	spk::Keyboard keyboard;
	spk::Mouse mouse;
	spk::UpdateContext context{.time = {}, .deltaTime = {}, .keyboard = keyboard, .mouse = mouse};
	UpdatableProbe probe;

	probe.updateState(context);
	EXPECT_EQ(probe.calls, (std::vector<std::string>{"before", "update", "after"}));

	probe.calls.clear();
	probe.canUpdate = false;
	probe.updateState(context);
	EXPECT_TRUE(probe.calls.empty());
}
