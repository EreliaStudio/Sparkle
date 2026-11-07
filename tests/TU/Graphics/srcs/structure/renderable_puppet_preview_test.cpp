#include <GL/glew.h>
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

#include "geometry/texture_mesh_3d.hpp"
#include "graphics/color.hpp"
#include "graphics/opengl/program.hpp"
#include "graphics/opengl/uniform.hpp"
#include "graphics/renderable_puppet.hpp"
#include "math/camera_3d.hpp"
#include "rendering/command/clear_render_command.hpp"
#include "rendering/command/viewport_render_command.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"

namespace
{
	using Blueprint = spk::Assembly::Blueprint;

	constexpr spk::Vector2UInt ImageSize{640, 480};
	constexpr spk::Color Background{0.06f, 0.09f, 0.14f, 1.0f};
	constexpr const char *GoldenCategory = "rendering/puppet_3d";

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
			builder.renderPass({"puppet-3d", 0}).emplace<DrawColorVolumeCommand>(_mesh, transform, _color);
		}
	};

	static_assert(spk::Assembly::PuppetGraphics<ColorView>);
	using Character = spk::Assembly::RenderablePuppet<ColorView>;

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
		rightArm.attach<ColorVolume>("wrist", "rightHand", spk::Color{0.98f, 0.79f, 0.54f, 1}, spk::Vector3{0, -0.15f, 0}, spk::Vector3{0.25f, 0.30f, 0.27f});
		torso.addAttachmentPoint("leftHip", {-0.25f, -0.72f, 0});
		torso.addAttachmentPoint("rightHip", {0.25f, -0.72f, 0});
		torso.attach<ColorVolume>("leftHip", "leftLeg", spk::Color{0.34f, 0.49f, 0.81f, 1}, spk::Vector3{0, -0.56f, 0}, spk::Vector3{0.34f, 1.11f, 0.40f});
		torso.attach<ColorVolume>("rightHip", "rightLeg", spk::Color{0.34f, 0.49f, 0.81f, 1}, spk::Vector3{0, -0.56f, 0}, spk::Vector3{0.34f, 1.11f, 0.40f});
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

	[[nodiscard]] spk::Camera3D previewCamera()
	{
		spk::Camera3D camera;
		camera.setPosition({3.0f, 2.0f, 7.2f});
		camera.lookAt({0, -0.2f, 0});
		camera.setPerspective(0.78f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 30.0f);
		return camera;
	}

	[[nodiscard]] sparkle_test::FramebufferImage capture(Character &character, const spk::Camera3D &camera, const std::string &name)
	{
		auto &context = sparkle_test::OpenGLTestContext::instance();
		context.reset(ImageSize);
		spk::RenderSnapshot::Builder builder;
		auto &pass = builder.renderPass({"puppet-3d", 0});
		pass.emplace<spk::ClearRenderCommand>(Background, spk::ClearRenderCommand::Mask::All);
		pass.emplace<spk::ViewportRenderCommand>(spk::Rect2D{.anchor = {0, 0}, .size = ImageSize});
		character.buildRenderSnapshot(builder, camera.viewProjectionMatrix());
		builder.build().execute(context.renderContext());
		context.save(sparkle_test::resultImagePath(GoldenCategory, name));
		return context.capture();
	}

	[[nodiscard]] spk::Vector4 pointOn(const Character &character, const char *part, spk::Vector3 local = {})
	{
		return character.element(part).globalTransform() * spk::Vector4{local.x, local.y, local.z, 1.0f};
	}

	void expectJointChain(const Character &character, const char *parent, const char *joint, const char *child)
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

class RenderablePuppet3DGoldenTest : public ::testing::TestWithParam<Pose>
{
};

TEST_P(RenderablePuppet3DGoldenTest, ArticulatedCharacterMatchesReference)
{
	const Pose pose = GetParam();
	Character character(characterBlueprint());
	poseCharacter(character, pose);
	const auto name = poseName(pose);
	const auto actual = sparkle_test::resultImagePath(GoldenCategory, name);
	const auto expected = sparkle_test::expectedImagePath(GoldenCategory, name);
	const auto difference = sparkle_test::resultImagePath(GoldenCategory, name + "_difference");
	const auto image = capture(character, previewCamera(), name);
	EXPECT_EQ(character.graphicsCount(), 8u);
	EXPECT_EQ(image.size.x, ImageSize.x);
	EXPECT_EQ(image.size.y, ImageSize.y);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	ASSERT_TRUE(std::filesystem::exists(expected)) << "Missing approved golden: " << expected << "; rendered: " << actual;

	sparkle_test::ImageComparisonOptions tolerance;
	tolerance.rgbTolerance = 6;
	tolerance.alphaTolerance = 8;
	const auto comparison = sparkle_test::compareImages(actual, expected, difference, tolerance, false);
	EXPECT_EQ(comparison.actualWidth, comparison.expectedWidth);
	EXPECT_EQ(comparison.actualHeight, comparison.expectedHeight);
	EXPECT_LE(comparison.differentPixelCount, 150u) << "Golden mismatch: " << name << "; diff: " << difference;
}

TEST(RenderablePuppet3DGoldenTest, ArticulationsMoveBothHandsAndLegsThroughParentTransforms)
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

TEST(RenderablePuppet3DGoldenTest, WalkingPoseSwingsLegsForwardAndBackwardWithoutLateralCrossing)
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
	RenderablePuppet3DGoldenTest,
	::testing::Values(Pose::Neutral, Pose::OneArmRaised, Pose::BothArmsRaised, Pose::Walking, Pose::Combined),
	[](const ::testing::TestParamInfo<Pose> &info) {
		return poseName(info.param);
	});
