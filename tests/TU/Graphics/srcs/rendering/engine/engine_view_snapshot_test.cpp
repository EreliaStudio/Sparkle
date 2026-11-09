#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "core/context/update_context.hpp"
#include "engine/transform3d.hpp"
#include "engine_view_test_utils.hpp"
#include "exception.hpp"

namespace
{
	using Fixture = sparkle_test::engine_view::Fixture;
	using Context = spk::EngineView::CaptureContext;

	void execute(const std::shared_ptr<const spk::RenderSnapshot> &snapshot)
	{
		spk::RenderContext context{.targetSurface = nullptr};
		snapshot->execute(context);
	}
}

TEST(EngineViewSnapshotTest, MultipleViewsShareContributorsButHaveIndependentContexts)
{
	Fixture fixture;
	fixture.addView("a");
	fixture.addView("b");
	auto &probe = fixture.record("shared");
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 2u);
	fixture.executeLatest();
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"a:shared", "b:shared"}));
}

TEST(EngineViewSnapshotTest, LogicalUpdateDoesNotCaptureOrPublish)
{
	Fixture fixture;
	fixture.addView("view");
	auto &probe = fixture.record("value");
	spk::UpdateContext update{};
	fixture.engine.updateState(update);
	EXPECT_EQ(probe.contributions, 0u);
	EXPECT_EQ(fixture.engine.latestRenderSnapshot(), nullptr);
}

TEST(EngineViewSnapshotTest, CapturesAreBuiltOnCPUWithoutGraphicsContext)
{
	Fixture fixture;
	fixture.addView("view");
	fixture.record("value");
	EXPECT_NO_THROW(fixture.engine.refreshRenderSnapshot());
	fixture.executeLatest();
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"view:value"}));
}

TEST(EngineViewSnapshotTest, MutationAfterCaptureDoesNotChangeOldCommands)
{
	Fixture fixture;
	fixture.addView("view");
	auto &probe = fixture.record("old");
	fixture.engine.refreshRenderSnapshot();
	auto historical = fixture.engine.latestRenderSnapshot();
	probe.callback = [&fixture](spk::RenderSnapshot::Builder &builder, const Context &ctx) {
		builder.renderPass(spk::EngineView::OpaquePassKey)
			.emplace<sparkle_test::engine_view::RecordCommand>(fixture.log, ctx.name + ":new");
	};
	fixture.engine.refreshRenderSnapshot();
	execute(historical);
	fixture.executeLatest();
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"view:old", "view:new"}));
}

TEST(EngineViewSnapshotTest, CameraMatricesAreCopiedWhenCapturing)
{
	Fixture fixture;
	fixture.addView("view");
	std::vector<float> capturedValues;
	auto &probe = fixture.engine.root().addBehaviour<sparkle_test::engine_view::Probe>();
	probe.callback = [&](spk::RenderSnapshot::Builder &, const Context &ctx) {
		capturedValues.push_back(ctx.viewMatrix[3][0]);
	};
	fixture.engine.refreshRenderSnapshot();
	const float before = capturedValues.back();
	fixture.cameraEntity.transform().move({4, 0, 0});
	fixture.engine.refreshRenderSnapshot();
	EXPECT_NE(before, capturedValues.back());
	EXPECT_NEAR(capturedValues.back(), -4.0f, 0.0001f);
}

TEST(EngineViewSnapshotTest, ViewParametersAreCapturedBeforeContributorChangesThem)
{
	Fixture fixture;
	auto &view = fixture.addView("view");
	view.setResolution({300, 200});
	view.setBackgroundColor({0.2f, 0.3f, 0.4f});
	view.setExposure(2.0f);
	view.setTargetFPS(75.0f);
	auto &probe = fixture.engine.root().addBehaviour<sparkle_test::engine_view::Probe>();
	probe.callback = [&](spk::RenderSnapshot::Builder &, const Context &ctx) {
		EXPECT_EQ(ctx.name, "view");
		EXPECT_EQ(ctx.identity, view.identity());
		EXPECT_EQ(ctx.resolution, spk::Vector2UInt(300, 200));
		EXPECT_EQ(ctx.backgroundColor, (spk::RGB{0.2f, 0.3f, 0.4f}));
		EXPECT_FLOAT_EQ(ctx.exposure, 2.0f);
		EXPECT_FLOAT_EQ(ctx.targetFPS, 75.0f);
		EXPECT_FLOAT_EQ(ctx.viewProjectionMatrix[0][0], ctx.projectionMatrix[0][0]);
		view.setExposure(3.0f);
	};
	fixture.engine.refreshRenderSnapshot();
	EXPECT_FLOAT_EQ(view.exposure(), 3.0f);
}

