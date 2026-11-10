#include <GL/glew.h>
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "geometry/texture_mesh_3d.hpp"
#include "graphics/color.hpp"
#include "graphics/opengl/program.hpp"
#include "graphics/opengl/uniform.hpp"
#include "model3d/object.hpp"
#include "rendering/command/clear_render_command.hpp"
#include "rendering/command/viewport_render_command.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/engine/engine_view.hpp"
#include "rendering/engine/rendering_behaviour.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/engine_widget.hpp"

namespace
{
	using Blueprint = spk::Model3D::Blueprint;

	constexpr spk::Vector2UInt ImageSize{640, 480};
	constexpr spk::Color Background{0.06f, 0.09f, 0.14f, 1.0f};
	constexpr const char *GoldenCategory = "rendering/model3d";

	class ColorVolume final : public Blueprint::Element
	{
	public:
		spk::Color color;
		spk::Vector3 center;
		spk::Vector3 size;

		ColorVolume(std::string name, spk::Color fill, spk::Vector3 position, spk::Vector3 dimensions) :
			Element(std::move(name)),
			color(fill),
			center(position),
			size(dimensions)
		{
		}
	};

	void addFace(
		spk::TextureMesh3D::Builder &mesh,
		spk::Vector3 normal,
		spk::Vector3 a,
		spk::Vector3 b,
		spk::Vector3 c,
		spk::Vector3 d)
	{
		mesh.addShape({a, normal, {0, 0}}, {b, normal, {0, 1}}, {c, normal, {1, 1}}, {d, normal, {1, 0}});
	}

	[[nodiscard]] spk::TextureMesh3D createBox(const ColorVolume &volume)
	{
		const auto point = [&](float x, float y, float z) -> spk::Vector3 {
			return {
				volume.center.x + x * volume.size.x * 0.5f,
				volume.center.y + y * volume.size.y * 0.5f,
				volume.center.z + z * volume.size.z * 0.5f};
		};
		spk::TextureMesh3D::Builder mesh;
		addFace(mesh, {0, 0, 1}, point(-1, 1, 1), point(-1, -1, 1), point(1, -1, 1), point(1, 1, 1));
		addFace(mesh, {0, 0, -1}, point(1, 1, -1), point(1, -1, -1), point(-1, -1, -1), point(-1, 1, -1));
		addFace(mesh, {1, 0, 0}, point(1, 1, 1), point(1, -1, 1), point(1, -1, -1), point(1, 1, -1));
		addFace(mesh, {-1, 0, 0}, point(-1, 1, -1), point(-1, -1, -1), point(-1, -1, 1), point(-1, 1, 1));
		addFace(mesh, {0, 1, 0}, point(-1, 1, -1), point(-1, 1, 1), point(1, 1, 1), point(1, 1, -1));
		addFace(mesh, {0, -1, 0}, point(-1, -1, 1), point(-1, -1, -1), point(1, -1, -1), point(1, -1, 1));
		return std::move(mesh).build();
	}

	class DrawColorVolumeCommand final : public spk::RenderCommand
	{
	private:
		spk::TextureMesh3D _mesh;
		spk::Matrix4x4 _transform;
		spk::Color _color;

		[[nodiscard]] static spk::Program &_program()
		{
			static spk::Program result(
				R"(#version 430 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
uniform mat4 uTransform;
flat out float vShade;
void main()
{
	gl_Position = uTransform * vec4(aPosition, 1.0);
	vShade = 0.65 + 0.35 * max(dot(aNormal, normalize(vec3(0.3, 0.6, 0.7))), 0.0);
}
)",
				R"(#version 430 core
flat in float vShade;
uniform vec4 uColor;
layout(location = 0) out vec4 fragColor;
void main() { fragColor = vec4(uColor.rgb * vShade, uColor.a); }
)");
			return result;
		}

	public:
		DrawColorVolumeCommand(spk::TextureMesh3D mesh, spk::Matrix4x4 transform, spk::Color color) :
			_mesh(std::move(mesh)),
			_transform(transform),
			_color(color)
		{
		}

