#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <future>
#include <limits>
#include <stdexcept>
#include <vector>

#include "voxel/occlusion/cache.hpp"

namespace
{
	using namespace spk::Voxel;
	using namespace spk::Voxel::Occlusion;
	using V = Shape::Vertex;

	std::vector<V> face(float x, float low, float high, bool positive)
	{
		std::vector<V> vertices{
			{{x, low, low}, {0, 0}, spk::Vector3{positive ? 1.0f : -1.0f, 0, 0}},
			{{x, high, low}, {1, 0}, spk::Vector3{positive ? 1.0f : -1.0f, 0, 0}},
			{{x, high, high}, {1, 1}, spk::Vector3{positive ? 1.0f : -1.0f, 0, 0}},
			{{x, low, high}, {0, 1}, spk::Vector3{positive ? 1.0f : -1.0f, 0, 0}}};
		if (!positive)
		{
			std::reverse(vertices.begin(), vertices.end());
		}
		return vertices;
	}

	Definition shape(float x, float low = 0, float high = 1, float opacity = 1, bool positive = true)
	{
		Shape::Builder builder;
		builder.addSlot("surface");
		builder.addPolygon("surface", face(x, low, high, positive));
		return Definition(builder.build(), {{"surface", {"surface", opacity}}});
	}

	Cache::Key key(const Definition &a, const Definition &b)
	{
		Cache::Key result;
		result.source = a;
		result.blocker = b;
		result.direction = Shape::Face::PositiveX;
		return result;
	}

	float surface(const Result &result)
	{
		float sum = 0;
		for (const auto &polygon : result.fragments())
		{
			const auto &vertices = polygon.vertices;
			for (std::size_t i = 1; i + 1 < vertices.size(); ++i)
			{
				auto a = vertices[i].position - vertices[0].position;
				auto b = vertices[i + 1].position - vertices[0].position;
				sum += static_cast<float>(a.cross(b).length()) * 0.5f;
			}
		}
		return sum;
	}
}

TEST(VoxelOcclusion, CompleteCoverage)
{
	auto a = shape(1);
	auto b = shape(0, 0, 1, 1, false);
	EXPECT_TRUE(Algorithm::compute(a, {}, b, {}, Shape::Face::PositiveX).empty());
}

TEST(VoxelOcclusion, CompleteCoverageWithOppositeTriangleDiagonals)
{
	auto source = shape(1);
	Shape::Builder builder;
	builder.addSlot("surface");
	builder.addPolygon("surface", {
		{{0, 0, 0}},
		{{0, 0, 1}},
		{{0, 1, 1}},
		{{0, 1, 0}}});
	const Definition blocker(builder.build(), {{"surface", {"surface"}}});
	const auto result = Algorithm::compute(source, {}, blocker, {}, Shape::Face::PositiveX);
	EXPECT_TRUE(result.empty());
}

TEST(VoxelOcclusion, NoCoverage)
{
	auto a = shape(1);
	auto b = shape(0, 0, 0.25f, 0, false);
	auto result = Algorithm::compute(a, {}, b, {}, Shape::Face::PositiveX);
	EXPECT_NEAR(surface(result), 1.0f, 1e-4f);
}

TEST(VoxelOcclusion, PartialCoverageAndUV)
{
	auto a = shape(1);
	auto b = shape(0, 0, 0.5f, 1, false);
	auto result = Algorithm::compute(a, {}, b, {}, Shape::Face::PositiveX);
	EXPECT_NEAR(surface(result), 0.75f, 1e-4f);
	for (const auto &polygon : result.fragments())
	{
		for (const auto &vertex : polygon.vertices)
		{
			EXPECT_NEAR(vertex.position.x, 1.0f, 1e-5f);
			EXPECT_GE(vertex.uv.x, -1e-4f);
			EXPECT_LE(vertex.uv.x, 1.0001f);
			EXPECT_GE(vertex.uv.y, -1e-4f);
			EXPECT_LE(vertex.uv.y, 1.0001f);
			ASSERT_TRUE(vertex.normal.has_value());
			EXPECT_NEAR(vertex.normal->length(), 1.0, 1e-4);
		}
	}
}

TEST(VoxelOcclusion, WrongFacingBlockerDoesNotOcclude)
{
	auto a = shape(1);
	auto b = shape(0);
	EXPECT_NEAR(surface(Algorithm::compute(a, {}, b, {}, Shape::Face::PositiveX)), 1.0f, 1e-4f);
}

TEST(VoxelOcclusion, CacheGetAndClear)
{
	auto a = shape(1);
	auto b = shape(0, 0, 0.5f, 1, false);
	Cache cache;
	auto k = key(a, b);
	auto first = cache.get(k);
	auto second = cache.get(k);
	EXPECT_EQ(&first.fragments(), &second.fragments());
	EXPECT_NEAR(surface(first), 0.75f, 1e-4f);
	cache.clear();
	auto third = cache.get(k);
	EXPECT_NE(&first.fragments(), &third.fragments());
	EXPECT_NEAR(surface(third), surface(first), 1e-5f);
}

TEST(VoxelOcclusion, CacheKeyDirectionAndTolerance)
{
	auto a = shape(1);
	auto b = shape(0, 0, 0.5f, 1, false);
	auto k = key(a, b);
	auto different = k;
	different.direction = Shape::Face::NegativeX;
	EXPECT_FALSE(k == different);
	different = k;
	different.tolerance = 1e-4f;
	EXPECT_FALSE(k == different);
	different = k;
	different.sourceOrientation.horizontal = Shape::HorizontalOrientation::XNegative;
	EXPECT_FALSE(k == different);
}

