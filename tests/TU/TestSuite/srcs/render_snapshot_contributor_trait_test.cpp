#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "design_pattern/trait/render_snapshot_contributor_trait.hpp"

namespace
{
	class RenderProbe : public spk::RenderSnapshotContributorTrait
	{
	public:
		bool canBuild = true;
		std::vector<std::string> calls;

	protected:
		[[nodiscard]] bool _canBuildRenderSnapshot() const override
		{
			return canBuild;
		}

		void _beforeBuildRenderSnapshot(spk::RenderSnapshot::Builder &) override
		{
			calls.emplace_back("before");
		}

		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &) override
		{
			calls.emplace_back("build");
		}

		void _afterBuildRenderSnapshot(spk::RenderSnapshot::Builder &) override
		{
			calls.emplace_back("after");
		}
	};
}

TEST(RenderSnapshotContributorTraitTest, RunsPhasesInOrderAndHonorsEligibility)
{
	spk::RenderSnapshot::Builder builder;
	RenderProbe probe;

	probe.buildRenderSnapshot(builder);
	EXPECT_EQ(probe.calls, (std::vector<std::string>{"before", "build", "after"}));

	probe.calls.clear();
	probe.canBuild = false;
	probe.buildRenderSnapshot(builder);
	EXPECT_TRUE(probe.calls.empty());
}