		void execute(spk::RenderContext &context) const override
		{
			auto &program = _program();
			program.activate(context);
			spk::Uniform<spk::Matrix4x4>("uTransform", _transform).activate(context);
			spk::Uniform<spk::Vector4>("uColor", {_color.r, _color.g, _color.b, _color.a}).activate(context);
			_mesh.layout().activate(context);
			program.render(spk::Program::Primitive::Triangles, _mesh.indexType(), 0, _mesh.indexCount());
		}
	};

	class ColorView final
	{
	public:
		using ElementType = ColorVolume;

	private:
		spk::TextureMesh3D _mesh;
		spk::Color _color;

	public:
		explicit ColorView(const ColorVolume &element) :
			_mesh(createBox(element)),
			_color(element.color)
		{
		}

		void buildRenderSnapshot(spk::RenderSnapshot::Builder &builder, const spk::Matrix4x4 &transform) const
		{
			builder.renderPass(spk::EngineView::OpaquePassKey).emplace<DrawColorVolumeCommand>(_mesh, transform, _color);
		}
	};

	static_assert(spk::Model3D::ObjectGraphics<ColorView>);
	using Character = spk::Model3D::Object<ColorView>;

	[[nodiscard]] Blueprint characterBlueprint()
	{
		Blueprint::Builder builder;
		auto &torso = builder.setRoot<ColorVolume>("torso", spk::Color{0.17f, 0.46f, 0.78f, 1}, spk::Vector3{}, spk::Vector3{0.93f, 1.35f, 0.48f});
		torso.addAttachmentPoint("neck", {0, 0.73f, 0});
		torso.attach<ColorVolume>("neck", "head", spk::Color{0.96f, 0.76f, 0.51f, 1}, spk::Vector3{0, 0.32f, 0}, spk::Vector3{0.62f, 0.64f, 0.54f});
		torso.addAttachmentPoint("leftShoulder", {-0.58f, 0.56f, 0});
		torso.addAttachmentPoint("rightShoulder", {0.58f, 0.56f, 0});
		auto &leftArm = torso.attach<ColorVolume>("leftShoulder", "leftArm", spk::Color{0.94f, 0.41f, 0.25f, 1}, spk::Vector3{0, -0.44f, 0}, spk::Vector3{0.28f, 0.88f, 0.35f});
		auto &rightArm = torso.attach<ColorVolume>("rightShoulder", "rightArm", spk::Color{0.37f, 0.73f, 0.54f, 1}, spk::Vector3{0, -0.44f, 0}, spk::Vector3{0.28f, 0.88f, 0.35f});
		leftArm.addAttachmentPoint("wrist", {0, -0.90f, 0});
		rightArm.addAttachmentPoint("wrist", {0, -0.90f, 0});
		leftArm.attach<ColorVolume>("wrist", "leftHand", spk::Color{0.98f, 0.79f, 0.54f, 1}, spk::Vector3{0, -0.15f, 0}, spk::Vector3{0.25f, 0.30f, 0.27f});
		auto &rightHand = rightArm.attach<ColorVolume>("wrist", "rightHand", spk::Color{0.98f, 0.79f, 0.54f, 1}, spk::Vector3{0, -0.15f, 0}, spk::Vector3{0.25f, 0.30f, 0.27f});
		rightHand.addAttachmentPoint("grip", {0, -0.28f, 0}, spk::Quaternion::fromEuler({0, 0, 90}));
		torso.addAttachmentPoint("leftHip", {-0.25f, -0.72f, 0});
		torso.addAttachmentPoint("rightHip", {0.25f, -0.72f, 0});
		torso.attach<ColorVolume>("leftHip", "leftLeg", spk::Color{0.34f, 0.49f, 0.81f, 1}, spk::Vector3{0, -0.56f, 0}, spk::Vector3{0.34f, 1.11f, 0.40f});
		torso.attach<ColorVolume>("rightHip", "rightLeg", spk::Color{0.34f, 0.49f, 0.81f, 1}, spk::Vector3{0, -0.56f, 0}, spk::Vector3{0.34f, 1.11f, 0.40f});
		return builder.build();
	}

	[[nodiscard]] Blueprint swordBlueprint()
	{
		Blueprint::Builder builder;
		auto &hilt = builder.setRoot<ColorVolume>(
			"swordHilt", spk::Color{0.37f, 0.25f, 0.16f, 1}, spk::Vector3{0, -0.22f, 0}, spk::Vector3{0.16f, 0.44f, 0.16f});
		hilt.addAttachmentPoint("guard", {0, -0.46f, 0});
		auto &guard = hilt.attach<ColorVolume>(
			"guard", "swordGuard", spk::Color{0.95f, 0.72f, 0.18f, 1}, spk::Vector3{}, spk::Vector3{0.58f, 0.13f, 0.17f});
		guard.addAttachmentPoint("blade", {0, -0.11f, 0});
		guard.attach<ColorVolume>(
			"blade", "swordBlade", spk::Color{0.82f, 0.88f, 0.95f, 1}, spk::Vector3{0, -0.48f, 0}, spk::Vector3{0.19f, 0.96f, 0.12f});
		return builder.build();
	}

	enum class Pose
	{
		Neutral,
		OneArmRaised,
		BothArmsRaised,
		Walking,
		Combined
	};

	[[nodiscard]] std::string poseName(Pose pose)
	{
		switch (pose)
		{
		case Pose::Neutral:
			return "neutral";
		case Pose::OneArmRaised:
			return "arm_raised";
		case Pose::BothArmsRaised:
			return "both_arms_raised";
		case Pose::Walking:
			return "walking";
		case Pose::Combined:
			return "combined";
		}
		return "unknown";
	}

	void poseCharacter(Character &character, Pose pose)
	{
		spk::Vector3 leftArm{}, rightArm{}, leftLeg{}, rightLeg{};
		switch (pose)
		{
		case Pose::OneArmRaised:
			leftArm.z = -95;
			break;
		case Pose::BothArmsRaised:
			leftArm.z = -95;
			rightArm.z = 95;
			break;
		case Pose::Walking:
			leftLeg.x = 28;
			rightLeg.x = -28;
			break;
		case Pose::Combined:
			leftArm.z = -55;
			rightArm.z = 75;
			leftLeg.x = 22;
			rightLeg.x = -22;
			break;
		default:
			break;
		}
		character.attachment("torso", "leftShoulder").setRotation(leftArm);
		character.attachment("torso", "rightShoulder").setRotation(rightArm);
		character.attachment("torso", "leftHip").setRotation(leftLeg);
		character.attachment("torso", "rightHip").setRotation(rightLeg);
	}

	class PreviewScene final
	{
	private:
		spk::Entity3D _cameraEntity{"Preview camera"};
		spk::CameraComponent3D &_camera;
		Character &_character;

	public:
		explicit PreviewScene(Character &character) :
			_camera(_cameraEntity.addComponent<spk::CameraComponent3D>("camera")),
			_character(character)
		{
			const spk::Vector3 position{3.0f, 2.0f, 7.2f};
			_cameraEntity.transform().place(position);
			_cameraEntity.transform().setRotation(spk::Quaternion::lookAt(position, {0, -0.2f, 0}));
			_camera.setPerspective(0.78f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 30.0f);
		}

		void buildRenderSnapshot(spk::RenderSnapshot::Builder &builder)
		{
			_character.buildRenderSnapshot(builder, _camera.viewProjectionMatrix());
		}
	};

	[[nodiscard]] sparkle_test::FramebufferImage capture(PreviewScene &scene, const std::string &name)
	{
		auto &context = sparkle_test::OpenGLTestContext::instance();
		context.reset(ImageSize);
		spk::RenderSnapshot::Builder builder;
		auto &pass = builder.renderPass({"puppet-3d", 0});
		pass.emplace<spk::ClearRenderCommand>(Background, spk::ClearRenderCommand::Mask::All);
		pass.emplace<spk::ViewportRenderCommand>(spk::Rect2D{.anchor = {0, 0}, .size = ImageSize});
		scene.buildRenderSnapshot(builder);
		builder.build().execute(context.renderContext());
		context.save(sparkle_test::resultImagePath(GoldenCategory, name));
		return context.capture();
	}

	void expectGolden(const std::string &name)
	{
		const auto actual = sparkle_test::resultImagePath(GoldenCategory, name);
		const auto expected = sparkle_test::expectedImagePath(GoldenCategory, name);
		const auto difference = sparkle_test::resultImagePath(GoldenCategory, name + "_difference");
		ASSERT_TRUE(std::filesystem::exists(expected)) << "Missing approved golden: " << expected << "; rendered: " << actual;
		sparkle_test::ImageComparisonOptions tolerance;
		tolerance.rgbTolerance = 6;
		tolerance.alphaTolerance = 8;
		const auto comparison = sparkle_test::compareImages(actual, expected, difference, tolerance, false);
		EXPECT_EQ(comparison.actualWidth, comparison.expectedWidth);
		EXPECT_EQ(comparison.actualHeight, comparison.expectedHeight);
		EXPECT_LE(comparison.differentPixelCount, 150u) << "Golden mismatch: " << name << "; diff: " << difference;
	}

	[[nodiscard]] spk::Vector4 pointOn(const Character &character, const char *part, spk::Vector3 local = {})
	{
		return character.element(part).globalTransform() * spk::Vector4{local.x, local.y, local.z, 1.0f};
	}

	void expectJointChain(const spk::Model3D::ObjectHeadless &character, const char *parent, const char *joint, const char *child)
	{
		const auto &source = character.element(parent);
		const auto &target = character.element(child);
		const auto expected = source.globalTransform() * source.attachment(joint).transform() * target.definition().anchor();
		for (std::size_t column = 0; column < 4; ++column)
		{
			for (std::size_t row = 0; row < 4; ++row)
			{
				EXPECT_NEAR(target.globalTransform()[column][row], expected[column][row], 0.0001f);
			}
		}
	}
}