TEST(EngineViewSnapshotTest, FailedCaptureKeepsPreviouslyPublishedSnapshot)
{
	Fixture fixture;
	fixture.addView("view");
	auto &probe = fixture.record("stable");
	fixture.engine.refreshRenderSnapshot();
	auto stable = fixture.engine.latestRenderSnapshot();
	probe.callback = [](spk::RenderSnapshot::Builder &, const Context &) {
		throw spk::Exception("deliberate failure");
	};
	EXPECT_THROW(fixture.engine.refreshRenderSnapshot(), spk::Exception);
	EXPECT_EQ(fixture.engine.latestRenderSnapshot(), stable);
	execute(stable);
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"view:stable"}));
}

TEST(EngineViewSnapshotTest, DetachedCameraPreventsAnyNewViewContribution)
{
	Fixture fixture;
	auto &view = fixture.addView("view");
	auto &probe = fixture.record("previous");
	fixture.engine.refreshRenderSnapshot();
	auto previous = fixture.engine.latestRenderSnapshot();
	view.detachCamera();
	fixture.engine.refreshRenderSnapshot();
	auto detached = fixture.engine.latestRenderSnapshot();
	EXPECT_NE(detached, previous);
	EXPECT_EQ(probe.contributions, 1u);
	execute(detached);
	EXPECT_TRUE(fixture.log.empty());
	execute(previous);
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"view:previous"}));
}

TEST(EngineViewSnapshotTest, RemovingViewKeepsOldSnapshotUsable)
{
	Fixture fixture;
	fixture.addView("removed");
	fixture.record("visible");
	fixture.engine.refreshRenderSnapshot();
	auto previous = fixture.engine.latestRenderSnapshot();
	fixture.engine.removeView("removed");
	fixture.engine.refreshRenderSnapshot();
	fixture.executeLatest();
	EXPECT_TRUE(fixture.log.empty());
	execute(previous);
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"removed:visible"}));
}

TEST(EngineViewSnapshotTest, ViewRemovedDuringCaptureCannotReappearAtCommit)
{
	Fixture fixture;
	fixture.addView("removed");
	fixture.addView("survivor");
	bool removed = false;
	auto &probe = fixture.record("value");
	probe.callback = [&](spk::RenderSnapshot::Builder &builder, const Context &ctx) {
		if (!removed && ctx.name == "removed")
		{
			removed = true;
			fixture.engine.removeView("removed");
		}
		builder.renderPass(spk::EngineView::OpaquePassKey)
			.emplace<sparkle_test::engine_view::RecordCommand>(fixture.log, ctx.name);
	};
	fixture.engine.refreshRenderSnapshot();
	fixture.executeLatest();
	EXPECT_TRUE(removed);
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"survivor"}));
}

TEST(EngineViewSnapshotTest, HomonymousViewRecreationCannotRestoreOldCapture)
{
	Fixture fixture;
	auto &original = fixture.addView("same");
	const auto oldIdentity = original.identity();
	bool recreated = false;
	auto &probe = fixture.engine.root().addBehaviour<sparkle_test::engine_view::Probe>();
	probe.callback = [&](spk::RenderSnapshot::Builder &builder, const Context &ctx) {
		if (!recreated)
		{
			recreated = true;
			fixture.engine.removeView("same");
			fixture.addView("same");
		}
		builder.renderPass(spk::EngineView::OpaquePassKey)
			.emplace<sparkle_test::engine_view::RecordCommand>(fixture.log, std::to_string(ctx.identity));
	};
	fixture.engine.refreshRenderSnapshot();
	EXPECT_NE(fixture.engine.view("same").identity(), oldIdentity);
	fixture.executeLatest();
	EXPECT_EQ(fixture.log, (std::vector<std::string>{std::to_string(fixture.engine.view("same").identity())}));
}

TEST(EngineViewSnapshotTest, TwoConsumersHoldTheSameImmutableSnapshot)
{
	Fixture fixture;
	fixture.addView("view");
	fixture.record("value");
	fixture.engine.refreshRenderSnapshot();
	auto consumerOne = fixture.engine.latestRenderSnapshot();
	auto consumerTwo = fixture.engine.latestRenderSnapshot();
	EXPECT_EQ(consumerOne, consumerTwo);
	execute(consumerOne);
	execute(consumerTwo);
	EXPECT_EQ(fixture.log, (std::vector<std::string>{"view:value", "view:value"}));
}

TEST(EngineViewSnapshotTest, CameraReattachmentResumesSceneContributions)
{
	Fixture fixture;
	auto &view = fixture.addView("view");
	auto &probe = fixture.record("visible");
	view.detachCamera();
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 0u);
	view.setCamera(&fixture.camera);
	fixture.engine.refreshRenderSnapshot();
	EXPECT_EQ(probe.contributions, 1u);
}
