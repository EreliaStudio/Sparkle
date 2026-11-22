#include <GL/glew.h>
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <utility>

#include "container/json/object.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "graphics/opengl/texture.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/engine_widget.hpp"
#include "voxel/chunk_mesher.hpp"
#include "voxel/chunk_rendering_system.hpp"
#include "voxel/mesher.hpp"
#include "voxel/renderer.hpp"
#include "world/chunk_collection.hpp"

namespace
{
	using Chunk = spk::Voxel::Chunk<4, 4, 4>;
	using Collection = spk::World::ChunkCollection<Chunk>;
	using Coordinate = spk::World::ChunkCoordinate;
	using Volume = spk::Voxel::Volume;
	constexpr spk::Vector2UInt ImageSize{320, 240};

	[[nodiscard]] spk::Voxel::Shape shape(const char *name)
	{
		spk::Voxel::Shape::Builder builder;
		builder.addFromJSON(spk::JSON::Value::loadFromFile(
			std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / (std::string(name) + ".json")));
		return builder.build();
	}

	[[nodiscard]] spk::Voxel::Definition::Registry definitions()
	{
		spk::Voxel::Definition::Registry::Builder builder;
		const auto cube = shape("cube");
		builder.add(1, spk::Voxel::Definition(cube, {{"surface", {"stone", 1.0f}}}));
		builder.add(2, spk::Voxel::Definition(cube, {{"surface", {"glass", 0.45f}}}));
		builder.add(3, spk::Voxel::Definition(shape("slab"), {{"surface", {"stone", 1.0f}}, {"profile", {"stone", 1.0f}}}));
		builder.add(4, spk::Voxel::Definition(shape("slope"), {{"surface", {"stone", 1.0f}}, {"profile", {"stone", 1.0f}}}));
		return builder.build();
	}

	[[nodiscard]] Chunk chunkWithCell(
		const spk::Voxel::Definition::Registry &registry,
		spk::Vector3UInt position,
		std::uint32_t definition = 1)
	{
		Volume::Builder builder({4, 4, 4});
		builder.bind(registry);
		builder.set(position, definition);
		Chunk chunk;
		chunk.setVolume(builder.build());
		return chunk;
	}

	[[nodiscard]] Chunk sceneChunk(
		const spk::Voxel::Definition::Registry &registry,
		std::int32_t coordinate)
	{
		Volume::Builder builder({4, 4, 4});
		builder.bind(registry);
		for (std::uint32_t x = 0; x < 4; ++x)
		{
			for (std::uint32_t z = 0; z < 4; ++z)
			{
				builder.set({x, 0, z}, 1);
			}
		}
		builder.set({1, 1, 1}, 1);
		if (coordinate == 0)
		{
			builder.set({2, 1, 1}, 3);
			builder.set({3, 1, 2}, 4);
		}
		Chunk chunk;
		chunk.setVolume(builder.build());
		return chunk;
	}

	class Atlas final : public spk::Texture
	{
	public:
		Atlas()
		{
			std::array<std::uint8_t, 8 * 4 * 4> pixels{};
			for (std::size_t y = 0; y < 4; ++y)
			{
				for (std::size_t x = 0; x < 8; ++x)
				{
					const std::size_t i = (y * 8 + x) * 4;
					pixels[i + 0] = x < 4 ? 190 : 32;
					pixels[i + 1] = x < 4 ? 160 : 160;
					pixels[i + 2] = x < 4 ? 105 : 230;
					pixels[i + 3] = 255;
				}
			}
			setPixels(std::span<const std::uint8_t>(pixels), {8, 4}, Format::RGBA);
			validate();
		}
	};

	[[nodiscard]] spk::Palette palette(const Atlas &atlas)
	{
		spk::Voxel::Palette::Builder builder;
		spk::Material::Builder stone;
		stone.add("albedo", atlas, spk::Texture::Section::fromPixels({8, 4}, {0, 0}, {4, 4}));
		builder.setMaterial("stone", stone.build());
		spk::Material::Builder glass;
		glass.add("albedo", atlas, spk::Texture::Section::fromPixels({8, 4}, {4, 0}, {4, 4}));
		builder.setMaterial("glass", glass.build());
		return builder.build();
	}