class Object3DGoldenTest : public ::testing::TestWithParam<Pose>
{
};

TEST_P(Object3DGoldenTest, ArticulatedCharacterMatchesReference)
{
	const Pose pose = GetParam();
	Character character(characterBlueprint());
	poseCharacter(character, pose);
	const auto name = poseName(pose);
	PreviewScene scene(character);
	const auto image = capture(scene, name);
	EXPECT_EQ(character.graphicsCount(), 8u);
	EXPECT_EQ(image.size.x, ImageSize.x);
	EXPECT_EQ(image.size.y, ImageSize.y);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	expectGolden(name);
}

TEST(Object3DGoldenTest, SwordGripIsPerpendicularToHandAxis)
{
	Character character(characterBlueprint());
	spk::Model3D::ObjectHeadless &sword = character.attachment("rightHand", "grip").attach(swordBlueprint());
	const auto handAxis = character.element("rightHand").globalTransform() * spk::Vector4{0, -1, 0, 0};
	const auto swordAxis = sword.root().globalTransform() * spk::Vector4{0, -1, 0, 0};
	const float dot = handAxis.x * swordAxis.x + handAxis.y * swordAxis.y + handAxis.z * swordAxis.z;
	EXPECT_NEAR(dot, 0.0f, 0.0001f);
	EXPECT_GT(swordAxis.x, 0.99f);
	EXPECT_NEAR(swordAxis.y, 0.0f, 0.0001f);
}

