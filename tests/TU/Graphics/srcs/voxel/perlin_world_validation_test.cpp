#include <GL/glew.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "core/context/update_context.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "light/directional.hpp"
#include "rendering/command/draw_engine_view_command.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/render_snapshot.hpp"
#include "shadow_map/contributor.hpp"
#include "shadow_map/rendering_system.hpp"
#include "sparkle_test/gif_comparison.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "threading/worker_pool.hpp"
#include "voxel/chunk_mesher.hpp"
#include "voxel/chunk_rendering_system.hpp"
#include "world/chunk_collection.hpp"
#include "world/chunk_interest.hpp"
#include "world/local_chunk_requester.hpp"
#include "world/world_observer.hpp"
#include "world/world_streaming_system.hpp"

#include "perlin_world_fixture.hpp"

namespace
{
	using namespace std::chrono_literals;
	namespace terrain = sparkle_test::perlin_world;
	using Chunk = terrain::Chunk;
	using Coordinate = terrain::Coordinate;
	using Collection = spk::World::ChunkCollection<Chunk>;
	using ChunkGraphics = spk::Voxel::ChunkRenderingSystem<16, 16, 16>;
	using Streamer = spk::World::WorldStreamingSystem<Chunk>;
	using Clock = std::chrono::steady_clock;
	constexpr const char *GoldenCategory = "rendering/perlin-world";
	constexpr std::uint32_t ShadowResolution = 512;

	[[nodiscard]] std::uint64_t checksum(const Chunk &chunk)
	{
		std::uint64_t value = 0xcbf29ce484222325ULL;
		for (std::uint32_t z = 0; z < 16; ++z)
		{
			for (std::uint32_t y = 0; y < 16; ++y)
			{
				for (std::uint32_t x = 0; x < 16; ++x)
				{
					value = (value ^ chunk.volume().atRaw({x, y, z})) * 0x100000001b3ULL;
				}
			}
		}
		return value;
	}

	template <typename Operation>
	void measure(const char *name, std::size_t operations, Operation &&operation)
	{
		constexpr std::size_t Samples = 5;
		std::array<double, Samples> durations{};
		for (std::size_t i = 0; i < Samples; ++i)
		{
			const auto begin = Clock::now();
			operation();
			durations[i] = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
		}
		std::sort(durations.begin(), durations.end());
		std::cout << std::fixed << std::setprecision(3)
				  << "\n[Perlin world performance] phase=" << name
				  << " | operations=" << operations
				  << " | median=" << durations[Samples / 2]
				  << " ms | per_operation=" << durations[Samples / 2] / static_cast<double>(operations)
				  << " ms | min=" << durations.front()
				  << " ms | max=" << durations.back() << " ms\n";
		EXPECT_GE(durations[Samples / 2], 0.0);
	}

	class Scene final
	{
	public:
		spk::Engine engine;
		terrain::Registry definitions = terrain::registry();
		terrain::OutlinedAtlas atlas;
		terrain::Generator generator{definitions};
		spk::WorkerPool workers{4};
		spk::World::LocalChunkRequester<Chunk, terrain::GenerationData, terrain::Builder> requester{workers, generator};
		Collection chunks;
		spk::Entity3D cameraEntity{"Perlin camera"};
		spk::Entity3D sunEntity{"Perlin sun"};
		spk::Entity3D firstObserver{"First observer"};
		spk::Entity3D secondObserver{"Second observer"};
		spk::CameraComponent3D &camera = cameraEntity.addComponent<spk::CameraComponent3D>();
		spk::EngineView &view;
		ChunkGraphics &graphics;
		spk::ShadowMap::RenderingSystem &shadowSystem;
		spk::Light::Directional &sun = sunEntity.addComponent<spk::Light::Directional>();
		Streamer *streamer = nullptr;
		bool graphicsActive = true;
		std::int64_t tickTime = 0;

