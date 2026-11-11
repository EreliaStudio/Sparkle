#include <GL/glew.h>
#include <gtest/gtest.h>

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
#include "engine/transform3d.hpp"
#include "graphics/image.hpp"
#include "model3d/object.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/engine_widget.hpp"
#include "voxel/mesher.hpp"
#include "voxel/model_element.hpp"
#include "voxel/model_element_view.hpp"
#include "voxel/model_renderer.hpp"
#include "voxel/palette.hpp"
#include "voxel/renderer.hpp"
#include "voxel/rendering_system.hpp"

namespace
{
	using Blueprint = spk::Model3D::Blueprint;
	using Model = spk::Voxel::ModelRenderer::Model;

	constexpr spk::Vector2UInt ImageSize{320, 240};
	constexpr const char *GoldenCategory = "rendering/model3d-voxel";

	class TestAtlas final : public spk::Texture
	{
	public:
		TestAtlas()
		{
			std::array<std::uint8_t, 8 * 8 * 4> pixels{};
			for (std::size_t y = 0; y < 8; ++y)
			{
				for (std::size_t x = 0; x < 8; ++x)
				{
					const auto offset = (y * 8 + x) * 4;
					pixels[offset] = x < 4 ? 225 : 60;
					pixels[offset + 1] = y < 4 ? 175 : 80;
					pixels[offset + 2] = x < 4 ? 100 : 240;
					pixels[offset + 3] = 255;
				}
			}
			setPixels(std::span<const std::uint8_t>(pixels), {8, 8}, Format::RGBA);
			validate();
		}
	};