TEST(Object3DGoldenTest, SwordAttachedToRightHandMatchesGoldensAndFollowsArm)
{
	Character character(characterBlueprint());
	auto &grip = character.attachment("rightHand", "grip");
	spk::Model3D::ObjectHeadless &sword = grip.attach(swordBlueprint());
	PreviewScene scene(character);
	const auto restingBlade = sword.element("swordBlade").globalTransform() * spk::Vector4{0, -0.96f, 0, 1};
	const auto resting = capture(scene, "sword_resting");
	EXPECT_EQ(character.graphicsCount(), 11u);
	ASSERT_NE(character.graphics(sword.root()), nullptr);
	const auto *hiltGraphics = character.graphics(sword.root());
	expectGolden("sword_resting");

	character.attachment("torso", "rightShoulder").setRotation(spk::Vector3{0, 0, 55});
	const auto raisedBlade = sword.element("swordBlade").globalTransform() * spk::Vector4{0, -0.96f, 0, 1};
	const auto raised = capture(scene, "sword_raised");
	EXPECT_NE(resting.pixels, raised.pixels);
	EXPECT_EQ(character.graphicsCount(), 11u);
	EXPECT_EQ(character.graphics(sword.root()), hiltGraphics);
	EXPECT_GT(std::abs(raisedBlade.x - restingBlade.x) + std::abs(raisedBlade.y - restingBlade.y), 1.0f);
	EXPECT_GT(raisedBlade.y, restingBlade.y + 0.5f);
	expectGolden("sword_raised");

	const auto expected = character.element("rightHand").globalTransform() * grip.transform() * sword.root().definition().anchor();
	for (std::size_t axis = 0; axis < 3; ++axis)
	{
		EXPECT_NEAR(sword.root().globalTransform()[3][axis], expected[3][axis], 0.0001f);
	}
	expectJointChain(sword, "swordHilt", "guard", "swordGuard");
	expectJointChain(sword, "swordGuard", "blade", "swordBlade");
	spk::Model3D::ObjectHeadless detached = grip.detach(sword);
	character.synchronize();
	EXPECT_EQ(character.graphicsCount(), 8u);
	EXPECT_EQ(grip.attachedCount(), 0u);
}

TEST(Object3DGoldenTest, ArticulationsMoveBothHandsAndLegsThroughParentTransforms)
{
	Character character(characterBlueprint());
	poseCharacter(character, Pose::Neutral);
	const auto leftHandAtRest = pointOn(character, "leftHand", {0, -0.15f, 0});
	const auto rightHandAtRest = pointOn(character, "rightHand", {0, -0.15f, 0});
	const auto leftFootAtRest = pointOn(character, "leftLeg", {0, -1.10f, 0});
	const auto rightFootAtRest = pointOn(character, "rightLeg", {0, -1.10f, 0});
	poseCharacter(character, Pose::Combined);
	expectJointChain(character, "torso", "leftShoulder", "leftArm");
	expectJointChain(character, "torso", "leftHip", "leftLeg");
	expectJointChain(character, "leftArm", "wrist", "leftHand");
	const auto leftHand = pointOn(character, "leftHand", {0, -0.15f, 0});
	const auto rightHand = pointOn(character, "rightHand", {0, -0.15f, 0});
	const auto leftFoot = pointOn(character, "leftLeg", {0, -1.10f, 0});
	const auto rightFoot = pointOn(character, "rightLeg", {0, -1.10f, 0});
	EXPECT_GT(std::abs(leftHand.x - leftHandAtRest.x), 0.35f);
	EXPECT_GT(std::abs(rightHand.x - rightHandAtRest.x), 0.35f);
	EXPECT_LT(leftFoot.z, leftFootAtRest.z - 0.30f);
	EXPECT_GT(rightFoot.z, rightFootAtRest.z + 0.30f);
	EXPECT_NEAR(leftFoot.x, leftFootAtRest.x, 0.001f);
	EXPECT_NEAR(rightFoot.x, rightFootAtRest.x, 0.001f);
}

