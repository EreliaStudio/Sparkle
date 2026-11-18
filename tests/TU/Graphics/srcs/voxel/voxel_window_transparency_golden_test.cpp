#include <gtest/gtest.h>

#include <GL/glew.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
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
	constexpr spk::Vector2UInt WindowImageSize{320, 240};
	constexpr std::uint32_t HalfImageX = WindowImageSize.x / 2;
	constexpr const char *WindowGoldenCategory = "rendering/voxel-transparency-windows";

	class SolidColorTexture final : public spk::Texture
	{
	public:
		explicit SolidColorTexture(std::array<std::uint8_t, 4> rgba)
		{
			std::array<std::uint8_t, 4 * 4 * 4> pixels{};
			for (std::size_t index = 0; index < 16; ++index)
			{
				std::copy(rgba.begin(), rgba.end(), pixels.begin() + 4 * index);
			}
			setPixels(std::span<const std::uint8_t>(pixels), {4, 4}, Format::RGBA);
			validate();
		}
	};

	[[nodiscard]] spk::Voxel::Shape cubeShape()
	{
		spk::Voxel::Shape::Builder builder;
		builder.addFromJSON(
			spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / "cube.json"));
		return builder.build();
	}

	[[nodiscard]] spk::Voxel::Definition definition(
		const spk::Voxel::Shape &shape,
		const char *material,
		float opacity = 1.0f)
	{
		spk::Voxel::Definition::SlotMapping mapping;
		for (const auto &slot : shape.slots())
		{
			mapping.emplace(slot.name, spk::Voxel::Definition::Slot{material, opacity});
		}
		return {shape, std::move(mapping)};
	}

	[[nodiscard]] spk::Voxel::Geometry wallGeometry(bool wideOpening)
	{
		spk::Voxel::Definition::Registry::Builder registry;
		registry.add(1, definition(cubeShape(), "wall"));
		spk::Voxel::Volume::Builder volume({4, 4, 1});
		volume.bind(registry.build());
		for (std::uint32_t x = 0; x < 4; ++x)
		{
			for (std::uint32_t y = 0; y < 4; ++y)
			{
				const bool aperture = (y == 1 || y == 2) && (x == 1 || (wideOpening && x == 2));
				if (!aperture)
				{
					volume.set({x, y, 0}, 1);
				}
			}
		}
		return spk::Voxel::Mesher().bake(volume.build());
	}

	[[nodiscard]] spk::Voxel::Geometry glassGeometry()
	{
		spk::Voxel::Definition::Registry::Builder registry;
		registry.add(1, definition(cubeShape(), "glass", 0.48f));
		spk::Voxel::Volume::Builder volume({1, 2, 1});
		volume.bind(registry.build());
		volume.set({0, 0, 0}, 1);
		volume.set({0, 1, 0}, 1);
		return spk::Voxel::Mesher().bake(volume.build());
	}

	[[nodiscard]] spk::Voxel::Geometry characterGeometry()
	{
		const auto shape = cubeShape();
		spk::Voxel::Definition::Registry::Builder registry;
		registry.add(1, definition(shape, "shirt"));
		registry.add(2, definition(shape, "skin"));
		registry.add(3, definition(shape, "hair"));
		registry.add(4, definition(shape, "trousers"));
		registry.add(5, definition(shape, "eyes"));

		spk::Voxel::Volume::Builder volume({7, 14, 3});
		volume.bind(registry.build());

		for (std::uint32_t y = 0; y < 4; ++y)
		{
			volume.set({2, y, 1}, 4);
			volume.set({4, y, 1}, 4);
		}
		for (std::uint32_t y = 4; y < 9; ++y)
		{
			for (std::uint32_t x = 2; x <= 4; ++x)
			{
				volume.set({x, y, 1}, 1);
			}
		}
		for (std::uint32_t y = 6; y < 9; ++y)
		{
			volume.set({1, y, 1}, 2);
			volume.set({5, y, 1}, 2);
		}
		for (std::uint32_t y = 9; y < 12; ++y)
		{
			for (std::uint32_t x = 2; x <= 4; ++x)
			{
				volume.set({x, y, 1}, 2);
			}
		}
		for (std::uint32_t x = 2; x <= 4; ++x)
		{
			volume.set({x, 12, 1}, 3);
		}
		volume.set({2, 11, 2}, 5);
		volume.set({4, 11, 2}, 5);
		return spk::Voxel::Mesher().bake(volume.build());
	}

	[[nodiscard]] spk::Palette solidPalette(const char *name, const spk::Texture &texture)
	{
		spk::Material::Builder material;
		material.add("albedo", texture);
		spk::Voxel::Palette::Builder builder;
		builder.setMaterial(name, material.build());
		return builder.build();
	}

	[[nodiscard]] spk::Palette characterPalette(
		const spk::Texture &shirt,
		const spk::Texture &skin,
		const spk::Texture &hair,
		const spk::Texture &trousers,
		const spk::Texture &eyes)
	{
		spk::Voxel::Palette::Builder builder;
		const auto add = [&builder](const char *name, const spk::Texture &texture) {
			spk::Material::Builder material;
			material.add("albedo", texture);
			builder.setMaterial(name, material.build());
		};
		add("shirt", shirt);
		add("skin", skin);
		add("hair", hair);
		add("trousers", trousers);
		add("eyes", eyes);
		return builder.build();
	}

	struct WindowAssets
	{
		SolidColorTexture wall{{120, 122, 134, 255}};
		SolidColorTexture glass{{25, 155, 245, 210}};
		SolidColorTexture shirt{{226, 65, 42, 255}};
		SolidColorTexture skin{{254, 186, 123, 255}};
		SolidColorTexture hair{{70, 40, 22, 255}};
		SolidColorTexture trousers{{45, 67, 193, 255}};
		SolidColorTexture eyes{{15, 16, 25, 255}};

		spk::Voxel::Geometry wallNarrow = wallGeometry(false);
		spk::Voxel::Geometry wallWide = wallGeometry(true);
		spk::Voxel::Geometry pane = glassGeometry();
		spk::Voxel::Geometry actor = characterGeometry();

		spk::Palette wallMaterials = solidPalette("wall", wall);
		spk::Palette glassMaterials = solidPalette("glass", glass);
		spk::Palette actorMaterials = characterPalette(shirt, skin, hair, trousers, eyes);
	};

	struct WindowScene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"window transparency camera"};
		spk::CameraComponent3D &camera = cameraEntity.addComponent<spk::CameraComponent3D>();
		spk::EngineView &view;
		spk::Voxel::RenderingSystem &voxels;
		std::vector<std::unique_ptr<spk::Entity3D>> objects;

		explicit WindowScene(float focusX) :
			view(_createView(focusX)),
			voxels(engine.addSystem<spk::Voxel::RenderingSystem>())
		{
			view.ambientLight().setColor({1.0f, 1.0f, 1.0f});
			view.ambientLight().setIntensity(1.0f);
		}

		spk::EngineView &_createView(float focusX)
		{
			const spk::Vector3 eye{focusX, 1.0f, 7.0f};
			cameraEntity.transform().place(eye);
			cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, {focusX, 1.0f, 0.0f}));
			camera.setPerspective(
				0.82f,
				static_cast<float>(WindowImageSize.x) / WindowImageSize.y,
				0.1f,
				30.0f);
			engine.addEntity(&cameraEntity);
			return engine.addView("window", &camera, WindowImageSize);
		}

		void add(
			const spk::Voxel::Geometry &geometry,
			const spk::Palette &palette,
			spk::Vector3 position,
			spk::Vector3 scale = {1.0f, 1.0f, 1.0f})
		{
			auto object = std::make_unique<spk::Entity3D>("window transparency object");
			object->transform().place(position);
			object->transform().rescale(scale);
			auto &renderer = object->addComponent<spk::Voxel::Renderer>();
			renderer.bind(geometry);
			renderer.bind(palette);
			engine.addEntity(object.get());
			objects.push_back(std::move(object));
		}

		~WindowScene()
		{
			for (const auto &object : objects)
			{
				engine.removeEntity(object.get());
			}
			engine.removeEntity(&cameraEntity);
		}
	};

	void populateWindow(
		WindowScene &scene,
		const WindowAssets &assets,
		bool wideOpening,
		bool includeGlass,
		bool includeCharacter,
		float characterCenterX)
	{
		scene.add(
			wideOpening ? assets.wallWide : assets.wallNarrow,
			assets.wallMaterials,
			{-1.0f, -1.0f, 0.0f});
		if (includeGlass)
		{
			scene.add(assets.pane, assets.glassMaterials, {0.0f, 0.0f, 0.0f});
		}
		if (includeCharacter)
		{
			scene.add(
				assets.actor,
				assets.actorMaterials,
				{characterCenterX - 0.49f, 0.15f, -1.45f},
				{0.14f, 0.12f, 0.12f});
		}
	}

	[[nodiscard]] sparkle_test::FramebufferImage renderWindow(
		WindowScene &scene,
		const std::string &golden = {})
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		gl.reset(WindowImageSize);
		spk::EngineWidget widget("window transparency", nullptr);
		widget.setEngine(&scene.engine);
		widget.setGeometry({.anchor = {0, 0}, .size = WindowImageSize});
		widget.activate();

		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		builder.build().execute(gl.renderContext());

		const auto image = gl.capture();
		EXPECT_EQ(image.size, WindowImageSize);
		EXPECT_EQ(glGetError(), GL_NO_ERROR);
		if (!golden.empty())
		{
			const auto category = std::filesystem::path(WindowGoldenCategory);
			const auto expected = sparkle_test::expectedImagePath(category, golden);
			const auto actual = sparkle_test::resultImagePath(category, golden);
			const auto difference = sparkle_test::resultImagePath(category, golden + "_difference");
			gl.save(actual);
			EXPECT_TRUE(std::filesystem::exists(expected)) << "Missing window golden " << expected;
			if (std::filesystem::exists(expected))
			{
				const auto comparison = sparkle_test::compareImages(actual, expected, difference);
				EXPECT_TRUE(comparison.matches)
					<< golden << ": " << comparison.differentPixelCount << " pixels differ";
			}
		}
		gl.reset();
		return image;
	}

	[[nodiscard]] std::size_t differentPixels(
		const sparkle_test::FramebufferImage &left,
		const sparkle_test::FramebufferImage &right,
		std::uint32_t startX,
		std::uint32_t endX)
	{
		std::size_t differences = 0;
		for (std::uint32_t y = 0; y < WindowImageSize.y; ++y)
		{
			for (std::uint32_t x = startX; x < endX; ++x)
			{
				if (left.pixel({x, y}) != right.pixel({x, y}))
				{
					++differences;
				}
			}
		}
		return differences;
	}
}