	struct Scene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"chunk golden camera"};
		spk::CameraComponent3D &camera = cameraEntity.addComponent<spk::CameraComponent3D>();
		spk::EngineView &view;

		Scene() :
			view(createView())
		{
			view.ambientLight().setIntensity(1.0f);
		}

		spk::EngineView &createView()
		{
			const spk::Vector3 eye{12.5f, 10.0f, 16.0f};
			const spk::Vector3 target{1.8f, 0.8f, 1.5f};
			cameraEntity.transform().place(eye);
			cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, target));
			camera.setPerspective(0.88f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 70.0f);
			engine.addEntity(&cameraEntity);
			return engine.addView("main", &camera, ImageSize);
		}

		~Scene()
		{
			engine.removeEntity(&cameraEntity);
		}
	};

	void render(Scene &scene, const std::string &name)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		gl.reset(ImageSize);
		spk::EngineWidget widget("chunk scene", nullptr);
		widget.setEngine(&scene.engine);
		widget.setGeometry({.anchor = {0, 0}, .size = ImageSize});
		widget.activate();
		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		builder.build().execute(gl.renderContext());
		gl.requireNoErrors();
		gl.save(sparkle_test::resultImagePath("rendering/voxel-chunks", name));
		gl.reset();
	}

	[[nodiscard]] spk::Voxel::Geometry singleVolumeGeometry(
		const spk::Voxel::Definition::Registry &registry)
	{
		Volume::Builder builder({12, 4, 4});
		builder.bind(registry);
		for (std::int32_t coordinate = -1; coordinate <= 1; ++coordinate)
		{
			const Chunk source = sceneChunk(registry, coordinate);
			for (std::uint32_t z = 0; z < 4; ++z)
			{
				for (std::uint32_t y = 0; y < 4; ++y)
				{
					for (std::uint32_t x = 0; x < 4; ++x)
					{
						const auto id = Volume::PackedCell::definitionId(source.volume().atRaw({x, y, z}));
						if (id != 0)
						{
							builder.set({static_cast<std::uint32_t>((coordinate + 1) * 4) + x, y, z}, id);
						}
					}
				}
			}
		}
		return spk::Voxel::Mesher().bake(builder.build());
	}
}

TEST(VoxelChunkMesherTest, AllSixAdjacentChunksHideInternalOpaqueFaces)
{
	const auto registry = definitions();
	const std::array<Coordinate, 6> directions{Coordinate{-1, 0, 0}, Coordinate{1, 0, 0},
		Coordinate{0, -1, 0}, Coordinate{0, 1, 0}, Coordinate{0, 0, -1}, Coordinate{0, 0, 1}};
	const std::array<spk::Vector3UInt, 6> sourcePositions{
		spk::Vector3UInt{0, 1, 1}, {3, 1, 1}, {1, 0, 1}, {1, 3, 1}, {1, 1, 0}, {1, 1, 3}};
	const std::array<spk::Vector3UInt, 6> adjacentPositions{
		spk::Vector3UInt{3, 1, 1}, {0, 1, 1}, {1, 3, 1}, {1, 0, 1}, {1, 1, 3}, {1, 1, 0}};

	for (std::size_t index = 0; index < directions.size(); ++index)
	{
		Collection collection;
		collection.addChunk({0, 0, 0}, chunkWithCell(registry, sourcePositions[index]));
		spk::Voxel::ChunkMesher<4, 4, 4> mesher(collection, {0, 0, 0});
		const auto isolated = mesher.bake(collection.at({0, 0, 0}).volume());
		EXPECT_EQ(isolated.opaqueMesh().vertexCount(), 36u);

		collection.addChunk(directions[index], chunkWithCell(registry, adjacentPositions[index]));
		const auto joined = mesher.bake(collection.at({0, 0, 0}).volume());
		EXPECT_EQ(joined.opaqueMesh().vertexCount(), 30u) << index;
		EXPECT_EQ(joined.opaqueMesh().indexCount(), 30u) << index;

		(void)collection.removeChunk(directions[index]);
		const auto restored = mesher.bake(collection.at({0, 0, 0}).volume());
		EXPECT_EQ(restored.opaqueMesh().vertexCount(), 36u) << index;
	}
}