TEST(Object3DGoldenTest, WalkingPoseSwingsLegsForwardAndBackwardWithoutLateralCrossing)
{
	Character character(characterBlueprint());
	poseCharacter(character, Pose::Neutral);
	const auto neutralLeft = pointOn(character, "leftLeg", {0, -1.10f, 0});
	const auto neutralRight = pointOn(character, "rightLeg", {0, -1.10f, 0});
	poseCharacter(character, Pose::Walking);
	const auto walkingLeft = pointOn(character, "leftLeg", {0, -1.10f, 0});
	const auto walkingRight = pointOn(character, "rightLeg", {0, -1.10f, 0});
	expectJointChain(character, "torso", "leftHip", "leftLeg");
	expectJointChain(character, "torso", "rightHip", "rightLeg");
	EXPECT_LT(walkingLeft.z, neutralLeft.z - 0.4f);
	EXPECT_GT(walkingRight.z, neutralRight.z + 0.4f);
	EXPECT_NEAR(walkingLeft.x, neutralLeft.x, 0.001f);
	EXPECT_NEAR(walkingRight.x, neutralRight.x, 0.001f);
	EXPECT_LT(walkingLeft.x, walkingRight.x);
}

INSTANTIATE_TEST_SUITE_P(
	FivePoses,
	Object3DGoldenTest,
	::testing::Values(Pose::Neutral, Pose::OneArmRaised, Pose::BothArmsRaised, Pose::Walking, Pose::Combined),
	[](const ::testing::TestParamInfo<Pose> &info) {
		return poseName(info.param);
	});

namespace
{
	constexpr spk::Vector2UInt RoomImageSize{320, 240};
	constexpr const char *RoomGoldenCategory = "rendering/engine/model3d_room";

	class RoomColorVolumeCommand final : public spk::RenderCommand
	{
	private:
		spk::TextureMesh3D _mesh;
		spk::Matrix4x4 _transform;
		spk::Vector4 _linearColor;

		[[nodiscard]] static spk::Program &_program()
		{
			static spk::Program result(
				R"(#version 430 core
layout(location = 0) in vec3 aPosition;
uniform mat4 uTransform;
void main()
{
	gl_Position = uTransform * vec4(aPosition, 1.0);
}
)",
				R"(#version 430 core
uniform vec4 uColor;
layout(location = 0) out vec4 fragColor;
void main()
{
	fragColor = uColor;
}
)");
			return result;
		}

	public:
		RoomColorVolumeCommand(const ColorVolume &volume, spk::Matrix4x4 transform, spk::RGB ambient) :
			_mesh(createBox(volume)),
			_transform(transform),
			_linearColor{
				volume.color.r * ambient.r,
				volume.color.g * ambient.g,
				volume.color.b * ambient.b,
				1.0f}
		{
		}

