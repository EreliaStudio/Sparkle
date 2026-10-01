#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "design_pattern/trait/geometry_state_trait.hpp"

namespace
{
	class GeometryStateProbe : public spk::GeometryStateTrait
	{
	public:
		std::vector<std::string> calls;
		std::vector<spk::Rect2D> observedGeometries;

	protected:
		void _onSetGeometry() override
		{
			calls.emplace_back("set");
			observedGeometries.push_back(geometry());
		}

		void _onGeometryChange(const spk::Rect2D &geometry) override
		{
			calls.emplace_back("change");
			observedGeometries.push_back(geometry);
		}

		void _afterGeometryChange(const spk::Rect2D &geometry) override
		{
			calls.emplace_back("after");
			observedGeometries.push_back(geometry);
		}
	};
}

TEST(GeometryStateTraitTest, SetWithNotificationDisabledOnlyStoresGeometry)
{
	GeometryStateProbe probe;
	const spk::Rect2D geometry{.anchor = {21, 43}, .size = {65, 87}};

	probe.setGeometry(geometry, false);

	EXPECT_EQ(probe.geometry(), geometry);
	EXPECT_EQ(probe.calls, (std::vector<std::string>{"set"}));
	EXPECT_EQ(probe.observedGeometries, (std::vector<spk::Rect2D>{geometry}));
}

TEST(GeometryStateTraitTest, DefaultSetStoresGeometryThenRunsAllHooksInOrder)
{
	GeometryStateProbe probe;
	const spk::Rect2D geometry{.anchor = {12, 34}, .size = {56, 78}};

	probe.setGeometry(geometry);

	EXPECT_EQ(probe.geometry(), geometry);
	EXPECT_EQ(probe.calls, (std::vector<std::string>{"set", "change", "after"}));
	EXPECT_EQ(probe.observedGeometries, (std::vector<spk::Rect2D>{geometry, geometry, geometry}));
}

TEST(GeometryStateTraitTest, ExplicitNotificationRunsAllHooks)
{
	GeometryStateProbe probe;
	const spk::Rect2D geometry{.anchor = {-9, 17}, .size = {31, 47}};

	probe.setGeometry(geometry, true);

	EXPECT_EQ(probe.calls, (std::vector<std::string>{"set", "change", "after"}));
}

TEST(GeometryStateTraitTest, NotifyGeometryChangeRunsNotificationHooksForCurrentGeometry)
{
	GeometryStateProbe probe;
	const spk::Rect2D geometry{.anchor = {8, 16}, .size = {32, 64}};

	probe.setGeometry(geometry, false);
	probe.calls.clear();
	probe.observedGeometries.clear();

	probe.notifyGeometryChange();

	EXPECT_EQ(probe.geometry(), geometry);
	EXPECT_EQ(probe.calls, (std::vector<std::string>{"change", "after"}));
	EXPECT_EQ(probe.observedGeometries, (std::vector<spk::Rect2D>{geometry, geometry}));
}

TEST(GeometryStateTraitTest, RepeatedAssignmentStillRunsTheRequestedHooks)
{
	GeometryStateProbe probe;
	const spk::Rect2D geometry{.anchor = {1, 2}, .size = {3, 4}};

	probe.setGeometry(geometry);
	probe.setGeometry(geometry, false);
	probe.setGeometry(geometry);

	EXPECT_EQ(
		probe.calls,
		(std::vector<std::string>{"set", "change", "after", "set", "set", "change", "after"}));
}
