#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "engine_view_test_utils.hpp"
#include "exception.hpp"

namespace
{
	using Fixture = sparkle_test::engine_view::Fixture;

	void expectInvalid(const spk::RenderPass::Key &key)
	{
		Fixture fixture;
		fixture.addView("invalid");
		auto &probe = fixture.engine.root().addBehaviour<sparkle_test::engine_view::Probe>();
		probe.callback = [key](spk::RenderSnapshot::Builder &builder, const spk::EngineView::CaptureContext &) {
			builder.renderPass(key);
		};
		EXPECT_THROW(fixture.engine.refreshRenderSnapshot(), spk::Exception);
		EXPECT_EQ(fixture.engine.latestRenderSnapshot(), nullptr);
	}
}

TEST(EngineViewPassValidationTest, AllFiveSystemKeysAreReservedExactly)
{
	const std::array keys{spk::EngineView::ShadowPassKey, spk::EngineView::SkyboxPassKey, spk::EngineView::OpaquePassKey, spk::EngineView::WBOITPassKey, spk::EngineView::CompositionPassKey};
	EXPECT_EQ(keys[0].name, "sparkle.engineview.shadow");
	EXPECT_EQ(keys[0].order, -1000000);
	EXPECT_EQ(keys[1].name, "sparkle.engineview.skybox");
	EXPECT_EQ(keys[1].order, -1);
	EXPECT_EQ(keys[2].name, "sparkle.engineview.opaque");
	EXPECT_EQ(keys[2].order, 0);
	EXPECT_EQ(keys[3].name, "sparkle.engineview.wboit");
	EXPECT_EQ(keys[3].order, 1000000);
	EXPECT_EQ(keys[4].name, "sparkle.engineview.composition");
	EXPECT_EQ(keys[4].order, 2000000);
}

TEST(EngineViewPassValidationTest, SystemPassContributionsExecuteInFiveStageOrder)
{
	Fixture fixture;
	fixture.addView("view");
	for (const auto &key : {spk::EngineView::CompositionPassKey, spk::EngineView::WBOITPassKey, spk::EngineView::OpaquePassKey, spk::EngineView::SkyboxPassKey, spk::EngineView::ShadowPassKey})
	{
		auto &probe = fixture.engine.root().addBehaviour<sparkle_test::engine_view::Probe>();
		probe.callback = [&fixture, key](spk::RenderSnapshot::Builder &builder, const spk::EngineView::CaptureContext &) {
			builder.renderPass(key).emplace<sparkle_test::engine_view::RecordCommand>(fixture.log, key.name);
		};
	}
	fixture.engine.refreshRenderSnapshot();
	fixture.executeLatest();
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"sparkle.engineview.shadow", "sparkle.engineview.skybox", "sparkle.engineview.opaque", "sparkle.engineview.wboit", "sparkle.engineview.composition"}));
}

TEST(EngineViewPassValidationTest, CustomPassBoundaryValuesAreInclusive)
{
	Fixture fixture;
	fixture.addView("view");
	const std::array<spk::RenderPass::Key, 8> keys{{{"a", std::numeric_limits<std::int32_t>::min()}, {"b", -1000001}, {"c", -999999}, {"d", -2}, {"e", 1}, {"f", 999999}, {"g", 1000001}, {"h", 1999999}}};
	auto &probe = fixture.engine.root().addBehaviour<sparkle_test::engine_view::Probe>();
	probe.callback = [&fixture, keys](spk::RenderSnapshot::Builder &builder, const spk::EngineView::CaptureContext &) {
		for (const auto &key : keys)
		{
			builder.renderPass(key).emplace<sparkle_test::engine_view::RecordCommand>(fixture.log, key.name);
		}
	};
	fixture.engine.refreshRenderSnapshot();
	fixture.executeLatest();
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"a", "b", "c", "d", "e", "f", "g", "h"}));
}

TEST(EngineViewPassValidationTest, EmptyPassNameIsRejected)
{
	expectInvalid({"", 5});
}

TEST(EngineViewPassValidationTest, ReservedPrefixIsRejected)
{
	expectInvalid({"sparkle.rogue", 5});
}