		void execute(spk::RenderContext &context) const override
		{
			auto &program = _program();
			program.activate(context);
			spk::Uniform<spk::Matrix4x4>("uTransform", _transform).activate(context);
			spk::Uniform<spk::Vector4>("uColor", _linearColor).activate(context);
			_mesh.layout().activate(context);
			program.render(spk::Program::Primitive::Triangles, _mesh.indexType(), 0, _mesh.indexCount());
		}
	};

	class RoomTransparentVolumeCommand final : public spk::RenderCommand
	{
	private:
		spk::TextureMesh3D _mesh;
		spk::Matrix4x4 _transform;
		spk::Vector4 _color;

		[[nodiscard]] static spk::Program &_program()
		{
			static spk::Program result(
				R"(#version 430 core
layout(location = 0) in vec3 aPosition;
uniform mat4 uTransform;
void main() { gl_Position = uTransform * vec4(aPosition, 1.0); }
)",
				R"(#version 430 core
uniform vec4 uColor;
layout(location = 0) out vec4 accumulation;
layout(location = 1) out vec4 revealage;
void main()
{
	accumulation = vec4(uColor.rgb * uColor.a, uColor.a);
	revealage = vec4(uColor.a);
}
)");
			return result;
		}

	public:
		RoomTransparentVolumeCommand(const ColorVolume &volume, spk::Matrix4x4 transform, spk::RGB ambient) :
			_mesh(createBox(volume)),
			_transform(transform),
			_color{volume.color.r * ambient.r, volume.color.g * ambient.g, volume.color.b * ambient.b, volume.color.a}
		{
		}

		void execute(spk::RenderContext &context) const override
		{
			auto &program = _program();
			program.activate(context);
			spk::Uniform<spk::Matrix4x4>("uTransform", _transform).activate(context);
			spk::Uniform<spk::Vector4>("uColor", _color).activate(context);
			_mesh.layout().activate(context);
			program.render(spk::Program::Primitive::Triangles, _mesh.indexType(), 0, _mesh.indexCount());
		}
	};

	class RoomColorContributor final : public spk::RenderingBehaviour
	{
	private:
		Character &_character;
		int _glassCount = 0;

		static void _append(
			spk::RenderSnapshot::Builder &builder,
			const ColorVolume &volume,
			const spk::Matrix4x4 &transform,
			spk::RGB ambient)
		{
			builder.renderPass(spk::EngineView::OpaquePassKey)
				.emplace<RoomColorVolumeCommand>(volume, transform, ambient);
		}

		static void _appendCharacter(
			spk::RenderSnapshot::Builder &builder,
			const spk::Model3D::Attachable &node,
			const spk::Matrix4x4 &viewProjection,
			spk::RGB ambient)
		{
			if (const auto *element = dynamic_cast<const spk::Model3D::ObjectHeadless::ElementNode *>(&node))
			{
				if (const auto *volume = dynamic_cast<const ColorVolume *>(&element->definition()))
				{
					_append(builder, *volume, viewProjection * element->globalTransform(), ambient);
				}
			}
			for (const auto *child : node.children())
			{
				_appendCharacter(builder, *child, viewProjection, ambient);
			}
		}

		static void _appendRoom(
			spk::RenderSnapshot::Builder &builder,
			const spk::EngineView::CaptureContext &context)
		{
			const ColorVolume floor("floor", {0.37f, 0.38f, 0.44f, 1.0f}, {0, -1.97f, 0}, {5.6f, 0.16f, 5.6f});
			const ColorVolume backWall("backWall", {0.28f, 0.39f, 0.47f, 1.0f}, {0, 0, -2.7f}, {5.6f, 4.2f, 0.15f});
			const ColorVolume leftWall("leftWall", {0.42f, 0.48f, 0.40f, 1.0f}, {-2.7f, 0, 0}, {0.15f, 4.2f, 5.6f});
			_append(builder, floor, context.viewProjectionMatrix, context.ambientRGB);
			_append(builder, backWall, context.viewProjectionMatrix, context.ambientRGB);
			_append(builder, leftWall, context.viewProjectionMatrix, context.ambientRGB);
		}

		static void _appendGlass(spk::RenderSnapshot::Builder &builder, const spk::EngineView::CaptureContext &context, int count)
		{
			const ColorVolume blue("blueGlass", {0.12f, 0.77f, 0.96f, 0.48f}, {-0.35f, -0.2f, 1.45f}, {1.8f, 3.5f, 0.06f});
			builder.renderPass(spk::EngineView::WBOITPassKey).emplace<RoomTransparentVolumeCommand>(blue, context.viewProjectionMatrix, context.ambientRGB);
			if (count > 1)
			{
				const ColorVolume amber("amberGlass", {0.96f, 0.30f, 0.10f, 0.35f}, {-0.45f, -0.08f, 1.9f}, {1.4f, 2.8f, 0.06f});
				builder.renderPass(spk::EngineView::WBOITPassKey).emplace<RoomTransparentVolumeCommand>(amber, context.viewProjectionMatrix, context.ambientRGB);
			}
		}

	public:
		explicit RoomColorContributor(Character &character) :
			_character(character)
		{
		}

		void setGlassCount(int count) noexcept
		{
			_glassCount = count;
		}

		void buildViewRenderSnapshot(
			spk::RenderSnapshot::Builder &builder,
			const spk::EngineView::CaptureContext &context) override
		{
			_appendRoom(builder, context);
			_appendCharacter(builder, _character, context.viewProjectionMatrix, context.ambientRGB);
			if (_glassCount > 0)
			{
				_appendGlass(builder, context, _glassCount);
			}
		}
	};

	struct RoomEngineScene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"Room camera"};
		spk::CameraComponent3D &camera;
		Character character{characterBlueprint()};
		spk::EngineView &view;
		RoomColorContributor &contributor;

		RoomEngineScene() :
			camera(cameraEntity.addComponent<spk::CameraComponent3D>()),
			view(_initialize()),
			contributor(engine.root().addBehaviour<RoomColorContributor>(character))
		{
		}

		spk::EngineView &_initialize()
		{
			const spk::Vector3 eye{3.0f, 2.0f, 7.2f};
			cameraEntity.transform().place(eye);
			cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, {0, -0.2f, 0}));
			camera.setPerspective(0.78f, static_cast<float>(RoomImageSize.x) / RoomImageSize.y, 0.1f, 30.0f);
			engine.addEntity(&cameraEntity);
			return engine.addView("room", &camera, RoomImageSize);
		}

		~RoomEngineScene()
		{
			engine.removeEntity(&cameraEntity);
		}
	};

	void prepareRoomWidget(spk::EngineWidget &widget, spk::Engine &engine, spk::Vector2UInt displaySize = RoomImageSize)
	{
		widget.setEngine(&engine);
		widget.setViewName("room");
		widget.setGeometry({.anchor = {0, 0}, .size = displaySize});
		widget.activate();
	}

	[[nodiscard]] spk::RenderSnapshot captureRoomWidget(spk::EngineWidget &widget)
	{
		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		return builder.build();
	}

	void expectRoomGolden(const char *name)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		const std::filesystem::path category(RoomGoldenCategory);
		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		const auto difference = sparkle_test::resultImagePath(category, std::string(name) + "_difference");
		gl.save(actual);
		ASSERT_TRUE(std::filesystem::exists(expected)) << "Missing reviewed golden: " << expected << "; rendered: " << actual;
		sparkle_test::ImageComparisonOptions tolerance;
		tolerance.rgbTolerance = 3;
		tolerance.alphaTolerance = 0;
		const auto result = sparkle_test::compareImages(actual, expected, difference, tolerance, false);
		EXPECT_EQ(result.actualWidth, result.expectedWidth);
		EXPECT_EQ(result.actualHeight, result.expectedHeight);
		EXPECT_LE(result.differentPixelCount, 50u) << "Room golden mismatch: " << name << "; diff: " << difference;
	}
}