	[[nodiscard]] spk::Voxel::Volume volume(spk::Vector3UInt dimensions)
	{
		spk::Voxel::Shape::Builder shapes;
		shapes.addFromJSON(spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / "cube.json"));
		spk::Voxel::Definition::SlotMapping mapping;
		const auto shape = shapes.build();
		for (const auto &slot : shape.slots())
		{
			mapping.emplace(slot.name, spk::Voxel::Definition::Slot{"stone", 1.0f});
		}
		spk::Voxel::Definition::Registry::Builder registry;
		registry.add(1, spk::Voxel::Definition(shape, std::move(mapping)));
		spk::Voxel::Volume::Builder builder(dimensions);
		builder.bind(registry.build());
		for (std::uint32_t x = 0; x < dimensions.x; ++x)
		{
			for (std::uint32_t y = 0; y < dimensions.y; ++y)
			{
				for (std::uint32_t z = 0; z < dimensions.z; ++z)
				{
					builder.set({x, y, z}, 1);
				}
			}
		}
		return builder.build();
	}

	[[nodiscard]] spk::Palette palette(const spk::Texture &atlas, bool alternate = false)
	{
		spk::Material::Builder material;
		spk::Texture::Section section{{0.0f, 0.0f}, {0.5f, 1.0f}};
		if (alternate)
		{
			section = {{0.5f, 0.0f}, {0.5f, 1.0f}};
		}
		material.add("albedo", atlas, section);
		spk::Voxel::Palette::Builder builder;
		builder.setMaterial("stone", material.build());
		return builder.build();
	}

	[[nodiscard]] Blueprint characterBlueprint()
	{
		Blueprint::Builder builder;
		auto &torso = builder.setRoot<spk::Voxel::ModelElement>("torso", volume({3, 4, 2}));
		torso.setAnchor({-1.5f, -2.0f, -1.0f});
		torso.addAttachmentPoint("neck", {1.5f, 4.0f, 1.0f});
		auto &head = torso.attach<spk::Voxel::ModelElement>("neck", "head", volume({2, 2, 2}));
		head.setAnchor({-1.0f, 0, -1.0f});
		torso.addAttachmentPoint("leftShoulder", {0, 3.5f, 1.0f});
		torso.addAttachmentPoint("rightShoulder", {3, 3.5f, 1.0f});
		auto &leftArm = torso.attach<spk::Voxel::ModelElement>("leftShoulder", "leftArm", volume({1, 3, 1}));
		auto &rightArm = torso.attach<spk::Voxel::ModelElement>("rightShoulder", "rightArm", volume({1, 3, 1}));
		leftArm.setAnchor({-0.5f, -3.0f, -0.5f});
		rightArm.setAnchor({-0.5f, -3.0f, -0.5f});
		rightArm.addAttachmentPoint("wrist", {0.5f, 0, 0.5f});
		auto &hand = rightArm.attach<spk::Voxel::ModelElement>("wrist", "rightHand", volume({1, 1, 1}));
		hand.setAnchor({-0.5f, -1.0f, -0.5f});
		hand.addAttachmentPoint("grip", {0.5f, 0, 0.5f}, spk::Quaternion::fromEuler({0, 0, 90}));
		torso.addAttachmentPoint("leftHip", {0.5f, 0, 1});
		torso.addAttachmentPoint("rightHip", {2.5f, 0, 1});
		auto &leftLeg = torso.attach<spk::Voxel::ModelElement>("leftHip", "leftLeg", volume({1, 3, 1}));
		auto &rightLeg = torso.attach<spk::Voxel::ModelElement>("rightHip", "rightLeg", volume({1, 3, 1}));
		leftLeg.setAnchor({-0.5f, -3.0f, -0.5f});
		rightLeg.setAnchor({-0.5f, -3.0f, -0.5f});
		return builder.build();
	}

	[[nodiscard]] Blueprint swordBlueprint()
	{
		Blueprint::Builder builder;
		auto &sword = builder.setRoot<spk::Voxel::ModelElement>("sword", volume({1, 4, 1}));
		sword.setAnchor({-0.5f, 0, -0.5f});
		return builder.build();
	}

	enum class Pose
	{
		Neutral,
		LeftArm,
		BothArms,
		Walking,
		Sword
	};

	[[nodiscard]] const char *poseName(Pose pose)
	{
		switch (pose)
		{
		case Pose::Neutral:
			return "neutral";
		case Pose::LeftArm:
			return "left_arm_raised";
		case Pose::BothArms:
			return "both_arms_raised";
		case Pose::Walking:
			return "walking";
		case Pose::Sword:
			return "sword";
		}
		return "unknown";
	}

	void setPose(Model &model, Pose pose)
	{
		if (pose == Pose::LeftArm || pose == Pose::BothArms)
		{
			model.attachment("torso", "leftShoulder").setRotation(spk::Vector3{0, 0, -95});
		}
		if (pose == Pose::BothArms)
		{
			model.attachment("torso", "rightShoulder").setRotation(spk::Vector3{0, 0, 95});
		}
		if (pose == Pose::Walking)
		{
			model.attachment("torso", "leftHip").setRotation(spk::Vector3{28, 0, 0});
			model.attachment("torso", "rightHip").setRotation(spk::Vector3{-28, 0, 0});
		}
		if (pose == Pose::Sword)
		{
			model.attachment("rightHand", "grip").attach(swordBlueprint());
		}
	}

	struct Scene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"Model camera"};
		spk::CameraComponent3D &camera = cameraEntity.addComponent<spk::CameraComponent3D>();
		spk::EngineView &view;
		spk::Voxel::RenderingSystem &system;
		std::vector<std::unique_ptr<spk::Entity3D>> entities;

		Scene() :
			view(_createView()),
			system(engine.addSystem<spk::Voxel::RenderingSystem>())
		{
		}

		spk::EngineView &_createView()
		{
			const spk::Vector3 eye{7, 5, 12};
			cameraEntity.transform().place(eye);
			cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, {0, -1, 0}));
			camera.setPerspective(0.78f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 40.0f);
			engine.addEntity(&cameraEntity);
			return engine.addView("main", &camera, ImageSize);
		}

		spk::Voxel::ModelRenderer &addModel(Blueprint blueprint, const spk::Palette &appearance, spk::Vector3 position = {})
		{
			auto entity = std::make_unique<spk::Entity3D>("model");
			entity->transform().place(position);
			auto &renderer = entity->addComponent<spk::Voxel::ModelRenderer>();
			renderer.bind(std::move(blueprint));
			renderer.bind(appearance);
			engine.addEntity(entity.get());
			entities.push_back(std::move(entity));
			return renderer;
		}

		void addOrdinary(spk::Voxel::Geometry geometry, const spk::Palette &appearance)
		{
			auto entity = std::make_unique<spk::Entity3D>("ordinary voxel");
			auto &renderer = entity->addComponent<spk::Voxel::Renderer>();
			renderer.bind(std::move(geometry));
			renderer.bind(appearance);
			engine.addEntity(entity.get());
			entities.push_back(std::move(entity));
		}

		[[nodiscard]] std::size_t commands()
		{
			spk::RenderSnapshot::Builder builder;
			spk::EngineView::CaptureContext context;
			system.buildViewRenderSnapshot(builder, context);
			return builder.renderPass(spk::EngineView::OpaquePassKey).commandCount();
		}

		[[nodiscard]] sparkle_test::FramebufferImage capture()
		{
			auto &gl = sparkle_test::OpenGLTestContext::instance();
			gl.reset(ImageSize);
			spk::EngineWidget widget("model", nullptr);
			widget.setEngine(&engine);
			widget.setGeometry({.anchor = {0, 0}, .size = ImageSize});
			widget.activate();
			spk::RenderSnapshot::Builder builder;
			widget.buildRenderSnapshot(builder);
			builder.build().execute(gl.renderContext());
			return gl.capture();
		}

		~Scene()
		{
			for (const auto &entity : entities)
			{
				engine.removeEntity(entity.get());
			}
			engine.removeEntity(&cameraEntity);
		}
	};

	void expectGolden(const std::string &name)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		const auto actual = sparkle_test::resultImagePath(GoldenCategory, name);
		const auto expected = sparkle_test::expectedImagePath(GoldenCategory, name);
		const auto difference = sparkle_test::resultImagePath(GoldenCategory, name + "_difference");
		gl.save(actual);
		ASSERT_TRUE(std::filesystem::exists(expected)) << "Missing approved reference: " << expected << "; actual: " << actual;
		const auto comparison = sparkle_test::compareImages(actual, expected, difference);
		EXPECT_TRUE(comparison.matches) << "Voxel Model3D mismatch: " << name << "; modified pixels: " << comparison.differentPixelCount << "; diff: " << difference;
	}
}