		explicit Scene(bool stream = false) :
			view(createView()),
			graphics(engine.addSystem<ChunkGraphics>()),
			shadowSystem(engine.addSystem<spk::ShadowMap::RenderingSystem>())
		{
			chunks.attach(engine);
			graphics.bind(chunks, terrain::palette(atlas));
			view.ambientLight().setColor({1.0f, 1.0f, 1.0f});
			view.ambientLight().setIntensity(0.18f);
			sunEntity.transform().place({-36.0f, 75.0f, 20.0f});
			sunEntity.transform().setRotation(spk::Quaternion::lookAt({-36, 75, 20}, {0, 0, 0}));
			sun.setColor({1.0f, 0.94f, 0.79f});
			sun.setIntensity(1.35f);
			sun.setCastsShadows(true);
			sun.setShadowResolution(ShadowResolution);
			sun.setShadowDistance(160.0f);
			engine.addEntity(&sunEntity);
			if (stream)
			{
				firstObserver.transform().place({0, 8, 0});
				secondObserver.transform().place({16, 8, 0});
				firstObserver.addComponent<spk::World::WorldObserver>(spk::World::ChunkInterest({1, 0, 1}));
				secondObserver.addComponent<spk::World::WorldObserver>(spk::World::ChunkInterest({1, 0, 1}));
				engine.addEntity(&firstObserver);
				engine.addEntity(&secondObserver);
				streamer = &engine.addSystem<Streamer>(
					chunks, requester, Streamer::Settings{.maxInFlight = 16, .timeout = 30s, .retryDelay = 500ms});
			}
		}

		spk::EngineView &createView()
		{
			setCamera({46.0f, 44.0f, 53.0f}, {0, 6, 0});
			camera.setPerspective(0.90f, static_cast<float>(terrain::ImageSize.x) / terrain::ImageSize.y, 0.1f, 240.0f);
			engine.addEntity(&cameraEntity);
			return engine.addView("Perlin", &camera, terrain::ImageSize);
		}

		void setCamera(spk::Vector3 position, spk::Vector3 target)
		{
			cameraEntity.transform().place(position);
			cameraEntity.transform().setRotation(spk::Quaternion::lookAt(position, target));
		}

		void fill(std::int32_t radius)
		{
			for (std::int32_t z = -radius; z <= radius; ++z)
			{
				for (std::int32_t x = -radius; x <= radius; ++x)
				{
					auto chunk = generator.generate({x, 0, z});
					ASSERT_TRUE(chunk.has_value());
					chunks.addChunk({x, 0, z}, std::move(*chunk));
				}
			}
			refreshCasters();
		}

		void refreshCasters()
		{
			graphics.synchronize();
			for (auto &[coordinate, entry] : chunks)
			{
				(void)coordinate;
				auto *renderer = entry.entity().getComponent<spk::Voxel::Renderer>();
				ASSERT_NE(renderer, nullptr);
				ASSERT_NE(renderer->geometry(), nullptr);
				auto *caster = entry.entity().getComponent<spk::ShadowMap::Contributor>();
				if (caster == nullptr)
				{
					caster = &entry.entity().addComponent<spk::ShadowMap::Contributor>();
					caster->setShadowMode(spk::ShadowMap::Mode::Static);
				}
				caster->bind(renderer->geometry()->depthMesh());
			}
		}

		void disableColorRendering()
		{
			graphics.unbind();
			engine.removeSystem(graphics);
			graphicsActive = false;
		}

		[[nodiscard]] std::size_t desiredChunks() const
		{
			std::set<Coordinate> desired;
			for (const auto *observer : {&firstObserver, &secondObserver})
			{
				const auto center = Chunk::worldToChunkCoordinate(observer->transform().position(spk::ReferenceFrame::World));
				for (const Coordinate &coordinate : spk::World::ChunkInterest({1, 0, 1}).coordinates(center))
				{
					desired.insert(coordinate);
				}
			}
			return desired.size();
		}

