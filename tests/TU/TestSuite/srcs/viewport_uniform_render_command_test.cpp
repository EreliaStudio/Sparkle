#include <gtest/gtest.h>

#include <GL/glew.h>

#include <stdexcept>

#include "rendering/command/viewport_uniform_render_command.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

static_assert(spk::ViewportUniformRenderCommand::MatrixUBOBindingPoint == 0);

namespace
{
	[[nodiscard]] spk::Rect2D makeRect(spk::Vector2Int anchor, spk::Vector2UInt size)
	{
		return spk::Rect2D{.anchor = anchor, .size = size};
	}

	struct BoundMatrix
	{
		GLint buffer = 0;
		spk::Matrix4x4 matrix;
	};

	[[nodiscard]] BoundMatrix boundMatrix()
	{
		BoundMatrix result;
		glGetIntegeri_v(
			GL_UNIFORM_BUFFER_BINDING,
			spk::ViewportUniformRenderCommand::MatrixUBOBindingPoint,
			&result.buffer);
		if (result.buffer != 0)
		{
			glBindBuffer(GL_UNIFORM_BUFFER, static_cast<GLuint>(result.buffer));
			glGetBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(result.matrix), &result.matrix);
		}
		return result;
	}

	void expectMatrixEqual(const spk::Matrix4x4 &actual, const spk::Matrix4x4 &expected)
	{
		for (std::size_t column = 0; column < 4; ++column)
			for (std::size_t row = 0; row < 4; ++row)
				EXPECT_FLOAT_EQ(actual[column][row], expected[column][row]);
	}
}

TEST(ViewportUniformRenderCommandTest, RejectsZeroWidthViewport)
{
	EXPECT_THROW(
		(void)spk::ViewportUniformRenderCommand(makeRect({17, 29}, {0, 240})),
		std::invalid_argument);
}

TEST(ViewportUniformRenderCommandTest, RejectsZeroHeightViewport)
{
	EXPECT_THROW(
		(void)spk::ViewportUniformRenderCommand(makeRect({17, 29}, {320, 0})),
		std::invalid_argument);
}

TEST(ViewportUniformRenderCommandTest, RejectsViewportWithBothDimensionsEmpty)
{
	EXPECT_THROW(
		(void)spk::ViewportUniformRenderCommand(makeRect({17, 29}, {0, 0})),
		std::invalid_argument);
}

TEST(ViewportUniformRenderCommandTest, OffsetViewportBuildsAndBindsProjectionMatrix)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::ViewportUniformRenderCommand(makeRect({17, 29}, {320, 240})).execute(openGL.renderContext());
	const auto actual = boundMatrix();
	ASSERT_NE(actual.buffer, 0);
	const auto expected = spk::Matrix4x4::ortho(0.0f, 320.0f, 240.0f, 0.0f);
	expectMatrixEqual(actual.matrix, expected);
	EXPECT_EQ((actual.matrix * spk::Vector4{0, 0, 0, 1}), (spk::Vector4{-1, 1, 0, 1}));
	EXPECT_EQ((actual.matrix * spk::Vector4{320, 240, 0, 1}), (spk::Vector4{1, -1, 0, 1}));
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(ViewportUniformRenderCommandTest, RepeatedExecutionKeepsProjectionAndBindingStable)
{
	auto &openGL = sparkle_test::OpenGLTestContext::instance();
	openGL.reset();
	spk::ViewportUniformRenderCommand command(makeRect({41, -13}, {512, 288}));
	command.execute(openGL.renderContext());
	const auto first = boundMatrix();
	spk::ViewportUniformRenderCommand(makeRect({0, 0}, {64, 64})).execute(openGL.renderContext());
	command.execute(openGL.renderContext());
	const auto second = boundMatrix();
	ASSERT_NE(first.buffer, 0);
	EXPECT_EQ(second.buffer, first.buffer);
	expectMatrixEqual(second.matrix, first.matrix);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}
