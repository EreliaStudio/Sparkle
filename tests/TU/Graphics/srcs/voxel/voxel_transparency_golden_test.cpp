#include <gtest/gtest.h>

#include <GL/glew.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "engine/entity3d.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/engine/rendering_behaviour.hpp"
#include "rendering/render_command.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/engine_widget.hpp"
#include "voxel/mesher.hpp"
#include "voxel/palette.hpp"
#include "voxel/renderer.hpp"
#include "voxel/rendering_system.hpp"

namespace
{
	constexpr spk::Vector2UInt ImageSize{128, 96};
	constexpr spk::Vector2UInt Center{64, 48};
	constexpr const char *Category = "rendering/voxel-transparency";

	class SolidAtlas final : public spk::Texture
	{
	public:
		explicit SolidAtlas(std::array<std::uint8_t, 4> color)
		{
			std::array<std::uint8_t, 4 * 4 * 4> pixels{};
			for (std::size_t index = 0; index < 16; ++index)
			{
				std::copy(color.begin(), color.end(), pixels.begin() + 4 * index);
			}
			setPixels(std::span<const std::uint8_t>(pixels), {4, 4}, Format::RGBA);
			validate();
		}
	};

	[[nodiscard]] spk::Voxel::Geometry cube(float opacity)
	{
		spk::Voxel::Shape::Builder shapes;
		shapes.addFromJSON(
			spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / "cube.json"));
		const auto shape = shapes.build();
		spk::Voxel::Definition::SlotMapping mapping;
		for (const auto &slot : shape.slots())
		{
			mapping.emplace(slot.name, spk::Voxel::Definition::Slot{"paint", opacity});
		}
		spk::Voxel::Definition::Registry::Builder registry;
		registry.add(1, spk::Voxel::Definition(shape, std::move(mapping)));
		spk::Voxel::Volume::Builder volume({1, 1, 1});
		volume.bind(registry.build());
		volume.set({0, 0, 0}, 1);
		return spk::Voxel::Mesher().bake(volume.build());
	}

	[[nodiscard]] spk::Palette palette(const spk::Texture &atlas)
	{
		spk::Material::Builder material;
		material.add("albedo", atlas);
		spk::Voxel::Palette::Builder builder;
		builder.setMaterial("paint", material.build());
		return builder.build();
	}

	struct WBOITReading
	{
		std::array<float, 4> accumulation{};
		float revealage = 1.0f;
		bool observed = false;
	};

	struct ReadWBOITCommand final : spk::RenderCommand
	{
		std::shared_ptr<WBOITReading> value;

		explicit ReadWBOITCommand(std::shared_ptr<WBOITReading> reading) :
			value(std::move(reading))
		{
		}

		void execute(spk::RenderContext &) const override
		{
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glReadPixels(Center.x, Center.y, 1, 1, GL_RGBA, GL_FLOAT, value->accumulation.data());
			glReadBuffer(GL_COLOR_ATTACHMENT1);
			glReadPixels(Center.x, Center.y, 1, 1, GL_RED, GL_FLOAT, &value->revealage);
			value->observed = true;
		}
	};

	struct WBOITObserver final : spk::RenderingBehaviour
	{
		std::shared_ptr<WBOITReading> reading;

		void buildViewRenderSnapshot(
			spk::RenderSnapshot::Builder &builder,
			const spk::EngineView::CaptureContext &) override
		{
			if (reading)
			{
				builder.renderPass({"test.voxel-transparency.inspect", spk::EngineView::WBOITPassKey.order + 1})
					.emplace<ReadWBOITCommand>(reading);
			}
		}
	};

	struct Scene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"transparency camera"};
		spk::CameraComponent3D &camera = cameraEntity.addComponent<spk::CameraComponent3D>();
		spk::EngineView &view;
		spk::Voxel::RenderingSystem &voxels;
		std::vector<std::unique_ptr<spk::Entity3D>> objects;

		Scene() :
			view(_createView()),
			voxels(engine.addSystem<spk::Voxel::RenderingSystem>())
		{
			view.ambientLight().setColor({1.0f, 1.0f, 1.0f});
			view.ambientLight().setIntensity(1.0f);
		}

		spk::EngineView &_createView()
		{
			const spk::Vector3 eye{0.5f, 0.5f, 4.0f};
			cameraEntity.transform().place(eye);
			cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, {0.5f, 0.5f, 0.0f}));
			camera.setPerspective(0.85f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 20.0f);
			engine.addEntity(&cameraEntity);
			return engine.addView("transparency", &camera, ImageSize);
		}

		void add(const spk::Voxel::Geometry &geometry, const spk::Texture &atlas, float depth)
		{
			auto object = std::make_unique<spk::Entity3D>("transparency cube");
			object->transform().place({0.0f, 0.0f, depth});
			auto &renderer = object->addComponent<spk::Voxel::Renderer>();
			renderer.bind(geometry);
			renderer.bind(palette(atlas));
			engine.addEntity(object.get());
			objects.push_back(std::move(object));
		}

		~Scene()
		{
			for (const auto &object : objects)
			{
				engine.removeEntity(object.get());
			}
			engine.removeEntity(&cameraEntity);
		}
	};

	[[nodiscard]] sparkle_test::FramebufferImage render(Scene &scene, const std::string &golden = {})
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		gl.reset(ImageSize);
		spk::EngineWidget widget("transparency", nullptr);
		widget.setEngine(&scene.engine);
		widget.setGeometry({.anchor = {0, 0}, .size = ImageSize});
		widget.activate();
		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		builder.build().execute(gl.renderContext());
		const auto image = gl.capture();
		EXPECT_EQ(image.size, ImageSize);
		EXPECT_EQ(glGetError(), GL_NO_ERROR);
		if (!golden.empty())
		{
			const auto category = std::filesystem::path(Category);
			const auto expected = sparkle_test::expectedImagePath(category, golden);
			const auto actual = sparkle_test::resultImagePath(category, golden);
			const auto difference = sparkle_test::resultImagePath(category, golden + "_difference");
			gl.save(actual);
			EXPECT_TRUE(std::filesystem::exists(expected)) << "Missing PR25 golden " << expected;
			if (std::filesystem::exists(expected))
			{
				const auto comparison = sparkle_test::compareImages(actual, expected, difference);
				EXPECT_TRUE(comparison.matches) << golden << ": " << comparison.differentPixelCount << " pixels differ";
			}
		}
		gl.reset();
		return image;
	}

	[[nodiscard]] int displayColor(float hdr)
	{
		const double reinhard = static_cast<double>(hdr) / (1.0 + hdr);
		const double srgb = reinhard <= 0.0031308
								? reinhard * 12.92
								: 1.055 * std::pow(reinhard, 1.0 / 2.4) - 0.055;
		return static_cast<int>(std::lround(srgb * 255.0));
	}
}