static_assert(spk::Model3D::ObjectGraphics<spk::Voxel::ModelElementView>);

TEST(VoxelModel3DTest, PosesKeepViewsAndGeometryIdentities)
{
	TestAtlas atlas;
	Scene scene;
	auto &renderer = scene.addModel(characterBlueprint(), palette(atlas));
	auto &model = *renderer.model();
	model.synchronize();
	const auto *first = model.graphics(model.element("leftArm"));
	ASSERT_NE(first, nullptr);
	const auto mesh = first->geometry().opaqueMesh().layout().vertexBuffer().identifier();
	const auto count = model.graphicsCount();
	model.attachment("torso", "leftShoulder").setRotation(spk::Vector3{0, 0, -95});
	model.attachment("torso", "leftHip").setRotation(spk::Vector3{28, 0, 0});
	model.synchronize();
	EXPECT_EQ(model.graphics(model.element("leftArm")), first);
	EXPECT_EQ(first->geometry().opaqueMesh().layout().vertexBuffer().identifier(), mesh);
	EXPECT_EQ(model.graphicsCount(), count);
}

TEST(VoxelModel3DTest, TwoModelsAndOrdinaryRendererShareInstancedGeometry)
{
	TestAtlas atlas;
	Scene scene;
	const auto blueprint = characterBlueprint();
	const auto appearance = palette(atlas);
	auto &first = scene.addModel(blueprint, appearance, {-4, 0, 0});
	const auto count = scene.commands();
	const auto geometry = first.model()->graphics(first.model()->element("torso"))->geometry();
	auto &second = scene.addModel(blueprint, appearance, {4, 0, 0});
	second.model()->attachment("torso", "rightShoulder").setRotation(spk::Vector3{0, 0, 80});
	EXPECT_EQ(scene.commands(), count);
	EXPECT_EQ(
		first.model()->graphics(first.model()->element("torso"))->geometry().opaqueMesh().layout().vertexBuffer().identifier(),
		second.model()->graphics(second.model()->element("torso"))->geometry().opaqueMesh().layout().vertexBuffer().identifier());
	scene.addOrdinary(geometry, appearance);
	EXPECT_EQ(scene.commands(), count);
	second.model()->graphics(second.model()->element("torso"))->bind(palette(atlas, true));
	EXPECT_EQ(scene.commands(), count + 1);
}

