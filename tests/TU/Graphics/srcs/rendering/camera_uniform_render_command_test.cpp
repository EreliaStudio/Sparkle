#include <gtest/gtest.h>

#include <GL/glew.h>

#include <cstddef>

#include "engine/reference_frame.hpp"
#include "engine/transform3d.hpp"
#include "math/camera_3d.hpp"
#include "rendering/command/camera_uniform_render_command.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	struct CameraReadback
	{
		spk::Matrix4x4 projection;
		spk::Matrix4x4 view;
		spk::Matrix4x4 inverseView;
		spk::Matrix4x4 viewProjection;
	};

	[[nodiscard]] spk::Camera3D makeCamera(float x)
	{
		spk::Camera3D camera;
		camera.setPerspective(1.4f, 1.5f, 0.1f, 100.0f);
		camera.setPosition({x, -2.0f, 4.0f});
		return camera;
	}

	[[nodiscard]] GLint boundBuffer(std::size_t bindingPoint)
	{
		GLint buffer = 0;
		glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, static_cast<GLuint>(bindingPoint), &buffer);
		return buffer;
	}

	[[nodiscard]] CameraReadback readCamera()
	{
		const auto buffer = boundBuffer(spk::CameraUniformRenderCommand::BindingPoint);
		EXPECT_NE(buffer, 0);
		CameraReadback result{};
		glBindBuffer(GL_UNIFORM_BUFFER, static_cast<GLuint>(buffer));
		glGetBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(result), &result);
		return result;
	}

	void expectMatrix(const spk::Matrix4x4 &actual, const spk::Matrix4x4 &expected)
	{
		for (std::size_t column = 0; column < 4; ++column)
		{
			for (std::size_t row = 0; row < 4; ++row)
			{
				EXPECT_FLOAT_EQ(actual[column][row], expected[column][row]);
			}
		}
	}

	void expectCamera(const CameraReadback &actual, const spk::Camera3D &camera)
	{
		expectMatrix(actual.projection, camera.projectionMatrix());
		expectMatrix(actual.view, camera.viewMatrix());
		expectMatrix(actual.inverseView, spk::Matrix4x4::translation(camera.position()) * spk::Matrix4x4::rotation(camera.rotation()));
		expectMatrix(actual.viewProjection, camera.viewProjectionMatrix());
	}
}

TEST(CameraUniformRenderCommandTest, UploadsFourCameraMatricesToDedicatedBinding)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	const auto camera = makeCamera(3.0f);
	const spk::CameraUniformRenderCommand command(camera);
	command.execute(gl.renderContext());
	expectCamera(readCamera(), camera);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(CameraUniformRenderCommandTest, SeparateSnapshotsRestoreTheirOwnCameraData)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	auto firstCamera = makeCamera(3.0f);
	const auto firstPose = firstCamera;
	const auto secondCamera = makeCamera(9.0f);
	const spk::CameraUniformRenderCommand first(firstCamera);
	const spk::CameraUniformRenderCommand second(secondCamera);
	firstCamera.setPosition({19.0f, 0.0f, 0.0f});

	first.execute(gl.renderContext());
	const auto firstBuffer = boundBuffer(spk::CameraUniformRenderCommand::BindingPoint);
	expectCamera(readCamera(), firstPose);
	second.execute(gl.renderContext());
	const auto secondBuffer = boundBuffer(spk::CameraUniformRenderCommand::BindingPoint);
	EXPECT_NE(secondBuffer, firstBuffer);
	expectCamera(readCamera(), secondCamera);

	first.execute(gl.renderContext());
	EXPECT_EQ(boundBuffer(spk::CameraUniformRenderCommand::BindingPoint), firstBuffer);
	expectCamera(readCamera(), firstPose);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(CameraUniformRenderCommandTest, TransformOverridesCameraPoseAtCapture)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	auto camera = makeCamera(40.0f);
	spk::Transform3D transform("camera transform");
	transform.place({3.0f, 5.0f, -2.0f});
	transform.setEulerRotation({0.0f, 45.0f, 0.0f});
	auto expected = camera;
	expected.setPosition(transform.position(spk::ReferenceFrame::World));
	expected.setRotation(transform.rotation(spk::ReferenceFrame::World));

	const spk::CameraUniformRenderCommand command(camera, transform);
	camera.setPerspective(0.9f, 1.0f, 0.2f, 50.0f);
	transform.move({5.0f, 0.0f, 0.0f});
	command.execute(gl.renderContext());
	expectCamera(readCamera(), expected);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(CameraUniformRenderCommandTest, DoesNotOverwriteViewportUniformBinding)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	const auto rect = spk::Rect2D{.anchor = {0, 0}, .size = {320, 240}};
	spk::ViewportUniformRenderCommand(rect).execute(gl.renderContext());
	const auto viewportBuffer = boundBuffer(spk::ViewportUniformRenderCommand::MatrixUBOBindingPoint);
	ASSERT_NE(viewportBuffer, 0);

	const auto camera = makeCamera(4.0f);
	const spk::CameraUniformRenderCommand command(camera);
	command.execute(gl.renderContext());
	EXPECT_EQ(boundBuffer(spk::ViewportUniformRenderCommand::MatrixUBOBindingPoint), viewportBuffer);
	expectCamera(readCamera(), camera);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}