TEST(VoxelTransparencyGolden, EmptyAndTransparentOnlyAndTwoLayers)
{
	SolidAtlas red({255, 0, 0, 255});
	SolidAtlas blue({0, 0, 255, 255});
	{
		Scene scene;
		const auto image = render(scene, "zero_layers");
		EXPECT_EQ(image.pixel(Center)[0], 0);
		EXPECT_EQ(image.pixel(Center)[3], 255);
	}
	{
		Scene scene;
		scene.add(cube(0.5f), red, 0.0f);
		const auto image = render(scene, "one_layer");
		EXPECT_NEAR(image.pixel(Center)[0], displayColor(0.5f), 3);
		EXPECT_EQ(image.pixel(Center)[3], 255);
	}
	{
		Scene scene;
		scene.add(cube(0.5f), red, 0.7f);
		scene.add(cube(0.5f), blue, -0.7f);
		const auto image = render(scene, "two_layers");
		EXPECT_GT(image.pixel(Center)[0], 0);
		EXPECT_GT(image.pixel(Center)[2], 0);
		EXPECT_EQ(image.pixel(Center)[3], 255);
	}
}

TEST(VoxelTransparencyGolden, IndependentDrawOrderPreservesWBOITResult)
{
	SolidAtlas red({255, 0, 0, 255});
	SolidAtlas blue({0, 0, 255, 255});
	Scene frontFirst;
	frontFirst.add(cube(0.5f), red, 0.7f);
	frontFirst.add(cube(0.5f), blue, -0.7f);
	const auto first = render(frontFirst, "two_layers_order_front_first");

	Scene backFirst;
	backFirst.add(cube(0.5f), blue, -0.7f);
	backFirst.add(cube(0.5f), red, 0.7f);
	const auto second = render(backFirst, "two_layers_order_back_first");
	for (std::size_t i = 0; i < first.pixels.size(); ++i)
	{
		EXPECT_NEAR(first.pixels[i], second.pixels[i], 2) << "Pixel byte " << i;
	}
}

