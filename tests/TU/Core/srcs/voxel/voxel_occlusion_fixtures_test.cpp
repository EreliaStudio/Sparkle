#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "voxel/occlusion/cache.hpp"

namespace
{
	using namespace spk::Voxel;
	using namespace spk::Voxel::Occlusion;
	using Face = Shape::Face;
	using Horizontal = Shape::HorizontalOrientation;
	using Vertical = Shape::VerticalOrientation;

	constexpr std::array<const char *, 4> Names = {"cube", "slab", "slope", "stair"};
	constexpr std::array<Shape::Orientation, 8> Orientations = {{
		{Horizontal::XPositive, Vertical::YPositive},
		{Horizontal::ZPositive, Vertical::YPositive},
		{Horizontal::XNegative, Vertical::YPositive},
		{Horizontal::ZNegative, Vertical::YPositive},
		{Horizontal::XPositive, Vertical::YNegative},
		{Horizontal::ZPositive, Vertical::YNegative},
		{Horizontal::XNegative, Vertical::YNegative},
		{Horizontal::ZNegative, Vertical::YNegative}}};
	constexpr std::array<Face, 6> Directions = {
		Face::NegativeX, Face::PositiveX,
		Face::NegativeY, Face::PositiveY,
		Face::NegativeZ, Face::PositiveZ};
	constexpr std::size_t Resolution = 64;
	constexpr std::size_t Samples = Resolution * Resolution;

	struct Fixture
	{
		std::string name;
		Definition opaque;
		Definition translucent;
	};

	struct Coverage
	{
		std::array<std::vector<std::uint8_t>, 6> source;
		std::array<std::vector<std::uint8_t>, 6> blocker;
	};

	Face opposite(Face face)
	{
		return static_cast<Face>(static_cast<unsigned>(face) ^ 1U);
	}

	spk::Vector3 offset(Face face)
	{
		switch (face)
		{
		case Face::NegativeX:
			return {-1, 0, 0};
		case Face::PositiveX:
			return {1, 0, 0};
		case Face::NegativeY:
			return {0, -1, 0};
		case Face::PositiveY:
			return {0, 1, 0};
		case Face::NegativeZ:
			return {0, 0, -1};
		case Face::PositiveZ:
			return {0, 0, 1};
		}
		return {};
	}

	spk::Vector2 project(const spk::Vector3 &point, Face face)
	{
		if (face == Face::NegativeX || face == Face::PositiveX)
		{
			return {point.y, point.z};
		}
		if (face == Face::NegativeY || face == Face::PositiveY)
		{
			return {point.x, point.z};
		}
		return {point.x, point.y};
	}

	bool contains(const Shape::Polygon &polygon, Face face, spk::Vector2 point)
	{
		bool positive = false;
		bool negative = false;
		for (std::size_t i = 0; i < polygon.vertices.size(); ++i)
		{
			const auto a = project(polygon.vertices[i].position, face);
			const auto b = project(polygon.vertices[(i + 1) % polygon.vertices.size()].position, face);
			const float side = (b.x - a.x) * (point.y - a.y) - (b.y - a.y) * (point.x - a.x);
			positive |= side > 1.0e-6f;
			negative |= side < -1.0e-6f;
			if (positive && negative)
			{
				return false;
			}
		}
		return true;
	}

	void paint(std::vector<std::uint8_t> &mask, const Shape::Polygon &polygon, Face face, bool unionOnly)
	{
		for (std::size_t y = 0; y < Resolution; ++y)
		{
			for (std::size_t x = 0; x < Resolution; ++x)
			{
				const spk::Vector2 point{
					(static_cast<float>(x) + 0.37f) / static_cast<float>(Resolution),
					(static_cast<float>(y) + 0.63f) / static_cast<float>(Resolution)};
				const std::size_t index = y * Resolution + x;
				if (contains(polygon, face, point))
				{
					mask[index] = unionOnly ? 1 : static_cast<std::uint8_t>(mask[index] + 1);
				}
			}
		}
	}

