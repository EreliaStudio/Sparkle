#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "engine_view_test_utils.hpp"
#include "rendering/engine/rendering_system.hpp"
#include <functional>
#include <optional>

namespace
{
	using Fixture = sparkle_test::engine_view::Fixture;
	using Layers = spk::EngineView::Layers;
	using Context = spk::EngineView::CaptureContext;

	class SystemProbe final : public spk::RenderingSystem
	{
	public:
		using ViewCaptureContext = spk::EngineView::CaptureContext;
		std::size_t contributions = 0;
		std::function<void(const ViewCaptureContext &)> callback;

		void buildViewRenderSnapshot(spk::RenderSnapshot::Builder &, const ViewCaptureContext &ctx) override
		{
			++contributions;
			if (callback)
			{
				callback(ctx);
			}
		}
	};
}

TEST(EngineViewLayersTest, UnconfiguredFilterAdmitsUnlayeredContributor)
{
	Fixture fixture;
	fixture.addView("view");
	auto &probe = fixture.record("visible");
	EXPECT_TRUE(probe.layers().empty());
	fixture.engine.refreshRenderSnapshot();
	fixture.executeLatest();
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"view:visible"}));
}

TEST(EngineViewLayersTest, ExplicitEmptyFilterRejectsEverything)
{
	Fixture fixture;
	auto &view = fixture.addView("view");
	auto &probe = fixture.record("invisible");
	probe.setLayers({"A"});
	view.setLayers({});
	fixture.engine.refreshRenderSnapshot();
	fixture.executeLatest();
	EXPECT_EQ(probe.contributions, 0u);
	EXPECT_TRUE(fixture.log.empty());
	EXPECT_TRUE(view.layers().has_value());
	EXPECT_TRUE(view.layers()->empty());
}

TEST(EngineViewLayersTest, MatchingSingleLayerAdmitsContributor)
{
	Fixture fixture;
	auto &view = fixture.addView("view");
	auto &probe = fixture.record("visible");
	probe.setLayers({"foreground"});
	view.setLayers({"foreground"});
	fixture.engine.refreshRenderSnapshot();
	fixture.executeLatest();
	EXPECT_EQ(probe.contributions, 1u);
}

TEST(EngineViewLayersTest, DisjointLayersRejectContributor)
{
	Fixture fixture;
	auto &view = fixture.addView("view");
	auto &probe = fixture.record("invisible");
	probe.setLayers({"foreground"});
	view.setLayers({"background"});
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 0u);
}

TEST(EngineViewLayersTest, MultipleLayersUseNonemptyIntersection)
{
	Fixture fixture;
	auto &view = fixture.addView("view");
	auto &probe = fixture.record("visible");
	probe.setLayers({"sky", "terrain", "characters"});
	view.setLayers({"hud", "characters"});
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 1u);
}

TEST(EngineViewLayersTest, UnlayeredContributorIsExcludedByConfiguredFilter)
{
	Fixture fixture;
	auto &view = fixture.addView("view");
	auto &probe = fixture.record("hidden");
	view.setLayers({"foreground"});
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 0u);
}

TEST(EngineViewLayersTest, LayerFilterCanBeReplacedBetweenCaptures)
{
	Fixture fixture;
	auto &view = fixture.addView("view");
	auto &probe = fixture.record("visible");
	probe.setLayers({"A"});
	view.setLayers({"B"});
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 0u);
	view.setLayers({"A"});
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 1u);
}

TEST(EngineViewLayersTest, FilterResetRestoresUnconfiguredAdmission)
{
	Fixture fixture;
	auto &view = fixture.addView("view");
	auto &probe = fixture.record("visible");
	view.setLayers({});
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 0u);
	view.clearLayerFilter();
	EXPECT_FALSE(view.layers().has_value());
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 1u);
}

TEST(EngineViewLayersTest, TwoViewsHaveIndependentFilters)
{
	Fixture fixture;
	auto &a = fixture.addView("a");
	auto &b = fixture.addView("b");
	auto &probe = fixture.record("selected");
	probe.setLayers({"alpha"});
	a.setLayers({"alpha"});
	b.setLayers({"beta"});
	fixture.engine.refreshRenderSnapshot();
	fixture.executeLatest();
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"a:selected"}));
}

TEST(EngineViewLayersTest, CaptureContextHelperSupportsAllThreeFilterModes)
{
	Context context;
	EXPECT_TRUE(context.isLayerCompatible({}));
	EXPECT_TRUE(context.isLayerCompatible({"A"}));
	context.layers = Layers{};
	EXPECT_FALSE(context.isLayerCompatible({}));
	EXPECT_FALSE(context.isLayerCompatible({"A"}));
	context.layers = Layers{"B", "C"};
	EXPECT_FALSE(context.isLayerCompatible({}));
	EXPECT_FALSE(context.isLayerCompatible({"A"}));
	EXPECT_TRUE(context.isLayerCompatible({"A", "C"}));
}

TEST(EngineViewLayersTest, CaptureHelperUsesFrozenFilterNotMutatedView)
{
	Fixture fixture;
	auto &view = fixture.addView("view");
	view.setLayers({"A"});
	auto &probe = fixture.engine.root().addBehaviour<sparkle_test::engine_view::Probe>();
	probe.setLayers({"A"});
	bool captured = false;
	probe.callback = [&](spk::RenderSnapshot::Builder &, const Context &context) {
		captured = true;
		view.setLayers({"B"});
		EXPECT_TRUE(context.isLayerCompatible({"A"}));
		EXPECT_FALSE(context.isLayerCompatible({"B"}));
	};
	fixture.engine.refreshRenderSnapshot();
	EXPECT_TRUE(captured);
	EXPECT_EQ(view.layers(), (std::optional<Layers>{Layers{"B"}}));
}

TEST(EngineViewLayersTest, InactiveContributorDoesNotContribute)
{
	Fixture fixture;
	fixture.addView("view");
	auto &probe = fixture.record("hidden");
	probe.deactivate();
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 0u);
}

TEST(EngineViewLayersTest, RenderingSystemMayFilterInternalLayersExplicitly)
{
	Fixture fixture;
	auto &view = fixture.addView("view");
	view.setLayers({"world"});
	auto &probe = fixture.engine.addSystem<SystemProbe>();
	bool selected = false;
	probe.callback = [&](const Context &ctx) {
		selected = ctx.isLayerCompatible({"world"}) && !ctx.isLayerCompatible({"ui"});
	};
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 0u);
	EXPECT_FALSE(selected);
	probe.setLayers({"world"});
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 1u);
	EXPECT_TRUE(selected);
}
