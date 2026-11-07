#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "geometry/color_mesh_2d.hpp"
#include "graphics/color.hpp"
#include "graphics/renderable_puppet.hpp"
#include "math/camera_3d.hpp"
#include "rendering/command/clear_render_command.hpp"
#include "rendering/command/draw_color_mesh_render_command.hpp"
#include "rendering/command/viewport_render_command.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"

namespace
{
	using Blueprint = spk::Assembly::Blueprint;
	using ElementNode = spk::Assembly::Puppet::ElementNode;

	class ColorElement final : public Blueprint::Element
	{
	public:
		spk::Color color;
		spk::Vector2 minimum;
		spk::Vector2 maximum;

		ColorElement(std::string name, spk::Color fill, spk::Vector2 min, spk::Vector2 max) :
			Element(std::move(name)),
			color(fill),
			minimum(min),
			maximum(max)
		{
		}
	};

	class ColorElementView final
	{
	public:
		using ElementType = ColorElement;

	private:
		spk::Color _color;
		spk::Vector2 _min;
		spk::Vector2 _max;

	public:
		explicit ColorElementView(const ColorElement &element) :
			_color(element.color),
			_min(element.minimum),
			_max(element.maximum)
		{
		}

		void append(const ElementNode &node, const spk::Camera3D &camera, spk::RenderPass &pass) const;
	};

	using Character = spk::Assembly::RenderablePuppet<ColorElementView>;
	constexpr spk::Vector2UInt ImageSize{640, 480};

	[[nodiscard]] spk::ColorVertex2D project(
		const spk::Matrix4x4 &transform,
		spk::Vector2 local,
		spk::Color color)
	{
		const auto clip = transform * spk::Vector4{local.x, local.y, 0.0f, 1.0f};
		const spk::Vector2 screen{
			(clip.x / clip.w + 1.0f) * ImageSize.x * 0.5f,
			(1.0f - clip.y / clip.w) * ImageSize.y * 0.5f};
		return {screen, 0.0f, color};
	}

	void ColorElementView::append(
		const ElementNode &node, const spk::Camera3D &camera, spk::RenderPass &pass) const
	{
		const auto transform = camera.viewProjectionMatrix() * node.globalTransform();
		spk::ColorMesh2D::Builder mesh;
		mesh.addShape(
			project(transform, {_min.x, _max.y}, _color),
			project(transform, {_min.x, _min.y}, _color),
			project(transform, {_max.x, _min.y}, _color),
			project(transform, {_max.x, _max.y}, _color));
		pass.emplace<spk::DrawColorMeshRenderCommand>(std::move(mesh).build());
	}

	[[nodiscard]] Blueprint characterBlueprint()
	{
		Blueprint::Builder builder;
		auto &torso = builder.setRoot<ColorElement>("torso", spk::Color{0.16f, 0.45f, 0.76f, 1}, spk::Vector2{-0.58f, -0.64f}, spk::Vector2{0.58f, 0.64f});
		torso.addAttachmentPoint("shoulder", {0.67f, 0.49f, 0.0f});
		auto &arm = torso.attach<ColorElement>("shoulder", "arm", spk::Color{0.96f, 0.43f, 0.24f, 1}, spk::Vector2{-0.15f, -0.9f}, spk::Vector2{0.15f, 0.05f});
		arm.addAttachmentPoint("wrist", {0, -0.92f, 0});
		arm.attach<ColorElement>("wrist", "hand", spk::Color{0.98f, 0.78f, 0.54f, 1}, spk::Vector2{-0.18f, -0.33f}, spk::Vector2{0.18f, 0.02f});
		torso.addAttachmentPoint("otherShoulder", {-0.67f, 0.49f, 0});
		torso.attach<ColorElement>("otherShoulder", "otherArm", spk::Color{0.34f, 0.72f, 0.57f, 1}, spk::Vector2{-0.15f, -0.98f}, spk::Vector2{0.15f, 0.05f});
		torso.addAttachmentPoint("neck", {0, 0.75f, 0});
		torso.attach<ColorElement>("neck", "head", spk::Color{0.98f, 0.77f, 0.46f, 1}, spk::Vector2{-0.4f, 0}, spk::Vector2{0.4f, 0.7f});
		return builder.build();
	}

	[[nodiscard]] spk::Camera3D previewCamera()
	{
		spk::Camera3D camera;
		camera.setPosition({0, 0, 5});
		camera.setOrthographic(-2.6f, 2.6f, -2.35f, 2.0f, 0.1f, 20.0f);
		return camera;
	}

	[[nodiscard]] sparkle_test::FramebufferImage render(
		Character &character, const spk::Camera3D &camera, const std::string &pose)
	{
		auto &context = sparkle_test::OpenGLTestContext::instance();
		context.reset(ImageSize);
		const spk::Rect2D viewport{.anchor = {0, 0}, .size = ImageSize};
		spk::RenderSnapshot::Builder builder;
		auto &pass = builder.renderPass({"puppet-preview", 0});
		pass.emplace<spk::ClearRenderCommand>(spk::Color{0.07f, 0.10f, 0.15f, 1}, spk::ClearRenderCommand::Mask::All);
		pass.emplace<spk::ViewportRenderCommand>(viewport);
		pass.emplace<spk::ViewportUniformRenderCommand>(viewport);
		character.visitGraphics([&](const ElementNode &node, ColorElementView &view) {
			view.append(node, camera, pass);
		});
		builder.build().execute(context.renderContext());
		context.save(sparkle_test::resultImagePath("rendering/puppet", pose));
		return context.capture();
	}

	[[nodiscard]] std::size_t orangePixels(const sparkle_test::FramebufferImage &image)
	{
		std::size_t count = 0;
		for (std::uint32_t y = 0; y < image.size.y; ++y)
		{
			for (std::uint32_t x = 0; x < image.size.x; ++x)
			{
				const auto *pixel = image.pixel({x, y});
				count += pixel[0] > 200 && pixel[1] > 70 && pixel[1] < 175 && pixel[2] < 110;
			}
		}
		return count;
	}
}

TEST(RenderablePuppetPreviewTest, ArmAndHandFollowShoulderAcrossThreeRenderedPoses)
{
	Character character(characterBlueprint());
	const auto camera = previewCamera();
	const auto resting = character.element("hand").globalTransform() * spk::Vector4{0, 0, 0, 1};
	const auto image0 = render(character, camera, "arm_000");
	EXPECT_GT(orangePixels(image0), 100u);

	character.attachment("torso", "shoulder").setRotation(spk::Vector3{0, 0, 45});
	const auto image45 = render(character, camera, "arm_045");
	EXPECT_NE(image0.pixels, image45.pixels);

	character.attachment("torso", "shoulder").setRotation(spk::Vector3{0, 0, 90});
	const auto raised = character.element("hand").globalTransform() * spk::Vector4{0, 0, 0, 1};
	const auto image90 = render(character, camera, "arm_090");
	EXPECT_NE(image45.pixels, image90.pixels);
	EXPECT_GT(raised.x, resting.x + 0.5f);
	EXPECT_GT(raised.y, resting.y + 0.5f);
	EXPECT_EQ(character.graphicsCount(), 5u);
}
