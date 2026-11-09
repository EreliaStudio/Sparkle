#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <vector>

#include "graphics/opengl/framebuffer.hpp"
#include "rendering/command/clear_render_command.hpp"
#include "rendering/command/render_to_framebuffer_command.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace
{
	class InspectFramebufferCommand final : public spk::RenderCommand
	{
	private:
		std::vector<std::array<GLint, 5>> *_states;

	public:
		explicit InspectFramebufferCommand(std::vector<std::array<GLint, 5>> &states) :
			_states(&states)
		{
		}

		void execute(spk::RenderContext &) const override
		{
			std::array<GLint, 5> state{};
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, state.data());
			glGetIntegerv(GL_VIEWPORT, state.data() + 1);
			_states->push_back(state);
		}
	};

	class FailingFramebufferCommand final : public spk::RenderCommand
	{
	public:
		void execute(spk::RenderContext &) const override
		{
			throw std::runtime_error("nested framebuffer render failed");
		}
	};

	[[nodiscard]] std::shared_ptr<const spk::RenderSnapshot> makeClearSnapshot(spk::Color color)
	{
		spk::RenderSnapshot::Builder builder;
		builder.renderPass({"clear", 0}).emplace<spk::ClearRenderCommand>(
			color,
			spk::ClearRenderCommand::Mask::Color);
		return std::make_shared<spk::RenderSnapshot>(builder.build());
	}
}

TEST(RenderToFramebufferCommandTest, RenderPassClearsTargetAndRestoresReadDrawAndViewport)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	spk::Framebuffer target({4, 3}, {spk::Texture::Format::RGBA}, std::nullopt);
	spk::Framebuffer previous({2, 2}, {spk::Texture::Format::RGBA}, std::nullopt);
	previous.activate(gl.renderContext());
	GLint previousName = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousName);
	ASSERT_GT(previousName, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(2, 3, 17, 19);

	std::vector<std::array<GLint, 5>> states;
	spk::RenderSnapshot::Builder nested;
	nested.renderPass({"clear", 0}).emplace<spk::ClearRenderCommand>(
		spk::Color{0.25f, 0.5f, 0.75f, 1.0f},
		spk::ClearRenderCommand::Mask::Color);
	nested.renderPass({"inspect", 1}).emplace<InspectFramebufferCommand>(states);
	spk::RenderSnapshot::Builder outer;
	outer.renderPass({"offscreen", 0}).emplace<spk::RenderToFramebufferCommand>(
		target.handle(),
		std::make_shared<spk::RenderSnapshot>(nested.build()));
	outer.build().execute(gl.renderContext());

	ASSERT_EQ(states.size(), 1u);
	EXPECT_GT(states[0][0], 0);
	EXPECT_EQ((std::array<GLint, 4>{states[0][1], states[0][2], states[0][3], states[0][4]}),
			  (std::array<GLint, 4>{0, 0, 4, 3}));
	GLint draw = -1, read = -1;
	std::array<GLint, 4> viewport{};
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
	glGetIntegerv(GL_VIEWPORT, viewport.data());
	EXPECT_EQ(draw, 0);
	EXPECT_EQ(read, previousName);
	EXPECT_EQ(viewport, (std::array<GLint, 4>{2, 3, 17, 19}));

	target.activate(gl.renderContext());
	std::array<std::uint8_t, 4> pixel{};
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
	EXPECT_NEAR(pixel[0], 64, 1);
	EXPECT_NEAR(pixel[1], 128, 1);
	EXPECT_NEAR(pixel[2], 191, 1);
	EXPECT_EQ(pixel[3], 255);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(RenderToFramebufferCommandTest, NestedFailureRestoresReadDrawAndViewport)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	spk::Framebuffer target({3, 2}, {spk::Texture::Format::RGBA}, std::nullopt);
	spk::Framebuffer previous({2, 2}, {spk::Texture::Format::RGBA}, std::nullopt);
	previous.activate(gl.renderContext());
	GLint previousName = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousName);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glViewport(5, 6, 22, 23);

	spk::RenderSnapshot::Builder builder;
	builder.renderPass({"failure", 0}).emplace<FailingFramebufferCommand>();
	spk::RenderToFramebufferCommand command(
		target.handle(),
		std::make_shared<spk::RenderSnapshot>(builder.build()));
	EXPECT_THROW(command.execute(gl.renderContext()), spk::Exception);

	GLint draw = -1, read = -1;
	std::array<GLint, 4> viewport{};
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
	glGetIntegerv(GL_VIEWPORT, viewport.data());
	EXPECT_EQ(draw, previousName);
	EXPECT_EQ(read, 0);
	EXPECT_EQ(viewport, (std::array<GLint, 4>{5, 6, 22, 23}));
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(RenderToFramebufferCommandTest, CapturedFramebufferVersionSurvivesResize)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset();
	spk::Framebuffer target({4, 3});
	std::vector<std::array<GLint, 5>> states;
	spk::RenderSnapshot::Builder nested;
	nested.renderPass({"inspect", 0}).emplace<InspectFramebufferCommand>(states);
	spk::RenderToFramebufferCommand command(
		target.handle(),
		std::make_shared<spk::RenderSnapshot>(nested.build()));
	target.resize({8, 7});

	command.execute(gl.renderContext());
	ASSERT_EQ(states.size(), 1u);
	EXPECT_EQ(states[0][3], 4);
	EXPECT_EQ(states[0][4], 3);
	EXPECT_EQ(target.size(), spk::Vector2UInt(8, 7));
}

TEST(RenderToFramebufferCommandTest, RejectsMissingInputs)
{
	spk::Framebuffer framebuffer({4, 4});
	auto snapshot = makeClearSnapshot({0, 0, 0, 1});
	EXPECT_THROW(
		(void)spk::RenderToFramebufferCommand(spk::Framebuffer::Handle{}, snapshot),
		std::invalid_argument);
	EXPECT_THROW(
		(void)spk::RenderToFramebufferCommand(framebuffer.handle(), nullptr),
		std::invalid_argument);
}
