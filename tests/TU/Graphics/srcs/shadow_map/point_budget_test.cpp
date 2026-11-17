#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

#include "core/context/render_context.hpp"
#include "shadow_map/point_budget.hpp"
#include "shadow_map/rendering_system.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	spk::ShadowMap::PointBudget::Request request(
		const void *system,
		std::uint64_t view,
		std::uint64_t light,
		float score,
		std::size_t &budget,
		bool &dirty)
	{
		return {
			.system = system,
			.viewIdentity = view,
			.registrationOrder = light,
			.admission = 1,
			.score = score,
			.limit = [&budget] { return budget; },
			.needsRebuild = [&dirty](spk::RenderContext &) { return dirty; }};
	}
}

TEST(PointShadowBudget, DefaultsAndExplicitZero)
{
	spk::ShadowMap::RenderingSystem system;
	EXPECT_EQ(system.maxPointShadowRebuildsPerImage(), 1u);
	system.setMaxPointShadowRebuildsPerImage(0);
	EXPECT_EQ(system.maxPointShadowRebuildsPerImage(), 0u);
	system.setMaxPointShadowRebuildsPerImage(3);
	EXPECT_EQ(system.maxPointShadowRebuildsPerImage(), 3u);
}

TEST(PointShadowBudget, LateHigherScoreWinsAndDoesNotReorderViews)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	spk::RenderContext context(gl.surface());
	spk::ShadowMap::PointBudget plan;
	int system = 0;
	std::size_t limit = 1;
	bool dirty = true;
	plan.add(request(&system, 1, 1, 10.0f, limit, dirty));
	plan.add(request(&system, 2, 2, 20.0f, limit, dirty));
	plan.prepare(context);
	EXPECT_FALSE(plan.consume(context, &system, 1, 1, 1));
	EXPECT_TRUE(plan.consume(context, &system, 2, 2, 1));
	EXPECT_FALSE(plan.consume(context, &system, 2, 2, 1));
	plan.prepare(context);
	EXPECT_FALSE(plan.consume(context, &system, 1, 1, 1));
}

TEST(PointShadowBudget, AlreadyValidPairsDoNotConsumeBudget)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	spk::RenderContext context(gl.surface());
	spk::ShadowMap::PointBudget plan;
	int system = 0;
	std::size_t limit = 1;
	bool valid = false;
	bool dirty = true;
	plan.add(request(&system, 1, 1, 100.0f, limit, valid));
	plan.add(request(&system, 2, 2, 1.0f, limit, dirty));
	plan.prepare(context);
	EXPECT_FALSE(plan.consume(context, &system, 1, 1, 1));
	EXPECT_TRUE(plan.consume(context, &system, 2, 2, 1));
}

TEST(PointShadowBudget, ZeroDefersRebuildButNextImageReevaluates)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	int system = 0;
	std::size_t limit = 0;
	bool dirty = true;
	spk::ShadowMap::PointBudget plan;
	plan.add(request(&system, 1, 1, 10.0f, limit, dirty));
	{
		spk::RenderContext first(gl.surface());
		plan.prepare(first);
		EXPECT_FALSE(plan.consume(first, &system, 1, 1, 1));
	}
	limit = 1;
	{
		spk::RenderContext second(gl.surface());
		plan.prepare(second);
		EXPECT_TRUE(plan.consume(second, &system, 1, 1, 1));
	}
	dirty = false;
	{
		spk::RenderContext third(gl.surface());
		plan.prepare(third);
		EXPECT_FALSE(plan.consume(third, &system, 1, 1, 1));
	}
}

TEST(PointShadowBudget, DeterministicTieAndDeduplication)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	spk::RenderContext context(gl.surface());
	int system = 0;
	std::size_t limit = 2;
	bool dirty = true;
	spk::ShadowMap::PointBudget plan;
	plan.add(request(&system, 2, 9, 10.0f, limit, dirty));
	plan.add(request(&system, 1, 8, 10.0f, limit, dirty));
	plan.add(request(&system, 1, 7, 10.0f, limit, dirty));
	plan.add(request(&system, 1, 7, 10.0f, limit, dirty));
	plan.prepare(context);
	EXPECT_TRUE(plan.consume(context, &system, 1, 7, 1));
	EXPECT_TRUE(plan.consume(context, &system, 1, 8, 1));
	EXPECT_FALSE(plan.consume(context, &system, 2, 9, 1));
	EXPECT_FALSE(plan.consume(context, &system, 1, 7, 1));
}

TEST(PointShadowBudget, IndependentSystemsHaveIndependentQuotas)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	spk::RenderContext context(gl.surface());
	int firstSystem = 0;
	int secondSystem = 0;
	std::size_t limit = 1;
	bool dirty = true;
	spk::ShadowMap::PointBudget plan;
	plan.add(request(&firstSystem, 1, 1, 1.0f, limit, dirty));
	plan.add(request(&secondSystem, 1, 2, 1.0f, limit, dirty));
	plan.prepare(context);
	EXPECT_TRUE(plan.consume(context, &firstSystem, 1, 1, 1));
	EXPECT_TRUE(plan.consume(context, &secondSystem, 1, 2, 1));
}