TEST(EngineViewModel3DRoomGoldenTest, NeutralCharacterWallsAndWhiteAmbient)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(RoomImageSize);
	RoomEngineScene scene;
	scene.view.ambientLight().setIntensity(1.0f);
	spk::EngineWidget widget("room", nullptr);
	prepareRoomWidget(widget, scene.engine);
	captureRoomWidget(widget).execute(gl.renderContext());
	ASSERT_TRUE(scene.view.output()->latest(gl.surface()));
	EXPECT_EQ(gl.capture().pixel({160, 120})[3], 255);
	expectRoomGolden("neutral_white_ambient");
	gl.reset();
}

TEST(EngineViewModel3DRoomGoldenTest, RaisedArmWallsWarmAmbientAndExposure)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(RoomImageSize);
	RoomEngineScene scene;
	scene.character.attachment("torso", "leftShoulder").setRotation(spk::Vector3{0, 0, -95});
	scene.view.ambientLight().setColor({1.0f, 0.55f, 0.3f});
	scene.view.ambientLight().setIntensity(1.2f);
	scene.view.setExposure(1.6f);
	spk::EngineWidget widget("room", nullptr);
	prepareRoomWidget(widget, scene.engine);
	captureRoomWidget(widget).execute(gl.renderContext());
	EXPECT_EQ(gl.capture().pixel({160, 120})[3], 255);
	expectRoomGolden("raised_arm_warm_ambient");
	gl.reset();
}

TEST(EngineViewModel3DRoomGoldenTest, PublishedRoomIsReusableByDrawOnlyWidget)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(RoomImageSize);
	RoomEngineScene scene;
	scene.view.ambientLight().setIntensity(1.0f);
	spk::EngineWidget producer("producer", nullptr, spk::EngineWidget::Type::Render);
	prepareRoomWidget(producer, scene.engine);
	captureRoomWidget(producer).execute(gl.renderContext());
	const auto published = scene.engine.latestRenderSnapshot();
	ASSERT_TRUE(scene.view.output()->latest(gl.surface()));
	gl.reset(RoomImageSize);
	spk::EngineWidget consumer("consumer", nullptr, spk::EngineWidget::Type::Draw);
	prepareRoomWidget(consumer, scene.engine);
	captureRoomWidget(consumer).execute(gl.renderContext());
	EXPECT_EQ(scene.engine.latestRenderSnapshot(), published);
	EXPECT_EQ(gl.capture().pixel({160, 120})[3], 255);
	expectRoomGolden("neutral_white_ambient");
	gl.reset();
}