TEST(VoxelTransparencyWindowGolden, FramedGlassShowsCharacterBehindWindow)
{
	WindowAssets assets;
	WindowScene glazed(0.5f);
	populateWindow(glazed, assets, false, true, true, 0.5f);
	const auto throughGlass = renderWindow(glazed, "framed_window_character_behind");

	WindowScene clear(0.5f);
	populateWindow(clear, assets, false, false, true, 0.5f);
	const auto withoutGlass = renderWindow(clear, "framed_window_without_glass");

	WindowScene empty(0.5f);
	populateWindow(empty, assets, false, true, false, 0.5f);
	const auto withoutCharacter = renderWindow(empty);

	EXPECT_GT(differentPixels(throughGlass, withoutCharacter, 0, WindowImageSize.x), 40u);
	EXPECT_GT(differentPixels(throughGlass, withoutGlass, 0, WindowImageSize.x), 40u);
	EXPECT_EQ(throughGlass.pixel({2, 2}), withoutGlass.pixel({2, 2}));
	EXPECT_EQ(throughGlass.pixel({2, 2})[3], 255);
}

TEST(VoxelTransparencyWindowGolden, CharacterStraddlesGlassAndUncoveredHalf)
{
	WindowAssets assets;
	WindowScene halfGlazed(1.0f);
	populateWindow(halfGlazed, assets, true, true, true, 1.0f);
	const auto glassAndClear = renderWindow(halfGlazed, "character_half_glass_half_clear");

	WindowScene fullyOpen(1.0f);
	populateWindow(fullyOpen, assets, true, false, true, 1.0f);
	const auto entirelyClear = renderWindow(fullyOpen, "character_both_halves_clear");

	WindowScene noCharacter(1.0f);
	populateWindow(noCharacter, assets, true, true, false, 1.0f);
	const auto emptyOpening = renderWindow(noCharacter);

	EXPECT_GT(differentPixels(glassAndClear, entirelyClear, 0, HalfImageX - 4), 40u);
	EXPECT_EQ(differentPixels(glassAndClear, entirelyClear, HalfImageX + 4, WindowImageSize.x), 0u);
	EXPECT_GT(differentPixels(glassAndClear, emptyOpening, 0, HalfImageX - 4), 20u);
	EXPECT_GT(differentPixels(glassAndClear, emptyOpening, HalfImageX + 4, WindowImageSize.x), 20u);
	EXPECT_EQ(glassAndClear.pixel({2, 2})[3], 255);
}

TEST(VoxelTransparencyWindowGolden, OpaqueWallFullyOccludesCharacterOutsideOpening)
{
	WindowAssets assets;
	WindowScene wall(0.5f);
	populateWindow(wall, assets, false, true, false, 2.0f);
	const auto emptyWall = renderWindow(wall);

	WindowScene hidden(0.5f);
	populateWindow(hidden, assets, false, true, true, 2.0f);
	const auto hiddenCharacter = renderWindow(hidden, "window_wall_hides_character");

	EXPECT_EQ(hiddenCharacter.pixels, emptyWall.pixels);
}