		void waitForStreaming()
		{
			ASSERT_NE(streamer, nullptr);
			const auto desired = [&]() {
				std::set<Coordinate> result;
				for (const auto *observer : {&firstObserver, &secondObserver})
				{
					const auto center = Chunk::worldToChunkCoordinate(observer->transform().position(spk::ReferenceFrame::World));
					for (const Coordinate &coordinate : spk::World::ChunkInterest({1, 0, 1}).coordinates(center))
					{
						result.insert(coordinate);
					}
				}
				return result;
			}();
			const auto tick = [&]() {
				spk::UpdateContext context{.time = std::chrono::milliseconds(tickTime++), .deltaTime = 1ms};
				engine.updateState(context);
			};
			tick();
			const auto deadline = Clock::now() + 15s;
			while (Clock::now() < deadline)
			{
				bool complete = chunks.size() == desired.size() && streamer->inFlightCount() == 0u;
				for (const auto &[coordinate, entry] : chunks)
				{
					(void)entry;
					complete = complete && desired.contains(coordinate);
				}
				if (complete)
				{
					break;
				}
				tick();
				std::this_thread::sleep_for(1ms);
			}
			EXPECT_EQ(chunks.size(), desired.size());
			for (const auto &[coordinate, entry] : chunks)
			{
				(void)entry;
				EXPECT_TRUE(desired.contains(coordinate));
			}
			EXPECT_EQ(streamer->inFlightCount(), 0u);
			refreshCasters();
		}

		[[nodiscard]] sparkle_test::FramebufferImage capture()
		{
			auto &gl = sparkle_test::OpenGLTestContext::instance();
			gl.reset(terrain::ImageSize);
			engine.refreshRenderSnapshot();
			const auto snapshot = engine.latestRenderSnapshot();
			EXPECT_NE(snapshot, nullptr);
			if (snapshot == nullptr)
			{
				return {};
			}
			snapshot->execute(gl.renderContext());
			spk::ViewportUniformRenderCommand({.anchor = {0, 0}, .size = terrain::ImageSize}).execute(gl.renderContext());
			spk::DrawEngineViewCommand(view.output(), {.anchor = {0, 0}, .size = terrain::ImageSize}).execute(gl.renderContext());
			gl.requireNoErrors();
			return gl.capture();
		}

		void verifyShadow()
		{
			auto &gl = sparkle_test::OpenGLTestContext::instance();
			const auto selection = view.lightChannel().current();
			ASSERT_NE(selection, nullptr);
			ASSERT_FALSE(selection->directional.empty());
			const auto result = view.lightChannel().shadowResultCollection().acquireLatest(gl.surface());
			ASSERT_NE(result, nullptr);
			const auto found = result->entries.find(selection->directional.front().registrationOrder);
			ASSERT_NE(found, result->entries.end());
			ASSERT_TRUE(found->second.depthTexture);
			EXPECT_EQ(found->second.resolution, ShadowResolution);
			spk::Framebuffer depth(
				{ShadowResolution, ShadowResolution},
				std::vector<spk::Texture::Handle>{},
				found->second.depthTexture);
			depth.activate(gl.renderContext());
			std::vector<float> samples(ShadowResolution * ShadowResolution, 1.0f);
			glReadPixels(0, 0, ShadowResolution, ShadowResolution, GL_DEPTH_COMPONENT, GL_FLOAT, samples.data());
			EXPECT_EQ(glGetError(), GL_NO_ERROR);
			EXPECT_LT(*std::min_element(samples.begin(), samples.end()), 1.0f);
		}

		~Scene()
		{
			if (streamer != nullptr)
			{
				engine.removeSystem(*streamer);
				engine.removeEntity(&secondObserver);
				engine.removeEntity(&firstObserver);
			}
			if (graphicsActive)
			{
				graphics.unbind();
			}
			chunks.detach();
			engine.removeEntity(&sunEntity);
			engine.removeEntity(&cameraEntity);
		}
	};