TEST(VoxelChunkMesherTest, GlassSeamsAndPartialShapesReuseExistingOcclusion)
{
	const auto registry = definitions();
	Collection collection;
	collection.addChunk({-1, 0, 0}, chunkWithCell(registry, {3, 1, 1}, 2));
	collection.addChunk({0, 0, 0}, chunkWithCell(registry, {0, 1, 1}, 2));
	spk::Voxel::ChunkMesher<4, 4, 4> mesher(collection, {-1, 0, 0});
	const auto joinedGlass = mesher.bake(collection.at({-1, 0, 0}).volume());
	EXPECT_EQ(joinedGlass.transparentMesh().vertexCount(), 30u);
	EXPECT_EQ(joinedGlass.opaqueMesh().vertexCount(), 0u);

	collection.addChunk({0, 0, 0}, chunkWithCell(registry, {0, 1, 1}, 3));
	const auto slab = mesher.bake(collection.at({-1, 0, 0}).volume());
	EXPECT_GT(slab.transparentMesh().vertexCount(), 0u);

	collection.addChunk({0, 0, 0}, chunkWithCell(registry, {0, 1, 1}, 4));
	const auto slope = mesher.bake(collection.at({-1, 0, 0}).volume());
	EXPECT_GT(slope.transparentMesh().vertexCount(), 0u);
}

TEST(VoxelChunkRenderingTest, RebuildsEditedChunkAndNeighborsOnce)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	const auto registry = definitions();
	Atlas atlas;
	Scene scene;
	Collection collection;
	collection.attach(scene.engine);
	auto &system = scene.engine.addSystem<spk::Voxel::ChunkRenderingSystem<4, 4, 4>>();
	system.bind(collection, palette(atlas));

	collection.addChunk({-1, 0, 0}, chunkWithCell(registry, {3, 1, 1}));
	collection.addChunk({0, 0, 0}, chunkWithCell(registry, {0, 1, 1}));
	system.synchronize();
	EXPECT_EQ(system.lastStatistics().chunks, 2u);
	EXPECT_EQ(system.lastStatistics().opaqueVertices, 60u);
	EXPECT_EQ(system.lastStatistics().voxels, 128u);

	const auto initial = system.lastStatistics().meshingTime;
	for (int count = 0; count < 6; ++count)
	{
		collection.at({0, 0, 0}).setVolume(chunkWithCell(registry, {0, 1, 1}).volume());
	}
	system.synchronize();
	EXPECT_EQ(system.lastStatistics().chunks, 2u);
	EXPECT_EQ(system.lastStatistics().opaqueVertices, 60u);
	EXPECT_GT(system.lastStatistics().meshingTime.count(), 0);
	EXPECT_GT(initial.count(), 0);

	(void)collection.removeChunk({0, 0, 0});
	system.synchronize();
	EXPECT_EQ(system.lastStatistics().chunks, 1u);
	EXPECT_EQ(system.lastStatistics().opaqueVertices, 36u);
	system.unbind();
	EXPECT_EQ(collection.begin()->second.entity().getComponent<spk::Voxel::Renderer>(), nullptr);
	scene.engine.removeSystem(system);
	collection.detach();
	gl.reset();
}

TEST(VoxelChunkGoldenTest, ThreeAdjacentChunksMatchSingleVolumeAtObliqueCamera)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	const auto registry = definitions();
	Atlas atlas;
	const auto materials = palette(atlas);

	{
		Scene reference;
		spk::Entity3D entity("monolithic reference");
		entity.transform().place({-4.0f, 0.0f, 0.0f});
		auto &renderer = entity.addComponent<spk::Voxel::Renderer>();
		renderer.bind(singleVolumeGeometry(registry));
		renderer.bind(materials);
		reference.engine.addEntity(&entity);
		auto &system = reference.engine.addSystem<spk::Voxel::RenderingSystem>();
		render(reference, "three_chunks_reference");
		reference.engine.removeSystem(system);
		reference.engine.removeEntity(&entity);
	}

	{
		Scene chunked;
		Collection chunks;
		chunks.attach(chunked.engine);
		auto &system = chunked.engine.addSystem<spk::Voxel::ChunkRenderingSystem<4, 4, 4>>();
		system.bind(chunks, materials);
		for (std::int32_t x = -1; x <= 1; ++x)
		{
			chunks.addChunk({x, 0, 0}, sceneChunk(registry, x));
		}
		render(chunked, "three_chunks_oblique");
		EXPECT_EQ(system.lastStatistics().chunks, 3u);
		EXPECT_EQ(system.lastStatistics().voxels, 192u);
		EXPECT_GT(system.lastStatistics().opaqueTriangles, 0u);
		system.unbind();
		chunked.engine.removeSystem(system);
		chunks.detach();
	}

	const std::filesystem::path category{"rendering/voxel-chunks"};
	const auto actual = sparkle_test::resultImagePath(category, "three_chunks_oblique");
	const auto reference = sparkle_test::resultImagePath(category, "three_chunks_reference");
	const auto diff = sparkle_test::resultImagePath(category, "three_chunks_difference");
	const auto comparison = sparkle_test::compareImages(actual, reference, diff);
	EXPECT_TRUE(comparison.matches) << comparison.differentPixelCount << " pixels differ between chunked and monolithic rendering";
	gl.reset();
}