TEST(VoxelModel3DTest, ReplacingVolumeDoesNotMutatePreviousGeometry)
{
	TestAtlas atlas;
	Scene scene;
	auto &renderer = scene.addModel(characterBlueprint(), palette(atlas));
	auto &model = *renderer.model();
	auto *view = model.graphics(model.element("head"));
	ASSERT_NE(view, nullptr);
	const auto old = view->geometry();
	const auto oldId = old.opaqueMesh().layout().vertexBuffer().identifier();
	view->bind(volume({3, 3, 3}));
	EXPECT_NE(view->geometry().opaqueMesh().layout().vertexBuffer().identifier(), oldId);
	EXPECT_EQ(old.opaqueMesh().layout().vertexBuffer().identifier(), oldId);
	model.attachment("torso", "neck").setRotation(spk::Vector3{0, 45, 0});
	EXPECT_EQ(model.graphics(model.element("head")), view);
	view->clearVolumeOverride();
	EXPECT_EQ(view->geometry().opaqueMesh().layout().vertexBuffer().identifier(), oldId);
}

TEST(VoxelModel3DTest, DynamicSwordAttachmentAndParentRotationAreCaptured)
{
	TestAtlas atlas;
	Scene scene;
	auto &renderer = scene.addModel(characterBlueprint(), palette(atlas));
	auto &model = *renderer.model();
	const auto count = scene.commands();
	auto &grip = model.attachment("rightHand", "grip");
	spk::Model3D::ObjectHeadless &sword = grip.attach(swordBlueprint());
	EXPECT_EQ(scene.commands(), count + 1);
	const auto &hand = model.element("rightHand");
	const auto expected = hand.globalTransform() * grip.transform() * sword.root().definition().anchor();
	for (std::size_t column = 0; column < 4; ++column)
	{
		for (std::size_t row = 0; row < 4; ++row)
		{
			EXPECT_NEAR(sword.root().globalTransform()[column][row], expected[column][row], 0.0001f);
		}
	}
	const auto original = sword.root().globalTransform() * spk::Vector4{0, 4, 0, 1};
	model.attachment("torso", "rightShoulder").setRotation(spk::Vector3{0, 0, 70});
	grip.setRotation(spk::Vector3{0, 30, 0});
	const auto updated = sword.root().globalTransform() * spk::Vector4{0, 4, 0, 1};
	EXPECT_GT(std::abs(updated.x - original.x) + std::abs(updated.y - original.y), 1.0f);
	spk::Model3D::ObjectHeadless detached = grip.detach(sword);
	EXPECT_EQ(grip.attachedCount(), 0u);
	EXPECT_EQ(scene.commands(), count);
	EXPECT_EQ(model.graphicsCount(), 7u);
}

TEST(VoxelModel3DTest, LayersAndInvalidScaleApplyToModels)
{
	TestAtlas atlas;
	Scene scene;
	auto &renderer = scene.addModel(characterBlueprint(), palette(atlas));
	renderer.setLayers({"characters"});
	spk::RenderSnapshot::Builder builder;
	spk::EngineView::CaptureContext context;
	context.layers = spk::EngineView::Layers{"terrain"};
	scene.system.buildViewRenderSnapshot(builder, context);
	EXPECT_EQ(builder.renderPass(spk::EngineView::OpaquePassKey).commandCount(), 0u);
	renderer.owner()->transform().rescale({1, 2, 1});
	EXPECT_THROW(scene.engine.refreshRenderSnapshot(), spk::Exception);
}

class VoxelModel3DGoldenTest : public ::testing::TestWithParam<Pose>
{
};

TEST_P(VoxelModel3DGoldenTest, ArticulatedCharacterUsesVoxelPipeline)
{
	TestAtlas atlas;
	Scene scene;
	auto &renderer = scene.addModel(characterBlueprint(), palette(atlas));
	setPose(*renderer.model(), GetParam());
	const auto image = scene.capture();
	EXPECT_EQ(image.size, ImageSize);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	expectGolden(poseName(GetParam()));
	sparkle_test::OpenGLTestContext::instance().reset();
}

INSTANTIATE_TEST_SUITE_P(
	ArticulatedPoses,
	VoxelModel3DGoldenTest,
	::testing::Values(Pose::Neutral, Pose::LeftArm, Pose::BothArms, Pose::Walking, Pose::Sword),
	[](const ::testing::TestParamInfo<Pose> &info) {
		return poseName(info.param);
	});