	// Temporary candidate transport for the initial, explicitly reviewed PNG/GIF
	// references. The CI run remains red when a reference is missing.
	void emitGoldenCandidate(const std::filesystem::path &filename, const std::string &destination)
	{
		std::ifstream input(filename, std::ios::binary);
		if (!input)
		{
			return;
		}
		std::vector<unsigned char> bytes(std::istreambuf_iterator<char>(input), {});
		constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		std::string encoded;
		encoded.reserve(((bytes.size() + 2) / 3) * 4);
		for (std::size_t index = 0; index < bytes.size(); index += 3)
		{
			const std::uint32_t a = bytes[index];
			const std::uint32_t b = index + 1 < bytes.size() ? bytes[index + 1] : 0;
			const std::uint32_t c = index + 2 < bytes.size() ? bytes[index + 2] : 0;
			const std::uint32_t word = (a << 16) | (b << 8) | c;
			encoded.push_back(alphabet[(word >> 18) & 63]);
			encoded.push_back(alphabet[(word >> 12) & 63]);
			encoded.push_back(index + 1 < bytes.size() ? alphabet[(word >> 6) & 63] : '=');
			encoded.push_back(index + 2 < bytes.size() ? alphabet[word & 63] : '=');
		}
		std::cout << "SPARKLE_GOLDEN_BEGIN:" << destination << ':' << bytes.size() << '\\n';
		for (std::size_t offset = 0; offset < encoded.size(); offset += 3072)
		{
			std::cout << "SPARKLE_GOLDEN_DATA:" << encoded.substr(offset, 3072) << '\\n';
		}
		std::cout << "SPARKLE_GOLDEN_END:" << destination << '\\n';
	}

	void expectGolden(Scene &scene, const std::string &name)
	{
		const auto pixels = scene.capture();
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		const auto actual = sparkle_test::resultImagePath(GoldenCategory, name);
		gl.save(actual);
		const auto expected = sparkle_test::expectedImagePath(GoldenCategory, name);
		if (!std::filesystem::exists(expected))
		{
			emitGoldenCandidate(actual, name + ".png");
		}
		ASSERT_TRUE(std::filesystem::exists(expected)) << "Missing approved Perlin world golden: " << expected << "; actual " << actual;
		sparkle_test::ImageComparisonOptions tolerance;
		tolerance.rgbTolerance = 8;
		tolerance.alphaTolerance = 8;
		const auto result = sparkle_test::compareImages(
			actual, expected, sparkle_test::resultImagePath(GoldenCategory, name + "_difference"), tolerance);
		EXPECT_TRUE(result.matches) << name << ": " << result.differentPixelCount << " different pixels";
		EXPECT_EQ(pixels.size, terrain::ImageSize);
	}

	void expectAnimatedGolden(const std::vector<sparkle_test::FramebufferImage> &frames, const std::string &name)
	{
		const auto expected = sparkle_test::expectedImagesDirectory() / GoldenCategory / (name + "_expected.gif");
		const auto actual = sparkle_test::freshResultFilePath(GoldenCategory, name + "_actual.gif");
		const auto diagnostics = sparkle_test::resultsDirectory() / GoldenCategory / (name + "_frames");
		sparkle_test::GifComparisonOptions options;
		options.image.rgbTolerance = 24;
		options.maximumDifferentPixelsPerFrame = 150;
		const auto result = sparkle_test::compareAnimationGif(frames, expected, actual, diagnostics, options);
		if (!std::filesystem::exists(expected))
		{
			emitGoldenCandidate(actual, name + "_expected.gif");
		}
		EXPECT_TRUE(result.matches) << "Golden GIF changed: " << actual;
	}
}