	Coverage coverage(const Shape &shape, Shape::Orientation orientation)
	{
		Coverage result;
		for (std::size_t direction = 0; direction < Directions.size(); ++direction)
		{
			result.source[direction].resize(Samples);
			result.blocker[direction].resize(Samples);
		}
		for (const auto &polygon : shape.instance(orientation).polygons())
		{
			if (polygon.shell != Shape::Shell::Outer || !polygon.boundary)
			{
				continue;
			}
			const Face face = *polygon.boundary;
			paint(result.source[static_cast<std::size_t>(face)], polygon, face, false);
			const Face direction = opposite(face);
			if (polygon.geometricNormal.dot(offset(direction)) < 0)
			{
				paint(result.blocker[static_cast<std::size_t>(direction)], polygon, face, true);
			}
		}
		return result;
	}

	std::array<Fixture, 4> fixtures()
	{
		std::array<Fixture, 4> result;
		for (std::size_t i = 0; i < Names.size(); ++i)
		{
			Shape::Builder builder;
			const auto path = std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / (std::string(Names[i]) + ".json");
			builder.addFromJSON(spk::JSON::Value::loadFromFile(path));
			const Shape shape = builder.build();
			Definition::SlotMapping opaque;
			Definition::SlotMapping translucent;
			for (const auto &slot : shape.slots())
			{
				opaque.emplace(slot.name, Definition::Slot{slot.name});
				translucent.emplace(slot.name, Definition::Slot{slot.name, 0.5f});
			}
			result[i] = Fixture{Names[i], Definition(shape, std::move(opaque)), Definition(shape, std::move(translucent))};
		}
		return result;
	}

	float expectedVisibleArea(const Coverage &source, const Coverage &blocker, Face direction)
	{
		const auto index = static_cast<std::size_t>(direction);
		float exposedSamples = 0;
		for (std::size_t i = 0; i < Samples; ++i)
		{
			if (!blocker.blocker[index][i])
			{
				exposedSamples += source.source[index][i];
			}
		}
		return exposedSamples / static_cast<float>(Samples);
	}

	float area(const std::vector<Shape::Polygon> &polygons)
	{
		float result = 0;
		for (const auto &polygon : polygons)
		{
			for (std::size_t i = 1; i + 1 < polygon.vertices.size(); ++i)
			{
				const auto a = polygon.vertices[i].position - polygon.vertices[0].position;
				const auto b = polygon.vertices[i + 1].position - polygon.vertices[0].position;
				result += static_cast<float>(a.cross(b).length()) * 0.5f;
			}
		}
		return result;
	}

	float boundaryArea(const Shape &shape, Shape::Orientation orientation, Face direction)
	{
		std::vector<Shape::Polygon> polygons;
		for (const auto &polygon : shape.instance(orientation).polygons())
		{
			if (polygon.shell == Shape::Shell::Outer && polygon.boundary == direction)
			{
				polygons.push_back(polygon);
			}
		}
		return area(polygons);
	}

	void checkFragments(const Result &result, Face direction)
	{
		const std::size_t axis = static_cast<std::size_t>(direction) / 2;
		const float plane = static_cast<unsigned>(direction) % 2 == 0 ? 0.0f : 1.0f;
		for (const auto &polygon : result.fragments())
		{
			EXPECT_EQ(polygon.shell, Shape::Shell::Outer);
			EXPECT_EQ(polygon.boundary, direction);
			for (const auto &vertex : polygon.vertices)
			{
				EXPECT_NEAR(vertex.position.data[axis], plane, 1.0e-5f);
				for (float coordinate : vertex.position.data)
				{
					EXPECT_GE(coordinate, -1.0e-5f);
					EXPECT_LE(coordinate, 1.00001f);
				}
				EXPECT_TRUE(std::isfinite(vertex.uv.x));
				EXPECT_TRUE(std::isfinite(vertex.uv.y));
			}
		}
	}

	Cache::Key makeKey(
		const Definition &source,
		const Definition &blocker,
		Shape::Orientation sourceOrientation,
		Shape::Orientation blockerOrientation,
		Face direction)
	{
		Cache::Key key;
		key.source = source;
		key.blocker = blocker;
		key.sourceOrientation = sourceOrientation;
		key.blockerOrientation = blockerOrientation;
		key.direction = direction;
		return key;
	}
}