TEST(VoxelTransparencyGolden, OpaqueDepthOccludesTranslucentBehind)
{
	SolidAtlas red({255, 0, 0, 255});
	SolidAtlas green({0, 255, 0, 255});
	Scene onlyOpaque;
	onlyOpaque.add(cube(1.0f), green, 0.0f);
	const auto reference = render(onlyOpaque);

	Scene withHiddenGlass;
	withHiddenGlass.add(cube(1.0f), green, 0.0f);
	withHiddenGlass.add(cube(0.5f), red, -1.4f);
	const auto actual = render(withHiddenGlass, "opaque_occludes_translucent");
	EXPECT_EQ(actual.pixels, reference.pixels);
	EXPECT_GT(actual.pixel(Center)[1], actual.pixel(Center)[0]);
}

TEST(VoxelTransparencyGolden, AlphaZeroSkipsBothTargetsAndAlphaOneQuantizesRevealToZero)
{
	SolidAtlas invisible({255, 0, 0, 0});
	SolidAtlas red({255, 0, 0, 255});
	{
		Scene scene;
		scene.add(cube(0.5f), invisible, 0.0f);
		const auto image = render(scene, "alpha_zero");
		EXPECT_EQ(image.pixel(Center)[0], 0);
		EXPECT_EQ(image.pixel(Center)[3], 255);
	}
	{
		Scene scene;
		scene.add(cube(0.9999f), red, 0.0f);
		const auto image = render(scene, "near_opaque_reveal_zero");
		EXPECT_NEAR(image.pixel(Center)[0], displayColor(1.0f), 2);
		EXPECT_EQ(image.pixel(Center)[3], 255);
	}
}

TEST(VoxelTransparencyGPU, RevealR8UsesQuantizedTransmittance)
{
	SolidAtlas red({255, 0, 0, 255});
	Scene scene;
	scene.add(cube(0.5f), red, 0.0f);
	const auto image = render(scene);
	const float revealR8 = std::round(0.5f * 255.0f) / 255.0f;
	EXPECT_NEAR(image.pixel(Center)[0], displayColor(1.0f - revealR8), 3);
	EXPECT_EQ(image.pixel(Center)[3], 255);
}

TEST(VoxelTransparencyGPU, MRTWeightUsesWindowDepthAndRevealQuantizesInR8)
{
	SolidAtlas red({255, 0, 0, 255});
	Scene scene;
	auto &observer = scene.engine.root().addBehaviour<WBOITObserver>();
	observer.reading = std::make_shared<WBOITReading>();
	scene.add(cube(0.5f), red, 0.0f);
	const auto image = render(scene);
	ASSERT_TRUE(observer.reading->observed);
	const float distance = 4.0f - 0.5f;
	const float nearPlane = 0.1f;
	const float farPlane = 20.0f;
	const float z = farPlane / (farPlane - nearPlane) -
					(farPlane * nearPlane) / ((farPlane - nearPlane) * distance);
	const float expectedWeight = 1.0f + 7.0f * (1.0f - z);
	EXPECT_NEAR(observer.reading->accumulation[0], 0.5f * expectedWeight, 0.03f);
	EXPECT_NEAR(observer.reading->accumulation[3], 0.5f * expectedWeight, 0.03f);
	EXPECT_NEAR(observer.reading->revealage, std::round(0.5f * 255.0f) / 255.0f, 1.0f / 255.0f);
	EXPECT_NEAR(image.pixel(Center)[0], displayColor(1.0f - observer.reading->revealage), 3);
}

TEST(VoxelTransparencyGPU, DepthWeightReferenceBounds)
{
	const auto weight = [](float windowDepth) {
		return 1.0f + 7.0f * (1.0f - windowDepth);
	};
	EXPECT_FLOAT_EQ(weight(0.0f), 8.0f);
	EXPECT_FLOAT_EQ(weight(0.5f), 4.5f);
	EXPECT_FLOAT_EQ(weight(1.0f), 1.0f);
}

TEST(VoxelTransparencyGPU, ExcessiveHDRDoesNotProduceUndefinedOutput)
{
	SolidAtlas red({255, 0, 0, 255});
	Scene scene;
	scene.view.ambientLight().setIntensity(1.0e10f);
	scene.add(cube(0.5f), red, 0.0f);
	const auto image = render(scene);
	EXPECT_GT(image.pixel(Center)[0], 0);
	EXPECT_EQ(image.pixel(Center)[3], 255);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}
