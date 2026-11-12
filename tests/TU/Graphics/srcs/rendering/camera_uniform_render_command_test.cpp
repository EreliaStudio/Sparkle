#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cstddef>

#include "rendering/command/camera_uniform_render_command.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	[[nodiscard]] spk::CameraData makeCamera(float x)
	{
		spk::CameraData data{};
		data.projection = spk::Matrix4x4::perspective(1.4f, 1.5f, 0.1f, 100.0f);
		data.view = spk::Matrix4x4::translation({-x, 2.0f, -4.0f});
		data.inverseView = spk::Matrix4x4::translation({x, -2.0f, 4.0f});
		data.viewProjection = data.projection * data.view;
		return data;
	}

	[[nodiscard]] GLint boundBuffer(std::size_t bindingPoint)
	{
		GLint buffer = 0;
		glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, static_cast<GLuint>(bindingPoint), &buffer);
		return buffer;
	}

	[[nodiscard]] spk::CameraData readCamera()
	{
		const auto buffer = boundBuffer(spk::CameraUniformRenderCommand::BindingPoint);
		EXPECT_NE(buffer, 0);
		spk::CameraData result{};
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

	void expectCamera(const spk::CameraData &actual, const spk::CameraData &expected)
	{
		expectMatrix(actual.projection, expected.projection);
		expectMatrix(actual.view, expected.view);
		expectMatrix(actual.inverseView, expected.inverseView);
		expectMatrix(actual.viewProjection, expected.viewProjection);
	}
}

TEST(CameraUniformRenderCommandTest, UploadsFourCapturedMatricesToDedicatedBinding)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	const auto expected = makeCamera(3.0f);
	const spk::CameraUniformRenderCommand command(expected);
	command.execute(gl.renderContext());
	expectCamera(readCamera(), expected);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(CameraUniformRenderCommandTest, SeparateSnapshotsRestoreTheirOwnCameraData)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	const auto firstData = makeCamera(3.0f);
	const auto secondData = makeCamera(9.0f);
	const spk::CameraUniformRenderCommand first(firstData);
	const spk::CameraUniformRenderCommand second(secondData);

	first.execute(gl.renderContext());
	const auto firstBuffer = boundBuffer(spk::CameraUniformRenderCommand::BindingPoint);
	expectCamera(readCamera(), firstData);

	second.execute(gl.renderContext());
	const auto secondBuffer = boundBuffer(spk::CameraUniformRenderCommand::BindingPoint);
	EXPECT_NE(secondBuffer, firstBuffer);
	expectCamera(readCamera(), secondData);

	first.execute(gl.renderContext());
	EXPECT_EQ(boundBuffer(spk::CameraUniformRenderCommand::BindingPoint), firstBuffer);
	expectCamera(readCamera(), firstData);
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

	const spk::CameraUniformRenderCommand camera(makeCamera(4.0f));
	camera.execute(gl.renderContext());
	EXPECT_EQ(boundBuffer(spk::ViewportUniformRenderCommand::MatrixUBOBindingPoint), viewportBuffer);
	expectCamera(readCamera(), makeCamera(4.0f));
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}