TEST(VoxelOcclusion, ConcurrentCacheGet)
{
	auto a = shape(1);
	auto b = shape(0, 0, 0.5f, 1, false);
	Cache cache;
	auto k = key(a, b);
	std::vector<std::future<Result>> jobs;
	auto task = [&] {
		return cache.get(k);
	};
	for (int i = 0; i < 16; ++i)
	{
		jobs.push_back(std::async(std::launch::async, task));
	}
	for (auto &job : jobs)
	{
		EXPECT_NEAR(surface(job.get()), 0.75f, 1e-4f);
	}
}

TEST(VoxelOcclusion, RejectInvalidTolerance)
{
	auto a = shape(1);
	auto b = shape(0, 0, 1, 1, false);
	EXPECT_ANY_THROW(Algorithm::compute(a, {}, b, {}, Shape::Face::PositiveX, 0));
	EXPECT_ANY_THROW(Algorithm::compute(a, {}, b, {}, Shape::Face::PositiveX, -1));
}

TEST(VoxelOcclusion, RepeatedClearAndGet)
{
	auto a = shape(1);
	auto b = shape(0, 0, 0.5f, 1, false);
	Cache cache;
	auto k = key(a, b);
	for (int i = 0; i < 100; ++i)
	{
		EXPECT_NEAR(surface(cache.get(k)), 0.75f, 1e-4f);
		cache.clear();
	}
}

TEST(VoxelOcclusion, CacheResultSurvivesClear)
{
	auto a = shape(1);
	auto b = shape(0, 0, 0.5f, 1, false);
	Cache cache;
	auto saved = cache.get(key(a, b));
	cache.clear();
	EXPECT_NEAR(surface(saved), 0.75f, 1e-4f);
}

TEST(VoxelOcclusion, ConcurrentGetAndClear)
{
	auto a = shape(1);
	auto b = shape(0, 0, 0.5f, 1, false);
	Cache cache;
	auto k = key(a, b);
	std::vector<std::future<void>> jobs;
	for (int worker = 0; worker < 8; ++worker)
	{
		auto task = [&, worker] {
			for (int i = 0; i < 100; ++i)
			{
				if (worker == 0)
				{
					cache.clear();
				}
				else
				{
					auto result = cache.get(k);
					if (std::abs(surface(result) - 0.75f) > 1e-4f)
					{
						throw std::runtime_error("Incorrect concurrent occlusion result");
					}
				}
			}
		};
		jobs.push_back(std::async(std::launch::async, task));
	}
	for (auto &job : jobs)
	{
		EXPECT_NO_THROW(job.get());
	}
}

TEST(VoxelOcclusion, AllEightOrientationsAreDeterministic)
{
	auto a = shape(1);
	auto b = shape(0, 0, 0.5f, 1, false);
	Cache cache;
	for (auto horizontal : {Shape::HorizontalOrientation::XPositive, Shape::HorizontalOrientation::ZPositive, Shape::HorizontalOrientation::XNegative, Shape::HorizontalOrientation::ZNegative})
	{
		for (auto vertical : {Shape::VerticalOrientation::YPositive, Shape::VerticalOrientation::YNegative})
		{
			auto k = key(a, b);
			k.sourceOrientation = {horizontal, vertical};
			k.blockerOrientation = {horizontal, vertical};
			auto direct = Algorithm::compute(
				k.source,
				k.sourceOrientation,
				k.blocker,
				k.blockerOrientation,
				k.direction,
				k.tolerance);
			auto cached = cache.get(k);
			EXPECT_NEAR(surface(direct), surface(cached), 1e-5f);
			EXPECT_NEAR(surface(cached), surface(cache.get(k)), 1e-5f);
		}
	}
}

TEST(VoxelOcclusion, MultipleOverlappingBlockersDoNotIncreaseVisibleArea)
{
	Shape::Builder builder;
	builder.addSlot("surface");
	builder.addPolygon("surface", face(0, 0, 0.5f, false));
	builder.addPolygon("surface", face(0, 0, 0.5f, false));
	auto blocker = Definition(builder.build(), {{"surface", {"surface"}}});
	auto source = shape(1);
	auto result = Algorithm::compute(source, {}, blocker, {}, Shape::Face::PositiveX);
	EXPECT_NEAR(surface(result), 0.75f, 1e-4f);
}

TEST(VoxelOcclusion, TransparentBlockerIsIgnored)
{
	auto source = shape(1);
	auto blocker = shape(0, 0, 1, 0.5f, false);
	EXPECT_NEAR(surface(Algorithm::compute(source, {}, blocker, {}, Shape::Face::PositiveX)), 1.0f, 1e-4f);
}

TEST(VoxelOcclusion, EmptySourceProducesNoFragments)
{
	Definition empty;
	auto blocker = shape(0, 0, 1, 1, false);
	EXPECT_TRUE(Algorithm::compute(empty, {}, blocker, {}, Shape::Face::PositiveX).empty());
}

TEST(VoxelOcclusion, RejectNonFiniteTolerance)
{
	auto a = shape(1);
	auto b = shape(0, 0, 1, 1, false);
	EXPECT_ANY_THROW(Algorithm::compute(a, {}, b, {}, Shape::Face::PositiveX, std::numeric_limits<float>::infinity()));
	EXPECT_ANY_THROW(Algorithm::compute(a, {}, b, {}, Shape::Face::PositiveX, std::numeric_limits<float>::quiet_NaN()));
}