TEST(PerlinWorldIntegration, NegativeCoordinatesAndGenerationOrderAreDeterministic)
{
	const auto definitions = terrain::registry();
	terrain::Generator generator(definitions);
	const std::array<Coordinate, 6> coordinates{
		Coordinate{-2, 0, -2}, {-1, 0, 0}, {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {2, 0, -1}};
	std::array<std::uint64_t, coordinates.size()> first{};
	for (std::size_t i = 0; i < coordinates.size(); ++i)
	{
		const auto chunk = generator.generate(coordinates[i]);
		ASSERT_TRUE(chunk.has_value());
		first[i] = checksum(*chunk);
	}
	for (std::size_t i = coordinates.size(); i > 0; --i)
	{
		const auto chunk = generator.generate(coordinates[i - 1]);
		ASSERT_TRUE(chunk.has_value());
		EXPECT_EQ(checksum(*chunk), first[i - 1]);
	}
	EXPECT_FALSE(generator.generate({5, 0, 0}).has_value());
	EXPECT_FALSE(generator.generate({0, 1, 0}).has_value());
	const auto negative = generator.generate({-1, 0, 0});
	const auto positive = generator.generate({0, 0, 0});
	ASSERT_TRUE(negative.has_value());
	ASSERT_TRUE(positive.has_value());
	const std::array<const Chunk *, 2> neighbors{&*negative, &*positive};
	for (std::uint32_t z = 0; z < 16; ++z)
	{
		for (std::size_t side = 0; side < neighbors.size(); ++side)
		{
			const Chunk *chunk = neighbors[side];
			const std::uint32_t localX = side == 0 ? 15u : 0u;
			const auto globalX = side == 0 ? -1 : 0;
			const auto top = static_cast<std::uint32_t>(terrain::height(globalX, static_cast<std::int32_t>(z)));
			EXPECT_NE(chunk->volume().atRaw({localX, top, z}), spk::Voxel::Volume::PackedCell::Air);
			if (top < 15)
			{
				EXPECT_EQ(chunk->volume().atRaw({localX, top + 1, z}), spk::Voxel::Volume::PackedCell::Air);
			}
		}
	}
}

TEST(PerlinWorldGolden, TerrainOverviewWithDirectionalShadows)
{
	Scene scene;
	scene.fill(2);
	EXPECT_EQ(scene.chunks.size(), 25u);
	expectGolden(scene, "overview_shadows");
	scene.verifyShadow();
}

TEST(PerlinWorldGolden, ObliqueCameraAndOutlinedShapes)
{
	Scene scene;
	scene.fill(2);
	scene.setCamera({-48, 35, 37}, {-5, 8, -5});
	expectGolden(scene, "oblique_outlined");
	scene.verifyShadow();
}

TEST(PerlinWorldGolden, EditedChunkAndSixNeighborRemeshing)
{
	Scene scene;
	scene.fill(2);
	const Coordinate center{0, 0, 0};
	auto *original = &scene.chunks.at(center);
	auto *entity = &scene.chunks.begin()->second.entity();
	(void)entity;
	const auto before = checksum(*original);
	spk::Voxel::Volume::Builder edit({16, 16, 16});
	edit.load(original->volume());
	for (std::uint32_t x = 5; x <= 10; ++x)
	{
		for (std::uint32_t z = 5; z <= 10; ++z)
		{
			edit.erase({x, static_cast<std::uint32_t>(terrain::height(static_cast<int>(x), static_cast<int>(z))), z});
		}
	}
	original->setVolume(edit.build());
	EXPECT_NE(checksum(*original), before);
	scene.refreshCasters();
	EXPECT_GE(scene.graphics.lastStatistics().chunks, 1u);
	expectGolden(scene, "terrain_edited");
	scene.verifyShadow();
}

TEST(PerlinWorldGolden, TwoObserversStreamingAnimatedGIF)
{
	Scene scene(true);
	std::vector<sparkle_test::FramebufferImage> frames;
	for (int step = 0; step < 4; ++step)
	{
		const float shift = static_cast<float>(step * 16);
		scene.firstObserver.transform().place({shift - 16.0f, 8, 0});
		scene.secondObserver.transform().place({shift, 8, 0});
		scene.waitForStreaming();
		EXPECT_EQ(scene.chunks.size(), scene.desiredChunks());
		scene.setCamera({shift + 28.0f, 35, 42}, {shift, 7, 0});
		frames.push_back(scene.capture());
		scene.verifyShadow();
	}
	expectAnimatedGolden(frames, "streaming");
}

TEST(PerlinWorldPerformance, GenerationOnly49Chunks)
{
	const auto definitions = terrain::registry();
	terrain::Generator generator(definitions);
	std::uint64_t guard = 0;
	measure("generation", 49, [&]() {
		for (int z = -3; z <= 3; ++z)
		{
			for (int x = -3; x <= 3; ++x)
			{
				const auto chunk = generator.generate({x, 0, z});
				ASSERT_TRUE(chunk.has_value());
				guard += chunk->volume().atRaw({8, 8, 8});
			}
		}
	});
	std::cout << "[Perlin world performance] generation guard=" << guard << '\n';
}

TEST(PerlinWorldPerformance, NeighborAwareMeshing25Chunks)
{
	const auto definitions = terrain::registry();
	terrain::Generator generator(definitions);
	Collection collection;
	for (int z = -2; z <= 2; ++z)
	{
		for (int x = -2; x <= 2; ++x)
		{
			collection.addChunk({x, 0, z}, *generator.generate({x, 0, z}));
		}
	}
	spk::Voxel::Occlusion::Cache cache;
	std::size_t vertices = 0;
	measure("neighbor_aware_meshing", 25, [&]() {
		for (const auto &[coordinate, entry] : collection)
		{
			spk::Voxel::ChunkMesher<16, 16, 16> mesher(collection, coordinate);
			mesher.attachCache(cache);
			const auto geometry = mesher.bake(entry.chunk().volume());
			vertices += geometry.opaqueMesh().vertexCount();
		}
	});
	EXPECT_GT(vertices, 0u);
	std::cout << "[Perlin world performance] meshing vertices=" << vertices / 5 << '\n';
}

TEST(PerlinWorldPerformance, ShadowMapComputationSeparateFromVoxelColorRendering)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(terrain::ImageSize);
	Scene scene;
	scene.fill(1);
	// Only shadow-map contributors remain in the scene's snapshot; voxel
	// color rendering is intentionally removed from this measurement.
	scene.disableColorRendering();
	std::vector<std::shared_ptr<const spk::RenderSnapshot>> snapshots;
	for (int index = 0; index < 5; ++index)
	{
		const spk::Vector3 position{-36.0f + static_cast<float>(index) * 4.0f, 75.0f, 20.0f};
		scene.sunEntity.transform().setRotation(spk::Quaternion::lookAt(position, {0, 0, 0}));
		scene.engine.refreshRenderSnapshot();
		snapshots.push_back(scene.engine.latestRenderSnapshot());
		ASSERT_NE(snapshots.back(), nullptr);
	}
	std::size_t index = 0;
	measure("shadow_map_depth_only", 1, [&]() {
		snapshots[index++]->execute(gl.renderContext());
		glFinish();
	});
	gl.requireNoErrors();
	gl.reset();
}

TEST(PerlinWorldPerformance, ColorRenderWithoutShadows)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(terrain::ImageSize);
	Scene scene;
	scene.fill(1);
	scene.sun.setCastsShadows(false);
	scene.engine.refreshRenderSnapshot();
	const auto snapshot = scene.engine.latestRenderSnapshot();
	ASSERT_NE(snapshot, nullptr);
	measure("voxel_color_without_shadows", 1, [&]() {
		snapshot->execute(gl.renderContext());
		glFinish();
	});
	gl.requireNoErrors();
	gl.reset();
}