TEST(VoxelOcclusionFixtures, EveryOrderedPairAndOrientationMatchesIndependentCoverage)
{
	const auto shapes = fixtures();
	std::array<std::array<Coverage, 8>, 4> masks;
	for (std::size_t shape = 0; shape < shapes.size(); ++shape)
	{
		for (std::size_t orientation = 0; orientation < Orientations.size(); ++orientation)
		{
			masks[shape][orientation] = coverage(shapes[shape].opaque.shape(), Orientations[orientation]);
		}
	}

	Cache cache;
	std::size_t cases = 0;
	for (std::size_t source = 0; source < shapes.size(); ++source)
	{
		for (std::size_t blocker = 0; blocker < shapes.size(); ++blocker)
		{
			for (std::size_t sourceOrientation = 0; sourceOrientation < Orientations.size(); ++sourceOrientation)
			{
				for (std::size_t blockerOrientation = 0; blockerOrientation < Orientations.size(); ++blockerOrientation)
				{
					for (Face direction : Directions)
					{
						SCOPED_TRACE(::testing::Message() << shapes[source].name << "[" << sourceOrientation << "] -> "
														  << shapes[blocker].name << "[" << blockerOrientation << "], face " << static_cast<int>(direction));
						const auto key = makeKey(
							shapes[source].opaque,
							shapes[blocker].opaque,
							Orientations[sourceOrientation],
							Orientations[blockerOrientation],
							direction);
						const Result direct = Algorithm::compute(key.source, key.sourceOrientation, key.blocker, key.blockerOrientation, direction);
						const Result cached = cache.get(key);
						const float visible = area(direct.fragments());
						const float expected = expectedVisibleArea(masks[source][sourceOrientation], masks[blocker][blockerOrientation], direction);
						EXPECT_NEAR(visible, expected, 0.02f);
						EXPECT_NEAR(area(cached.fragments()), visible, 1.0e-5f);
						EXPECT_EQ(cached.fragments().size(), direct.fragments().size());
						checkFragments(direct, direction);
						if (shapes[blocker].name == "cube")
						{
							EXPECT_NEAR(visible, 0.0f, 1.0e-5f);
						}
						if (shapes[source].name == "cube")
						{
							const float blocked = boundaryArea(shapes[blocker].opaque.shape(), Orientations[blockerOrientation], opposite(direction));
							EXPECT_NEAR(visible, 1.0f - blocked, 1.0e-5f);
						}
						++cases;
					}
				}
			}
		}
	}
	EXPECT_EQ(cases, 4u * 4u * 8u * 8u * 6u);
}

TEST(VoxelOcclusionFixtures, EveryTranslucentBlockerLeavesSourceBoundaryVisible)
{
	const auto shapes = fixtures();
	std::size_t cases = 0;
	Cache cache;
	for (const auto &source : shapes)
	{
		for (const auto &blocker : shapes)
		{
			for (std::size_t sourceOrientation = 0; sourceOrientation < Orientations.size(); ++sourceOrientation)
			{
				for (std::size_t blockerOrientation = 0; blockerOrientation < Orientations.size(); ++blockerOrientation)
				{
					for (Face direction : Directions)
					{
						SCOPED_TRACE(::testing::Message() << source.name << "[" << sourceOrientation << "] -> translucent "
														  << blocker.name << "[" << blockerOrientation << "], face " << static_cast<int>(direction));
						const auto key = makeKey(
							source.opaque,
							blocker.translucent,
							Orientations[sourceOrientation],
							Orientations[blockerOrientation],
							direction);
						const auto result = cache.get(key);
						const float sourceArea = boundaryArea(source.opaque.shape(), key.sourceOrientation, direction);
						EXPECT_NEAR(area(result.fragments()), sourceArea, 1.0e-5f);
						++cases;
					}
				}
			}
		}
	}
	EXPECT_EQ(cases, 4u * 4u * 8u * 8u * 6u);
}