TEST(VoxelChunkPerformanceTest, ReportsVertexCountsAndMeshingTimePerChunkAndVoxel)
{
	const auto registry = definitions();
	const auto output = sparkle_test::freshResultFilePath("metrics", "chunk_meshing.csv");
	std::ofstream report(output);
	report << "size,pattern,voxels,vertices,triangles,mean_ns_per_chunk,mean_ns_per_voxel\n";
	const auto benchmark = [&]<std::int32_t S>(const char *pattern, bool sparse)
	{
		using BenchChunk = spk::Voxel::Chunk<S, S, S>;
		spk::World::ChunkCollection<BenchChunk> collection;
		Volume::Builder builder({S, S, S});
		builder.bind(registry);
		for (std::uint32_t z = 0; z < S; ++z)
		{
			for (std::uint32_t y = 0; y < S; ++y)
			{
				for (std::uint32_t x = 0; x < S; ++x)
				{
					if (!sparse || (x * 17 + y * 7 + z * 11) % 31 == 0)
					{
						builder.set({x, y, z}, 1);
					}
				}
			}
		}
		BenchChunk chunk;
		chunk.setVolume(builder.build());
		collection.addChunk({-1, 0, 0}, std::move(chunk));
		spk::Voxel::ChunkMesher<S, S, S> mesher(collection, {-1, 0, 0});
		const auto &volume = collection.at({-1, 0, 0}).volume();
		std::chrono::nanoseconds elapsed{0};
		std::size_t vertices = 0;
		std::size_t triangles = 0;
		constexpr int Repetitions = 5;
		for (int repeat = 0; repeat < Repetitions; ++repeat)
		{
			const auto start = std::chrono::steady_clock::now();
			const auto geometry = mesher.bake(volume);
			elapsed += std::chrono::duration_cast<std::chrono::nanoseconds>(
				std::chrono::steady_clock::now() - start);
			vertices = geometry.opaqueMesh().vertexCount() + geometry.transparentMesh().vertexCount();
			triangles = (geometry.opaqueMesh().indexCount() + geometry.transparentMesh().indexCount()) / 3;
		}
		const std::size_t voxels = static_cast<std::size_t>(S) * S * S;
		const double meanNs = static_cast<double>(elapsed.count()) / Repetitions;
		report << S << ',' << pattern << ',' << voxels << ',' << vertices << ',' << triangles
			   << ',' << meanNs << ',' << meanNs / voxels << '\n';
		std::cout << "ChunkMesher " << S << "^3 " << pattern << ": "
				  << vertices << " vertices, " << triangles << " triangles, "
				  << meanNs / 1000000.0 << " ms/chunk, " << meanNs / voxels << " ns/voxel\n";
		EXPECT_GT(elapsed.count(), 0);
		EXPECT_GT(vertices, 0u);
		EXPECT_GT(triangles, 0u);
	};
	benchmark.template operator()<8>("solid", false);
	benchmark.template operator()<16>("solid", false);
	benchmark.template operator()<32>("solid", false);
	benchmark.template operator()<16>("sparse", true);
	benchmark.template operator()<32>("sparse", true);
	report.flush();
	EXPECT_TRUE(std::filesystem::exists(output));
}
