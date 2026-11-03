#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include <GL/glew.h>

#include "graphics/opengl/layout_buffer.hpp"
#include "graphics/opengl/program.hpp"
#include "math/camera_3d.hpp"
#include "model.hpp"
#include "rendering/command/clear_render_command.hpp"
#include "rendering/render_command.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget.hpp"

namespace
{
	struct BoxBody final : spk::Body
	{
		spk::Vector3 center{};
		spk::Vector3 halfSize{};
		spk::Vector3 color{};
	};

	struct Vertex
	{
		spk::Vector3 position;
		spk::Vector3 color;
	};

	[[nodiscard]] std::string shaderSource(const std::string &filename)
	{
		const auto path = std::filesystem::path(SPARKLE_TEST_RESOURCES_DIR) / "shaders" / filename;
		std::ifstream file(path);
		if (!file)
		{
			throw std::runtime_error("Cannot read model golden-image shader: " + path.string());
		}
		return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
	}

	class FlatModelCommand final : public spk::RenderCommand
	{
	private:
		spk::LayoutBuffer _mesh;
		spk::Matrix4x4 _viewProjection;
		std::size_t _indexCount = 0;

		[[nodiscard]] static spk::Program &_program()
		{
			static spk::Program program(
				shaderSource("flat_color_3d.vert.glsl"),
				shaderSource("flat_color_3d.frag.glsl"));
			return program;
		}

	public:
		FlatModelCommand(const std::vector<Vertex> &vertices, const std::vector<std::uint32_t> &indices, spk::Matrix4x4 viewProjection) :
			_viewProjection(viewProjection),
			_indexCount(indices.size())
		{
			auto &verticesBuffer = _mesh.vertexBuffer();
			verticesBuffer.addAttribute(0, spk::VertexBuffer::Attribute::Type::Float, 3);
			verticesBuffer.addAttribute(1, spk::VertexBuffer::Attribute::Type::Float, 3);
			verticesBuffer.insert<Vertex>(std::span<const Vertex>(vertices));
			auto &indicesBuffer = _mesh.indexBuffer();
			indicesBuffer.setType(spk::IndexBuffer::Type::UnsignedInt);
			indicesBuffer.insert<std::uint32_t>(std::span<const std::uint32_t>(indices));
			_mesh.validate();
		}

		void execute(spk::RenderContext &context) const override
		{
			auto &program = _program();
			program.activate(context);
			GLint nativeProgram = 0;
			::glGetIntegerv(GL_CURRENT_PROGRAM, &nativeProgram);
			const GLint uniform = ::glGetUniformLocation(static_cast<GLuint>(nativeProgram), "uViewProjection");
			::glUniformMatrix4fv(uniform, 1, GL_FALSE, &_viewProjection[0][0]);
			_mesh.activate(context);
			program.render(spk::Program::Primitive::Triangles, spk::IndexBuffer::Type::UnsignedInt, 0, _indexCount);
		}
	};

	class ModelPreviewWidget final : public spk::Widget
	{
	private:
		const spk::Model *_model;
		const spk::Camera3D *_camera;

		void _addBox(const spk::Skeleton::Part &part, const BoxBody &body, std::vector<Vertex> &vertices, std::vector<std::uint32_t> &indices) const
		{
			const auto transform = part.globalMatrix() *
								   spk::Matrix4x4::translation(body.center) *
								   spk::Matrix4x4::scale(body.halfSize);
			const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
			const std::array<spk::Vector3, 8> corners{spk::Vector3{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}, {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}};
			for (const auto &corner : corners)
			{
				const auto position = transform * spk::Vector4{corner.x, corner.y, corner.z, 1};
				vertices.push_back({{position.x, position.y, position.z}, body.color});
			}
			constexpr std::array<std::uint32_t, 36> faces{0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 1, 5, 6, 1, 6, 2, 4, 0, 3, 4, 3, 7, 3, 2, 6, 3, 6, 7, 4, 5, 1, 4, 1, 0};
			for (const auto index : faces)
			{
				indices.push_back(base + index);
			}
		}

		void _collect(const spk::Skeleton::Part &part, std::vector<Vertex> &vertices, std::vector<std::uint32_t> &indices) const
		{
			if (const auto *body = dynamic_cast<const BoxBody *>(part.body()))
			{
				_addBox(part, *body, vertices, indices);
			}
			for (const auto &[name, joint] : part.joints())
			{
				if (const auto *child = joint.child())
				{
					_collect(*child, vertices, indices);
				}
				if (const auto *model = joint.model())
				{
					_collect(model->skeleton().root(), vertices, indices);
				}
			}
		}

		void _buildRenderSnapshot(spk::RenderSnapshot::Builder &builder) override
		{
			std::vector<Vertex> vertices;
			std::vector<std::uint32_t> indices;
			_collect(_model->skeleton().root(), vertices, indices);
			auto &pass = builder.renderPass(targetRenderPass());
			pass.emplace<spk::ClearRenderCommand>(spk::Color{0, 0, 0, 1}, spk::ClearRenderCommand::Mask::All);
			pass.emplace<FlatModelCommand>(vertices, indices, _camera->viewProjectionMatrix());
		}