TEST(EngineViewModel3DRoomGoldenTest, CapturedAmbientAndExposureRemainImmutable)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(RoomImageSize);
	RoomEngineScene scene;
	scene.view.ambientLight().setIntensity(1.0f);
	spk::EngineWidget producer("producer", nullptr, spk::EngineWidget::Type::Render);
	prepareRoomWidget(producer, scene.engine);
	auto first = captureRoomWidget(producer);
	spk::EngineWidget consumer("consumer", nullptr, spk::EngineWidget::Type::Draw);
	prepareRoomWidget(consumer, scene.engine);
	scene.view.ambientLight().setColor({1.0f, 0.2f, 0.2f});
	scene.view.setExposure(2.0f);
	first.execute(gl.renderContext());
	captureRoomWidget(consumer).execute(gl.renderContext());
	const auto before = scene.view.output()->latest(gl.surface());
	ASSERT_TRUE(before);
	EXPECT_EQ(before->size(), RoomImageSize);
	const auto firstImage = gl.capture();
	gl.reset(RoomImageSize);
	captureRoomWidget(producer).execute(gl.renderContext());
	captureRoomWidget(consumer).execute(gl.renderContext());
	const auto secondImage = gl.capture();
	EXPECT_NE(firstImage.pixels, secondImage.pixels);
	gl.reset();
}

TEST(EngineViewModel3DRoomGoldenTest, ExistingRoomOutputScalesToLargerDestination)
{
	constexpr spk::Vector2UInt destination{640, 480};
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(RoomImageSize);
	RoomEngineScene scene;
	scene.view.ambientLight().setIntensity(1.0f);

	spk::EngineWidget producer("room-producer", nullptr, spk::EngineWidget::Type::Render);
	prepareRoomWidget(producer, scene.engine);
	captureRoomWidget(producer).execute(gl.renderContext());

	const auto originalOutput = scene.view.output()->latest(gl.surface());
	ASSERT_TRUE(originalOutput);
	EXPECT_EQ(originalOutput->size(), RoomImageSize);
	const auto publishedSnapshot = scene.engine.latestRenderSnapshot();

	gl.reset(destination);
	spk::EngineWidget display("room-large-destination", nullptr, spk::EngineWidget::Type::Draw);
	prepareRoomWidget(display, scene.engine, destination);
	captureRoomWidget(display).execute(gl.renderContext());

	EXPECT_EQ(scene.view.resolution(), RoomImageSize);
	EXPECT_EQ(scene.engine.latestRenderSnapshot(), publishedSnapshot);
	EXPECT_EQ(scene.view.output()->latest(gl.surface()).identifier(), originalOutput.identifier());
	const auto image = gl.capture();
	EXPECT_EQ(image.size, destination);
	EXPECT_EQ(image.pixel({320, 240})[3], 255);
	expectRoomGolden("neutral_white_ambient_large_destination");
	gl.reset();
}

TEST(EngineViewModel3DRoomGoldenTest, CyanTransparentBoxUsesWBOITWithOpaqueDepth)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(RoomImageSize);
	RoomEngineScene scene;
	scene.view.ambientLight().setIntensity(1.0f);
	spk::EngineWidget widget("glass", nullptr);
	prepareRoomWidget(widget, scene.engine);
	captureRoomWidget(widget).execute(gl.renderContext());
	const auto opaque = gl.capture();

	scene.contributor.setGlassCount(1);
	gl.reset(RoomImageSize);
	captureRoomWidget(widget).execute(gl.renderContext());
	const auto transparent = gl.capture();
	EXPECT_NE(opaque.pixels, transparent.pixels);
	EXPECT_EQ(transparent.pixel({160, 120})[3], 255);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	expectRoomGolden("cyan_glass_wboit");
	gl.reset();
}

TEST(EngineViewModel3DRoomGoldenTest, OverlappingTransparentBoxesComposeWithWarmAmbient)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(RoomImageSize);
	RoomEngineScene scene;
	scene.view.ambientLight().setColor({1.0f, 0.80f, 0.58f});
	scene.view.ambientLight().setIntensity(1.2f);
	scene.view.setExposure(1.4f);
	scene.character.attachment("torso", "leftShoulder").setRotation(spk::Vector3{0, 0, -95});
	scene.contributor.setGlassCount(2);
	spk::EngineWidget widget("glass-two", nullptr);
	prepareRoomWidget(widget, scene.engine);
	captureRoomWidget(widget).execute(gl.renderContext());
	EXPECT_EQ(gl.capture().pixel({160, 120})[3], 255);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	expectRoomGolden("overlapping_glass_warm_ambient");
	gl.reset();
}