TEST(EngineViewPassValidationTest, ReservedPrefixRejectsSystemLikeName)
{
	expectInvalid({"sparkle.engineview.custom", -5});
}

TEST(EngineViewPassValidationTest, ShadowOrderCannotBeStolen)
{
	expectInvalid({"custom.shadow", -1000000});
}

TEST(EngineViewPassValidationTest, SkyboxOrderCannotBeStolen)
{
	expectInvalid({"custom.skybox", -1});
}

TEST(EngineViewPassValidationTest, OpaqueOrderCannotBeStolen)
{
	expectInvalid({"custom.opaque", 0});
}

TEST(EngineViewPassValidationTest, WBOITOrderCannotBeStolen)
{
	expectInvalid({"custom.wboit", 1000000});
}

TEST(EngineViewPassValidationTest, CompositionOrderCannotBeStolen)
{
	expectInvalid({"custom.composition", 2000000});
}

TEST(EngineViewPassValidationTest, SystemNameWithWrongOrderIsRejected)
{
	expectInvalid({"sparkle.engineview.opaque", 2});
}

TEST(EngineViewPassValidationTest, NothingCanBeAddedAfterComposition)
{
	expectInvalid({"custom.after", 2000001});
	expectInvalid({"custom.max", std::numeric_limits<std::int32_t>::max()});
}

TEST(EngineViewPassValidationTest, SameOrderWithDifferentNamesIsRejected)
{
	Fixture fixture;
	fixture.addView("view");
	auto &probe = fixture.engine.root().addBehaviour<sparkle_test::engine_view::Probe>();
	probe.callback = [](spk::RenderSnapshot::Builder &builder,
						const spk::EngineView::CaptureContext &) {
		builder.renderPass({"game.one", 100});
		builder.renderPass({"game.two", 100});
	};
	EXPECT_THROW(fixture.engine.refreshRenderSnapshot(), spk::Exception);
}

TEST(EngineViewPassValidationTest, SameNameWithDifferentOrdersIsRejected)
{
	Fixture fixture;
	fixture.addView("view");
	auto &probe = fixture.engine.root().addBehaviour<sparkle_test::engine_view::Probe>();
	probe.callback = [](spk::RenderSnapshot::Builder &builder,
						const spk::EngineView::CaptureContext &) {
		builder.renderPass({"game.shared", 90});
		builder.renderPass({"game.shared", 91});
	};
	EXPECT_THROW(fixture.engine.refreshRenderSnapshot(), spk::Exception);
}

TEST(EngineViewPassValidationTest, SameKeyFromDifferentContributorsMergesCommandsInInsertionOrder)
{
	Fixture fixture;
	fixture.addView("view");
	fixture.record("first", {"custom.shared", 100});
	fixture.record("second", {"custom.shared", 100});
	fixture.engine.refreshRenderSnapshot();
	fixture.executeLatest();
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"view:first", "view:second"}));
}

TEST(EngineViewPassValidationTest, TwoViewsMayIndependentlyUseSameCustomKey)
{
	Fixture fixture;
	fixture.addView("a");
	fixture.addView("b");
	fixture.record("same", {"custom.shared", 100});
	fixture.engine.refreshRenderSnapshot();
	fixture.executeLatest();
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"a:same", "b:same"}));
}

TEST(EngineViewPassValidationTest, GenericBuilderStillAllowsTwoNamesAtSameOrder)
{
	spk::RenderSnapshot::Builder builder;
	EXPECT_NO_THROW(builder.renderPass({"one", 100}));
	EXPECT_NO_THROW(builder.renderPass({"two", 100}));
	EXPECT_EQ(builder.passKeys().size(), 2u);
	EXPECT_NO_THROW(static_cast<void>(builder.build()));
}

TEST(EngineViewPassValidationTest, PendingKeyInspectionDoesNotModifyBuilder)
{
	spk::RenderSnapshot::Builder builder;
	builder.renderPass({"one", 5});
	auto keys = builder.passKeys();
	keys[0].name = "changed";
	EXPECT_EQ(builder.passKeys()[0].name, "one");
}