	public:
		ModelPreviewWidget(const spk::Model &model, const spk::Camera3D &camera) :
			spk::Widget("ModelPreview", nullptr),
			_model(&model),
			_camera(&camera)
		{
			setGeometry({.anchor = {0, 0}, .size = {320, 320}});
			activate();
		}
	};

	struct Figure
	{
		std::array<BoxBody, 6> bodies;
		spk::Model model;

		Figure()
		{
			const auto assign = [](BoxBody &body, spk::Vector3 center, spk::Vector3 halfSize, spk::Vector3 color) {
				body.center = center;
				body.halfSize = halfSize;
				body.color = color;
			};
			assign(bodies[0], {0, 0.125f, 0}, {0.375f, 0.625f, 0.125f}, {0.25f, 0.625f, 1});
			assign(bodies[1], {0, 0.25f, 0}, {0.25f, 0.25f, 0.125f}, {1, 0.75f, 0.25f});
			assign(bodies[2], {-0.5f, 0, 0}, {0.5f, 0.125f, 0.125f}, {1, 0.25f, 0.375f});
			assign(bodies[3], {0.5f, 0, 0}, {0.5f, 0.125f, 0.125f}, {0.25f, 1, 0.5f});
			assign(bodies[4], {0, -0.625f, 0}, {0.1875f, 0.625f, 0.125f}, {0.375f, 0.5f, 1});
			assign(bodies[5], {0, -0.625f, 0}, {0.1875f, 0.625f, 0.125f}, {0.875f, 0.375f, 1});
			auto &root = model.skeleton().root();
			root.setBody(&bodies[0]);
			root.addJoint("head", spk::Matrix4x4::translation(0, 0.75f, 0));
			root.addJoint("left_arm", spk::Matrix4x4::translation(-0.375f, 0.625f, 0));
			root.addJoint("right_arm", spk::Matrix4x4::translation(0.375f, 0.625f, 0));
			root.addJoint("left_leg", spk::Matrix4x4::translation(-0.25f, -0.5f, 0));
			root.addJoint("right_leg", spk::Matrix4x4::translation(0.25f, -0.5f, 0));
			root.joint("head").attach(spk::Skeleton::Part(&bodies[1]));
			root.joint("left_arm").attach(spk::Skeleton::Part(&bodies[2]));
			root.joint("right_arm").attach(spk::Skeleton::Part(&bodies[3]));
			root.joint("left_leg").attach(spk::Skeleton::Part(&bodies[4]));
			root.joint("right_leg").attach(spk::Skeleton::Part(&bodies[5]));
		}
	};

	void expectGolden(const Figure &figure, const std::string &name)
	{
		spk::Camera3D camera;
		camera.setPosition({3.5f, 2.5f, 6.0f});
		camera.lookAt({0, 0, 0});
		camera.setOrthographic(-2.2f, 2.2f, -2.2f, 2.2f, 0.1f, 20.0f);
		ASSERT_EQ(camera.projectionMode(), spk::Camera3D::ProjectionMode::Orthographic);
		ModelPreviewWidget widget(figure.model, camera);
		auto &context = sparkle_test::OpenGLTestContext::instance();
		context.reset({320, 320});
		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		builder.build().execute(context.renderContext());
		const auto category = std::filesystem::path("model/poses");
		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		const auto difference = sparkle_test::resultImagePath(category, name + "_difference");
		context.save(actual);
		ASSERT_TRUE(std::filesystem::exists(expected)) << "Missing golden image: " << expected << "; actual: " << actual;
		const auto comparison = sparkle_test::compareImages(actual, expected, difference);
		EXPECT_LE(comparison.differentPixelCount, 48u)
			<< "Pose [" << name << "], actual: " << actual << ", expected: " << expected
			<< ", difference: " << difference;
	}
}

TEST(ModelGoldenImageTest, TPose)
{
	Figure figure;
	expectGolden(figure, "t_pose");
}

TEST(ModelGoldenImageTest, LeftArmRaised)
{
	Figure figure;
	figure.model.skeleton().joint("left_arm").setRotation(spk::Quaternion::fromEuler({0, 0, -90}));
	expectGolden(figure, "left_arm_raised");
}

TEST(ModelGoldenImageTest, RightArmLowered)
{
	Figure figure;
	figure.model.skeleton().joint("right_arm").setRotation(spk::Quaternion::fromEuler({0, 0, -90}));
	expectGolden(figure, "right_arm_lowered");
}

TEST(ModelGoldenImageTest, LeftLegExtended)
{
	Figure figure;
	figure.model.skeleton().joint("left_leg").setRotation(spk::Quaternion::fromEuler({0, 0, -90}));
	expectGolden(figure, "left_leg_extended");
}

TEST(ModelGoldenImageTest, CombinedPose)
{
	Figure figure;
	figure.model.skeleton().joint("left_arm").setRotation(spk::Quaternion::fromEuler({0, 0, -90}));
	figure.model.skeleton().joint("right_arm").setRotation(spk::Quaternion::fromEuler({0, 0, -90}));
	figure.model.skeleton().joint("left_leg").setRotation(spk::Quaternion::fromEuler({0, 0, -90}));
	expectGolden(figure, "combined_pose");
}

TEST(ModelGoldenImageTest, TranslatedModel)
{
	Figure figure;
	figure.model.setTransform(spk::Matrix4x4::translation(0.25f, 0, 0));
	expectGolden(figure, "translated_model");
}
